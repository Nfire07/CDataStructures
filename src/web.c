#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "../include/web.h"
#include "../include/pointers.h"
#include "../include/strings.h"
#include "../include/arrays.h"
#include "../include/maps.h"

#define MG_ENABLE_MBUF_RELOCATE 1
#include "../include/core/mongoose.h"

#include <curl/curl.h>
#include <cjson/cJSON.h>

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

static bool            _webTimingEnabled = true;
static double          _webLastElapsedMs = 0.0;

static void _webTimerBegin(struct timespec *ts) {
    if (_webTimingEnabled)
        clock_gettime(CLOCK_MONOTONIC, ts);
}

static void _webTimerEnd(const struct timespec *start) {
    if (!_webTimingEnabled) return;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    _webLastElapsedMs = (now.tv_sec  - start->tv_sec)  * 1000.0
                      + (now.tv_nsec - start->tv_nsec) / 1e6;
}

void webTimingEnable(bool enabled) {
    _webTimingEnabled = enabled;
}

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

static HashMap _strHashMapCreate(size_t capacity) {
    HashMap m = mapCreate(64, sizeof(String), capacity);
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
    HashMap map = _strHashMapCreate(WEB_QUERY_CAPACITY);
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
    req->params = _strHashMapCreate(WEB_PARAM_CAPACITY);
    req->headers = _strHashMapCreate(WEB_HEADER_CAPACITY);

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
    res->headers = _strHashMapCreate(WEB_HEADER_CAPACITY);
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

            HashMap tempParams = _strHashMapCreate(WEB_PARAM_CAPACITY);
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

    cJSON *root = cJSON_Parse(json);
    if (!root) return map;

    cJSON *item = NULL;
    cJSON_ArrayForEach(item, root) {
        if (!item->string) continue;

        char *kCopy = (char *)xMalloc(strlen(item->string) + 1);
        strcpy(kCopy, item->string);

        String vStr = NULL;
        if (cJSON_IsString(item) && item->valuestring) {
            vStr = stringNew(item->valuestring);
        } else if (cJSON_IsNumber(item)) {
            char numBuf[64];
            if (item->valuedouble == (double)(long long)item->valuedouble)
                snprintf(numBuf, sizeof(numBuf), "%lld", (long long)item->valuedouble);
            else
                snprintf(numBuf, sizeof(numBuf), "%g", item->valuedouble);
            vStr = stringNew(numBuf);
        } else if (cJSON_IsBool(item)) {
            vStr = stringNew(cJSON_IsTrue(item) ? "true" : "false");
        } else if (cJSON_IsNull(item)) {
            vStr = stringNew("null");
        } else {
            char *printed = cJSON_PrintUnformatted(item);
            vStr = printed ? stringNew(printed) : stringNew("");
            if (printed) free(printed);
        }

        mapPut(map, kCopy, &vStr);
        xFree(kCopy);
    }

    cJSON_Delete(root);
    return map;
}

typedef struct {
    char *data;
    size_t size;
} _CurlBuffer;

static size_t _curlWriteBody(void *ptr, size_t size, size_t nmemb, void *userdata) {
    _CurlBuffer *buf = (_CurlBuffer *)userdata;
    size_t realSize = size * nmemb;
    char *newData = (char *)realloc(buf->data, buf->size + realSize + 1);
    if (!newData) return 0;
    buf->data = newData;
    memcpy(buf->data + buf->size, ptr, realSize);
    buf->size += realSize;
    buf->data[buf->size] = '\0';
    return realSize;
}

static size_t _curlWriteHeaders(char *buffer, size_t size, size_t nitems, void *userdata) {
    HashMap headers = (HashMap)userdata;
    size_t total = size * nitems;

    char *line = (char *)malloc(total + 1);
    memcpy(line, buffer, total);
    line[total] = '\0';

    char *colon = strchr(line, ':');
    if (colon && colon != line) {
        *colon = '\0';
        char *key = line;
        char *val = colon + 1;
        while (*val == ' ') val++;
        size_t vlen = strlen(val);
        while (vlen > 0 && (val[vlen-1] == '\r' || val[vlen-1] == '\n' || val[vlen-1] == ' '))
            val[--vlen] = '\0';

        char *kLower = (char *)malloc(strlen(key) + 1);
        for (size_t i = 0; key[i]; i++) kLower[i] = (char)tolower((unsigned char)key[i]);
        kLower[strlen(key)] = '\0';

        char *kCopy = (char *)xMalloc(strlen(kLower) + 1);
        strcpy(kCopy, kLower);
        String vStr = stringNew(val);
        mapPut(headers, kCopy, &vStr);
        xFree(kCopy);
        free(kLower);
    }

    free(line);
    return total;
}

WebClientOptions webClientOptionsCreate(void) {
    WebClientOptions opts;
    opts.headers = _strHashMapCreate(WEB_HEADER_CAPACITY);
    opts.body = NULL;
    opts.timeoutMs = 30000;
    opts.followRedirects = true;
    opts.verifySsl = true;
    return opts;
}

