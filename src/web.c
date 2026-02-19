#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../include/web.h"
#include "../include/pointers.h"
#include "../include/strings.h"
#include "../include/arrays.h"
#include "../include/maps.h"

#define MG_ENABLE_MBUF_RELOCATE 1
#include "../include/core/mongoose.h"

#define WEB_DEFAULT_MAX_BODY 4194304
#define WEB_ROUTE_CAPACITY   64
#define WEB_HEADER_CAPACITY  32
#define WEB_QUERY_CAPACITY   32
#define WEB_PARAM_CAPACITY   16

struct WebServerStruct {
    struct mg_mgr mgr;
    char *address;
    Array routes;
    bool running;
    bool corsEnabled;
    size_t maxBodySize;
};

static void _routeFree(void *elem) {
    WebRoute *r = (WebRoute *)elem;
    if (r->method) stringFree(r->method);
    if (r->pattern) stringFree(r->pattern);
}

static void _requestFree(WebRequest *req) {
    if (!req) return;
    if (req->method) stringFree(req->method);
    if (req->path) stringFree(req->path);
    if (req->body) stringFree(req->body);
    if (req->headers) mapFree(req->headers, NULL, NULL);
    if (req->query) mapFree(req->query, NULL, NULL);
    if (req->params) mapFree(req->params, NULL, NULL);
    xFree(req);
}

static void _responseFree(WebResponse *res) {
    if (!res) return;
    if (res->body) stringFree(res->body);
    if (res->headers) mapFree(res->headers, NULL, NULL);
    xFree(res);
}

static HashMap _headersMapCreate(void) {
    HashMap m = mapCreate(64, sizeof(String), WEB_HEADER_CAPACITY);
    m->hashFunc = hashString;
    m->keyEquals = keyEqualsString;
    return m;
}

static HashMap _queryMapCreate(void) {
    HashMap m = mapCreate(64, sizeof(String), WEB_QUERY_CAPACITY);
    m->hashFunc = hashString;
    m->keyEquals = keyEqualsString;
    return m;
}

static HashMap _paramsMapCreate(void) {
    HashMap m = mapCreate(64, sizeof(String), WEB_PARAM_CAPACITY);
    m->hashFunc = hashString;
    m->keyEquals = keyEqualsString;
    return m;
}

static bool _matchRoute(const char *pattern, const char *path, HashMap params) {
    const char *p = pattern;
    const char *u = path;

    while (*p && *u) {
        if (*p == ':') {
            p++;
            char key[128];
            int ki = 0;
            while (*p && *p != '/') key[ki++] = *p++;
            key[ki] = '\0';

            char val[256];
            int vi = 0;
            while (*u && *u != '/') val[vi++] = *u++;
            val[vi] = '\0';

            if (params) {
                char *kCopy = (char *)xMalloc(strlen(key) + 1);
                strcpy(kCopy, key);
                String vStr = stringNew(val);
                mapPut(params, kCopy, &vStr);
                xFree(kCopy);
            }
        } else if (*p == '*') {
            return true;
        } else {
            if (*p != *u) return false;
            p++;
            u++;
        }
    }

    while (*p == '/') p++;
    while (*u == '/') u++;

    return (*p == '\0' && *u == '\0');
}

static HashMap _parseQueryString(const char *qs) {
    HashMap map = _queryMapCreate();
    if (!qs || *qs == '\0') return map;

    char *buf = (char *)xMalloc(strlen(qs) + 1);
    strcpy(buf, qs);

    char *token = strtok(buf, "&");
    while (token) {
        char *eq = strchr(token, '=');
        if (eq) {
            *eq = '\0';
            char *k = token;
            char *v = eq + 1;

            char *kCopy = (char *)xMalloc(strlen(k) + 1);
            strcpy(kCopy, k);
            String vStr = stringNew(v);
            mapPut(map, kCopy, &vStr);
            xFree(kCopy);
        }
        token = strtok(NULL, "&");
    }

    xFree(buf);
    return map;
}

