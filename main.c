#include "include/web.h"

void getUsers(WebRequest *req, WebResponse *res) {
    webResponseJson(res, 200, "{\"users\":[{\"id\":1,\"name\":\"Nicolo\"}]}");
}

void getUser(WebRequest *req, WebResponse *res) {
    String id = webRequestGetParam(req, "id");
    webResponseJson(res, 200, "{\"id\":1}");
}

void createUser(WebRequest *req, WebResponse *res) {
    HashMap body = webParseJson(stringGetData(req->body));
    mapFree(body, NULL, NULL);
    webResponseJson(res, 201, "{\"created\":true}");
}

int main(void) {
    WebServer srv = webServerCreate("0.0.0.0", 8080);
    webServerSetCORS(srv, true);
    webServerGet(srv, "/users", getUsers);
    webServerGet(srv, "/users/:id", getUser);
    webServerPost(srv, "/users", createUser);
    webServerRun(srv);  
    webServerFree(srv);
    return 0;
}