void webClientOptionsFree(WebClientOptions *opts) {
    if (!opts) return;
    if (opts->headers) mapFree(opts->headers, NULL, NULL);
    if (opts->body) stringFree(opts->body);
    opts->headers = NULL;
    opts->body = NULL;
}

void webClientOptionsSetHeader(WebClientOptions *opts, const char *key, const char *value) {
    if (!opts || !key) return;
    char *kCopy = (char *)xMalloc(strlen(key) + 1);
    strcpy(kCopy, key);
    String vStr = stringNew(value ? value : "");
    mapPut(opts->headers, kCopy, &vStr);
    xFree(kCopy);
}

void webClientOptionsSetBody(WebClientOptions *opts, const char *body) {
    if (!opts) return;
    if (opts->body) stringFree(opts->body);
    opts->body = body ? stringNew(body) : NULL;
}

WebClientResponse webClientRequest(const char *method, const char *url, WebClientOptions *opts) {
    WebClientResponse result;
    result.status = 0;
    result.body = NULL;
    result.headers = _strHashMapCreate(WEB_HEADER_CAPACITY);
    result.ok = false;
    result.error = NULL;

    if (!url || !method) {
        result.error = stringNew("Invalid URL or method");
        return result;
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        result.error = stringNew("Failed to initialize libcurl");
        return result;
    }

    _CurlBuffer bodyBuf = {NULL, 0};
    bodyBuf.data = (char *)malloc(1);
    bodyBuf.data[0] = '\0';

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _curlWriteBody);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &bodyBuf);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, _curlWriteHeaders);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, result.headers);

    long timeoutMs = opts ? opts->timeoutMs : 30000;
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeoutMs);

    bool followRedirects = opts ? opts->followRedirects : true;
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, followRedirects ? 1L : 0L);

    bool verifySsl = opts ? opts->verifySsl : true;
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, verifySsl ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, verifySsl ? 2L : 0L);

    struct curl_slist *curlHeaders = NULL;

    if (opts && opts->headers) {
        for (size_t i = 0; i < opts->headers->capacity; i++) {
            MapEntry *entry = opts->headers->buckets[i];
            while (entry) {
                char *k = (char *)entry->key;
                String *vPtr = (String *)entry->value;
                if (vPtr && *vPtr) {
                    size_t hLen = strlen(k) + 2 + stringLength(*vPtr) + 1;
                    char *hLine = (char *)malloc(hLen);
                    snprintf(hLine, hLen, "%s: %s", k, stringGetData(*vPtr));
                    curlHeaders = curl_slist_append(curlHeaders, hLine);
                    free(hLine);
                }
                entry = entry->next;
            }
        }
    }

    const char *bodyData = NULL;
    size_t bodyLen = 0;
    if (opts && opts->body) {
        bodyData = stringGetData(opts->body);
        bodyLen = stringLength(opts->body);
    }

    if (strcmp(method, WEB_METHOD_GET) == 0) {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    } else if (strcmp(method, WEB_METHOD_POST) == 0) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, bodyData ? bodyData : "");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)bodyLen);
    } else if (strcmp(method, WEB_METHOD_PUT) == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        if (bodyData) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, bodyData);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)bodyLen);
        }
    } else if (strcmp(method, WEB_METHOD_DELETE) == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    } else if (strcmp(method, WEB_METHOD_PATCH) == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
        if (bodyData) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, bodyData);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)bodyLen);
        }
    } else {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
        if (bodyData) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, bodyData);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)bodyLen);
        }
    }

    if (curlHeaders)
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curlHeaders);

    struct timespec _t0;
    _webTimerBegin(&_t0);
    CURLcode res = curl_easy_perform(curl);
    _webTimerEnd(&_t0);

    if (res != CURLE_OK) {
        result.error = stringNew(curl_easy_strerror(res));
        result.body = stringNew("");
    } else {
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        result.status = (int)httpCode;
        result.body = stringNew(bodyBuf.data ? bodyBuf.data : "");
        result.ok = (httpCode >= 200 && httpCode < 300);
    }

    if (curlHeaders) curl_slist_free_all(curlHeaders);
    free(bodyBuf.data);
    curl_easy_cleanup(curl);

    return result;
}

WebClientResponse webClientGet(const char *url, WebClientOptions *opts) {
    return webClientRequest(WEB_METHOD_GET, url, opts);
}