static WebRequest *_buildRequest(struct mg_http_message *hm) {
    WebRequest *req = (WebRequest *)xMalloc(sizeof(WebRequest));

    req->method = stringNew("");
    if (hm->method.len > 0) {
        char tmp[16] = {0};
        size_t l = hm->method.len < 15 ? hm->method.len : 15;
        memcpy(tmp, hm->method.buf, l);
        stringFree(req->method);
        req->method = stringNew(tmp);
    }

    char pathBuf[1024] = {0};
    char queryBuf[2048] = {0};
    size_t uriLen = hm->uri.len < 1023 ? hm->uri.len : 1023;
    memcpy(pathBuf, hm->uri.buf, uriLen);

    if (hm->query.len > 0) {
        size_t qLen = hm->query.len < 2047 ? hm->query.len : 2047;
        memcpy(queryBuf, hm->query.buf, qLen);
    }

    req->path = stringNew(pathBuf);
    req->query = _parseQueryString(queryBuf);
    req->params = _paramsMapCreate();
    req->headers = _headersMapCreate();

    for (int i = 0; i < MG_MAX_HTTP_HEADERS; i++) {
        struct mg_str name = hm->headers[i].name;
        struct mg_str value = hm->headers[i].value;
        if (name.len == 0) break;

        char *k = (char *)xMalloc(name.len + 1);
        memcpy(k, name.buf, name.len);
        k[name.len] = '\0';

        char vTmp[1024] = {0};
        size_t vLen = value.len < 1023 ? value.len : 1023;
        memcpy(vTmp, value.buf, vLen);
        String vStr = stringNew(vTmp);

        mapPut(req->headers, k, &vStr);
        xFree(k);
    }

    if (hm->body.len > 0) {
        char *bodyBuf = (char *)xMalloc(hm->body.len + 1);
        memcpy(bodyBuf, hm->body.buf, hm->body.len);
        bodyBuf[hm->body.len] = '\0';
        req->body = stringNew(bodyBuf);
        xFree(bodyBuf);
    } else {
        req->body = stringNew("");
    }

    return req;
}

static WebResponse *_responseCreate(void) {
    WebResponse *res = (WebResponse *)xMalloc(sizeof(WebResponse));
    res->status = 200;
    res->body = stringNew("");
    res->headers = _headersMapCreate();
    return res;
}

static void _sendResponse(struct mg_connection *c, WebResponse *res, bool corsEnabled) {
    String headers = stringNew("");

    if (corsEnabled) {
        String corsH = stringNew(
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, PATCH, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
        );
        headers = stringAppend(headers, corsH);
        stringFree(corsH);
    }

    for (size_t i = 0; i < res->headers->capacity; i++) {
        MapEntry *entry = res->headers->buckets[i];
        while (entry) {
            char *k = (char *)entry->key;
            String *vPtr = (String *)entry->value;
            if (vPtr && *vPtr) {
                String line = stringNew(k);
                String colon = stringNew(": ");
                String crlf = stringNew("\r\n");
                line = stringAppend(line, colon);
                line = stringAppend(line, *vPtr);
                line = stringAppend(line, crlf);
                headers = stringAppend(headers, line);
                stringFree(colon);
                stringFree(crlf);
                stringFree(line);
            }
            entry = entry->next;
        }
    }

    const char *bodyData = res->body ? stringGetData(res->body) : "";
    size_t bodyLen = res->body ? stringLength(res->body) : 0;

    mg_http_reply(c, res->status, stringGetData(headers), "%.*s", (int)bodyLen, bodyData);
    stringFree(headers);
}

static void _handleOptions(struct mg_connection *c, bool corsEnabled) {
    if (corsEnabled) {
        mg_http_reply(c, 204,
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, PATCH, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
            "Content-Length: 0\r\n",
            "");
    } else {
        mg_http_reply(c, 204, "Content-Length: 0\r\n", "");
    }
}

static void _webEventHandler(struct mg_connection *c, int ev, void *ev_data) {
    WebServer server = (WebServer)c->fn_data;

    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;

        char methodBuf[16] = {0};
        size_t ml = hm->method.len < 15 ? hm->method.len : 15;
        memcpy(methodBuf, hm->method.buf, ml);

        if (strcmp(methodBuf, "OPTIONS") == 0) {
            _handleOptions(c, server->corsEnabled);
            return;
        }

        char pathBuf[1024] = {0};
        size_t pl = hm->uri.len < 1023 ? hm->uri.len : 1023;
        memcpy(pathBuf, hm->uri.buf, pl);

        for (size_t i = 0; i < server->routes->len; i++) {
            WebRoute *route = (WebRoute *)arrayGetRef(server->routes, (int)i);
            if (!route) continue;

            const char *routeMethod = stringGetData(route->method);
            const char *routePattern = stringGetData(route->pattern);

            if (strcmp(routeMethod, methodBuf) != 0) continue;

            HashMap tempParams = _paramsMapCreate();
            if (_matchRoute(routePattern, pathBuf, tempParams)) {
                WebRequest *req = _buildRequest(hm);
                mapFree(req->params, NULL, NULL);
                req->params = tempParams;

                WebResponse *res = _responseCreate();
                route->handler(req, res);

                _sendResponse(c, res, server->corsEnabled);

                _requestFree(req);
                _responseFree(res);
                return;
            }
            mapFree(tempParams, NULL, NULL);
        }

        WebResponse *notFound = _responseCreate();
        notFound->status = 404;
        stringFree(notFound->body);
        notFound->body = stringNew("{\"error\":\"Not Found\"}");
        String ctKey = stringNew("Content-Type");
        String ctVal = stringNew("application/json");
        mapPut(notFound->headers, stringGetData(ctKey), &ctVal);
        stringFree(ctKey);
        _sendResponse(c, notFound, server->corsEnabled);
        _responseFree(notFound);
    }
}

