#ifndef WEB_H
#define WEB_H

#include <stdbool.h>
#include <stddef.h>
#include "strings.h"
#include "arrays.h"
#include "maps.h"

#define WEB_METHOD_GET     "GET"
#define WEB_METHOD_POST    "POST"
#define WEB_METHOD_PUT     "PUT"
#define WEB_METHOD_DELETE  "DELETE"
#define WEB_METHOD_PATCH   "PATCH"

#define WEB_STATUS_200 200
#define WEB_STATUS_201 201
#define WEB_STATUS_204 204
#define WEB_STATUS_400 400
#define WEB_STATUS_401 401
#define WEB_STATUS_403 403
#define WEB_STATUS_404 404
#define WEB_STATUS_405 405
#define WEB_STATUS_500 500

typedef struct WebRequest {
    String method;
    String path;
    String body;
    HashMap headers;
    HashMap query;
    HashMap params;
} WebRequest;

typedef struct WebResponse {
    int status;
    String body;
    HashMap headers;
} WebResponse;

typedef void (*WebHandler)(WebRequest *req, WebResponse *res);

typedef struct WebRoute {
    String method;
    String pattern;
    WebHandler handler;
} WebRoute;

typedef struct WebServerStruct WebServerStruct;
typedef WebServerStruct *WebServer;

WebServer webServerCreate(const char *address, int port);
void webServerFree(WebServer server);

void webServerGet(WebServer server, const char *path, WebHandler handler);
void webServerPost(WebServer server, const char *path, WebHandler handler);
void webServerPut(WebServer server, const char *path, WebHandler handler);
void webServerDelete(WebServer server, const char *path, WebHandler handler);
void webServerPatch(WebServer server, const char *path, WebHandler handler);

void webServerStart(WebServer server);
void webServerStop(WebServer server);
void webServerPoll(WebServer server, int timeout_ms);
void webServerRun(WebServer server);

void webServerSetCORS(WebServer server, bool enabled);
void webServerSetMaxBodySize(WebServer server, size_t maxBytes);

void webResponseSetStatus(WebResponse *res, int status);
void webResponseSetBody(WebResponse *res, const char *body);
void webResponseSetBodyString(WebResponse *res, String body);
void webResponseSetHeader(WebResponse *res, const char *key, const char *value);
void webResponseJson(WebResponse *res, int status, const char *json);
void webResponseText(WebResponse *res, int status, const char *text);
void webResponseEmpty(WebResponse *res, int status);

String webRequestGetHeader(WebRequest *req, const char *key);
String webRequestGetQuery(WebRequest *req, const char *key);
String webRequestGetParam(WebRequest *req, const char *key);

String webBuildJson(HashMap map);
HashMap webParseJson(const char *json);
String webEncodeUrl(const char *str);
String webDecodeUrl(const char *str);

typedef struct WebClientResponse {
    int status;
    String body;
    HashMap headers;
    bool ok;
    String error;
} WebClientResponse;

typedef struct WebClientOptions {
    HashMap headers;
    String body;
    long timeoutMs;
    bool followRedirects;
    bool verifySsl;
} WebClientOptions;

WebClientOptions webClientOptionsCreate(void);
void webClientOptionsFree(WebClientOptions *opts);
void webClientOptionsSetHeader(WebClientOptions *opts, const char *key, const char *value);
void webClientOptionsSetBody(WebClientOptions *opts, const char *body);

WebClientResponse webClientRequest(const char *method, const char *url, WebClientOptions *opts);
WebClientResponse webClientGet(const char *url, WebClientOptions *opts);
WebClientResponse webClientPost(const char *url, const char *jsonBody, WebClientOptions *opts);
WebClientResponse webClientPut(const char *url, const char *jsonBody, WebClientOptions *opts);
WebClientResponse webClientDelete(const char *url, WebClientOptions *opts);
WebClientResponse webClientPatch(const char *url, const char *jsonBody, WebClientOptions *opts);

void webClientResponseFree(WebClientResponse *res);

HashMap webClientParseResponseJson(WebClientResponse *res);
String webClientGetResponseHeader(WebClientResponse *res, const char *key);

typedef struct {
    const char *key;
    const char *value;
} WebJsonField;

String webJsonBuildObject(WebJsonField *fields, size_t count);
String webJsonBuildArray(String *items, size_t count);
String webJsonGetString(HashMap json, const char *key);
int    webJsonGetInt(HashMap json, const char *key);
double webJsonGetDouble(HashMap json, const char *key);
bool   webJsonGetBool(HashMap json, const char *key);

#endif