WebClientResponse webClientPost(const char *url, const char *jsonBody, WebClientOptions *opts) {
    WebClientOptions defaultOpts;
    bool created = false;

    if (!opts) {
        defaultOpts = webClientOptionsCreate();
        opts = &defaultOpts;
        created = true;
    }

    if (jsonBody) webClientOptionsSetBody(opts, jsonBody);

    char *existingCT = NULL;
    if (opts->headers) {
        char *ctKey = (char *)xMalloc(13);
        strcpy(ctKey, "Content-Type");
        existingCT = (char *)mapGet(opts->headers, ctKey);
        xFree(ctKey);
    }
    if (!existingCT)
        webClientOptionsSetHeader(opts, "Content-Type", "application/json");

    WebClientResponse result = webClientRequest(WEB_METHOD_POST, url, opts);

    if (created) webClientOptionsFree(opts);
    return result;
}

WebClientResponse webClientPut(const char *url, const char *jsonBody, WebClientOptions *opts) {
    WebClientOptions defaultOpts;
    bool created = false;

    if (!opts) {
        defaultOpts = webClientOptionsCreate();
        opts = &defaultOpts;
        created = true;
    }

    if (jsonBody) webClientOptionsSetBody(opts, jsonBody);
    webClientOptionsSetHeader(opts, "Content-Type", "application/json");

    WebClientResponse result = webClientRequest(WEB_METHOD_PUT, url, opts);

    if (created) webClientOptionsFree(opts);
    return result;
}

WebClientResponse webClientDelete(const char *url, WebClientOptions *opts) {
    return webClientRequest(WEB_METHOD_DELETE, url, opts);
}

WebClientResponse webClientPatch(const char *url, const char *jsonBody, WebClientOptions *opts) {
    WebClientOptions defaultOpts;
    bool created = false;

    if (!opts) {
        defaultOpts = webClientOptionsCreate();
        opts = &defaultOpts;
        created = true;
    }

    if (jsonBody) webClientOptionsSetBody(opts, jsonBody);
    webClientOptionsSetHeader(opts, "Content-Type", "application/json");

    WebClientResponse result = webClientRequest(WEB_METHOD_PATCH, url, opts);

    if (created) webClientOptionsFree(opts);
    return result;
}

void webClientResponseFree(WebClientResponse *res) {
    if (!res) return;
    if (res->body) stringFree(res->body);
    if (res->headers) mapFree(res->headers, NULL, NULL);
    if (res->error) stringFree(res->error);
    res->body = NULL;
    res->headers = NULL;
    res->error = NULL;
}

HashMap webClientParseResponseJson(WebClientResponse *res) {
    if (!res || !res->body) return webParseJson(NULL);
    return webParseJson(stringGetData(res->body));
}

String webClientGetResponseHeader(WebClientResponse *res, const char *key) {
    if (!res || !res->headers || !key) return NULL;
    char *kLower = (char *)xMalloc(strlen(key) + 1);
    for (size_t i = 0; key[i]; i++) kLower[i] = (char)tolower((unsigned char)key[i]);
    kLower[strlen(key)] = '\0';
    String *val = (String *)mapGet(res->headers, kLower);
    xFree(kLower);
    return val ? *val : NULL;
}

String webJsonBuildObject(WebJsonField *fields, size_t count) {
    if (!fields || count == 0) return stringNew("{}");

    cJSON *root = cJSON_CreateObject();
    for (size_t i = 0; i < count; i++) {
        if (fields[i].key && fields[i].value)
            cJSON_AddStringToObject(root, fields[i].key, fields[i].value);
    }

    char *printed = cJSON_PrintUnformatted(root);
    String result = printed ? stringNew(printed) : stringNew("{}");
    if (printed) free(printed);
    cJSON_Delete(root);
    return result;
}

String webJsonBuildArray(String *items, size_t count) {
    if (!items || count == 0) return stringNew("[]");

    cJSON *arr = cJSON_CreateArray();
    for (size_t i = 0; i < count; i++) {
        if (items[i])
            cJSON_AddItemToArray(arr, cJSON_CreateString(stringGetData(items[i])));
    }

    char *printed = cJSON_PrintUnformatted(arr);
    String result = printed ? stringNew(printed) : stringNew("[]");
    if (printed) free(printed);
    cJSON_Delete(arr);
    return result;
}

String webJsonGetString(HashMap json, const char *key) {
    if (!json || !key) return NULL;
    char *kCopy = (char *)xMalloc(strlen(key) + 1);
    strcpy(kCopy, key);
    String *val = (String *)mapGet(json, kCopy);
    xFree(kCopy);
    return val ? *val : NULL;
}

int webJsonGetInt(HashMap json, const char *key) {
    String val = webJsonGetString(json, key);
    return val ? stringParseInt(val) : 0;
}

double webJsonGetDouble(HashMap json, const char *key) {
    String val = webJsonGetString(json, key);
    return val ? stringParseDouble(val) : 0.0;
}

bool webJsonGetBool(HashMap json, const char *key) {
    String val = webJsonGetString(json, key);
    if (!val) return false;
    const char *s = stringGetData(val);
    return s && strcmp(s, "true") == 0;
}

double webGetElapsedMs(void) {
    return _webLastElapsedMs;
}

double webGetElapsedS(void) {
    return _webLastElapsedMs / 1000.0;
}