WebServer webServerCreate(const char *address, int port) {
    WebServer server = (WebServer)xMalloc(sizeof(struct WebServerStruct));

    size_t addrLen = strlen(address);
    char portStr[8];
    snprintf(portStr, sizeof(portStr), "%d", port);

    size_t fullLen = addrLen + 1 + strlen(portStr) + 1;
    server->address = (char *)xMalloc(fullLen);
    snprintf(server->address, fullLen, "%s:%s", address, portStr);

    server->routes = array(sizeof(WebRoute));
    server->running = false;
    server->corsEnabled = false;
    server->maxBodySize = WEB_DEFAULT_MAX_BODY;

    mg_mgr_init(&server->mgr);

    return server;
}

void webServerFree(WebServer server) {
    if (!server) return;
    mg_mgr_free(&server->mgr);
    arrayFree(server->routes, _routeFree);
    xFree(server->address);
    xFree(server);
}

static void _addRoute(WebServer server, const char *method, const char *path, WebHandler handler) {
    WebRoute route;
    route.method = stringNew(method);
    route.pattern = stringNew(path);
    route.handler = handler;
    arrayAdd(server->routes, &route);
}

void webServerGet(WebServer server, const char *path, WebHandler handler) {
    _addRoute(server, WEB_METHOD_GET, path, handler);
}

void webServerPost(WebServer server, const char *path, WebHandler handler) {
    _addRoute(server, WEB_METHOD_POST, path, handler);
}

void webServerPut(WebServer server, const char *path, WebHandler handler) {
    _addRoute(server, WEB_METHOD_PUT, path, handler);
}

void webServerDelete(WebServer server, const char *path, WebHandler handler) {
    _addRoute(server, WEB_METHOD_DELETE, path, handler);
}

void webServerPatch(WebServer server, const char *path, WebHandler handler) {
    _addRoute(server, WEB_METHOD_PATCH, path, handler);
}

void webServerStart(WebServer server) {
    if (!server) return;
    mg_http_listen(&server->mgr, server->address, _webEventHandler, server);
    server->running = true;
}

void webServerStop(WebServer server) {
    if (!server) return;
    server->running = false;
}

void webServerPoll(WebServer server, int timeout_ms) {
    if (!server) return;
    mg_mgr_poll(&server->mgr, timeout_ms);
}

void webServerRun(WebServer server) {
    if (!server) return;
    webServerStart(server);
    while (server->running) {
        mg_mgr_poll(&server->mgr, 100);
    }
}

void webServerSetCORS(WebServer server, bool enabled) {
    if (server) server->corsEnabled = enabled;
}

void webServerSetMaxBodySize(WebServer server, size_t maxBytes) {
    if (server) server->maxBodySize = maxBytes;
}

void webResponseSetStatus(WebResponse *res, int status) {
    if (res) res->status = status;
}

void webResponseSetBody(WebResponse *res, const char *body) {
    if (!res) return;
    if (res->body) stringFree(res->body);
    res->body = stringNew(body ? body : "");
}

void webResponseSetBodyString(WebResponse *res, String body) {
    if (!res) return;
    if (res->body) stringFree(res->body);
    res->body = body;
}

void webResponseSetHeader(WebResponse *res, const char *key, const char *value) {
    if (!res || !key) return;
    String vStr = stringNew(value ? value : "");
    char *kCopy = (char *)xMalloc(strlen(key) + 1);
    strcpy(kCopy, key);
    mapPut(res->headers, kCopy, &vStr);
    xFree(kCopy);
}

void webResponseJson(WebResponse *res, int status, const char *json) {
    if (!res) return;
    res->status = status;
    if (res->body) stringFree(res->body);
    res->body = stringNew(json ? json : "{}");
    webResponseSetHeader(res, "Content-Type", "application/json");
}

void webResponseText(WebResponse *res, int status, const char *text) {
    if (!res) return;
    res->status = status;
    if (res->body) stringFree(res->body);
    res->body = stringNew(text ? text : "");
    webResponseSetHeader(res, "Content-Type", "text/plain");
}

void webResponseEmpty(WebResponse *res, int status) {
    if (!res) return;
    res->status = status;
    if (res->body) stringFree(res->body);
    res->body = stringNew("");
}

String webRequestGetHeader(WebRequest *req, const char *key) {
    if (!req || !key) return NULL;
    char *kCopy = (char *)xMalloc(strlen(key) + 1);
    strcpy(kCopy, key);
    String *val = (String *)mapGet(req->headers, kCopy);
    xFree(kCopy);
    return val ? *val : NULL;
}

String webRequestGetQuery(WebRequest *req, const char *key) {
    if (!req || !key) return NULL;
    char *kCopy = (char *)xMalloc(strlen(key) + 1);
    strcpy(kCopy, key);
    String *val = (String *)mapGet(req->query, kCopy);
    xFree(kCopy);
    return val ? *val : NULL;
}

String webRequestGetParam(WebRequest *req, const char *key) {
    if (!req || !key) return NULL;
    char *kCopy = (char *)xMalloc(strlen(key) + 1);
    strcpy(kCopy, key);
    String *val = (String *)mapGet(req->params, kCopy);
    xFree(kCopy);
    return val ? *val : NULL;
}

String webEncodeUrl(const char *str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    char *out = (char *)xMalloc(len * 3 + 1);
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)str[i];
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out[j++] = (char)c;
        } else {
            out[j++] = '%';
            out[j++] = "0123456789ABCDEF"[c >> 4];
            out[j++] = "0123456789ABCDEF"[c & 15];
        }
    }
    out[j] = '\0';
    String result = stringNew(out);
    xFree(out);
    return result;
}

String webDecodeUrl(const char *str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    char *out = (char *)xMalloc(len + 1);
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (str[i] == '%' && i + 2 < len) {
            char h[3] = {str[i+1], str[i+2], '\0'};
            out[j++] = (char)strtol(h, NULL, 16);
            i += 2;
        } else if (str[i] == '+') {
            out[j++] = ' ';
        } else {
            out[j++] = str[i];
        }
    }
    out[j] = '\0';
    String result = stringNew(out);
    xFree(out);
    return result;
}

String webBuildJson(HashMap map) {
    if (!map) return stringNew("{}");

    String json = stringNew("{");
    bool first = true;

    for (size_t i = 0; i < map->capacity; i++) {
        MapEntry *entry = map->buckets[i];
        while (entry) {
            char *k = (char *)entry->key;
            String *vPtr = (String *)entry->value;

            if (!first) {
                String comma = stringNew(",");
                json = stringAppend(json, comma);
                stringFree(comma);
            }
            first = false;

            String keyPart = stringNew("\"");
            String keyStr = stringNew(k);
            String colon = stringNew("\":\"");
            String closing = stringNew("\"");

            json = stringAppend(json, keyPart);
            stringFree(keyPart);
            json = stringAppend(json, keyStr);
            stringFree(keyStr);
            json = stringAppend(json, colon);
            stringFree(colon);

            if (vPtr && *vPtr) {
                json = stringAppend(json, *vPtr);
            } else {
                String nullStr = stringNew("null");
                json = stringAppend(json, nullStr);
                stringFree(nullStr);
            }

            json = stringAppend(json, closing);
            stringFree(closing);

            entry = entry->next;
        }
    }

    String end = stringNew("}");
    json = stringAppend(json, end);
    stringFree(end);
    return json;
}

HashMap webParseJson(const char *json) {
    HashMap map = mapCreate(64, sizeof(String), 16);
    map->hashFunc = hashString;
    map->keyEquals = keyEqualsString;

    if (!json) return map;

    const char *p = json;
    while (*p && *p != '{') p++;
    if (*p == '{') p++;

    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == ',') p++;
        if (*p == '}' || *p == '\0') break;

        if (*p != '"') { p++; continue; }
        p++;

        char key[256];
        int ki = 0;
        while (*p && *p != '"' && ki < 255) key[ki++] = *p++;
        key[ki] = '\0';
        if (*p == '"') p++;

        while (*p == ' ' || *p == ':') p++;

        char val[1024];
        int vi = 0;
        bool isString = false;

        if (*p == '"') {
            isString = true;
            p++;
            while (*p && *p != '"' && vi < 1023) {
                if (*p == '\\') { p++; if (*p) val[vi++] = *p++; }
                else val[vi++] = *p++;
            }
            if (*p == '"') p++;
        } else {
            while (*p && *p != ',' && *p != '}' && vi < 1023) val[vi++] = *p++;
        }
        val[vi] = '\0';

        char *kCopy = (char *)xMalloc(strlen(key) + 1);
        strcpy(kCopy, key);
        String vStr = stringNew(val);
        mapPut(map, kCopy, &vStr);
        xFree(kCopy);

        (void)isString;
    }

    return map;
}