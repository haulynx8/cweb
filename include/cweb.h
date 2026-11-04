#ifndef CWEB_H
#define CWEB_H

#include <stddef.h>

#define CWEB_MAX_ROUTES 64
#define CWEB_MAX_BODY 8192
#define CWEB_MAX_HEADERS 4096
#define CWEB_MAX_PARAMS 8
#define CWEB_MAX_MIDDLEWARE 16
#define CWEB_MAX_REQUEST_HEADERS 32

typedef struct {
    char method[8];
    char path[256];
    char body[CWEB_MAX_BODY];
    char param_names[CWEB_MAX_PARAMS][32];
    char param_values[CWEB_MAX_PARAMS][128];
    int param_count;
    char header_names[CWEB_MAX_REQUEST_HEADERS][64];
    char header_values[CWEB_MAX_REQUEST_HEADERS][512];
    int header_count;
    int user_id;
} CwebRequest;

typedef struct {
    int status;
    const char *content_type;
    char body[CWEB_MAX_BODY];
} CwebResponse;

typedef int (*CwebHandler)(CwebRequest *request, CwebResponse *response);
typedef int (*CwebMiddleware)(CwebRequest *request, CwebResponse *response);

typedef struct {
    const char *method;
    const char *path;
    CwebHandler handler;
} CwebRoute;

typedef struct {
    CwebRoute routes[CWEB_MAX_ROUTES];
    int route_count;
    CwebMiddleware middleware[CWEB_MAX_MIDDLEWARE];
    int middleware_count;
} CwebApp;

#define CWEB_APP(name) CwebApp name = {0}
#define GET(app, path, handler) cweb_add_route(&(app), "GET", (path), (handler))
#define POST(app, path, handler) cweb_add_route(&(app), "POST", (path), (handler))

void cweb_add_route(CwebApp *app, const char *method, const char *path, CwebHandler handler);
const char *cweb_param(CwebRequest *request, const char *name);
const char *cweb_header(CwebRequest *request, const char *name);
void cweb_use(CwebApp *app, CwebMiddleware middleware);
void cweb_json(CwebResponse *response, int status, const char *json);
void cweb_text(CwebResponse *response, int status, const char *text);
int cweb_json_escape(const char *text, char *escaped, size_t escaped_size);
int cweb_json_get_string(const char *json, const char *field, char *value, size_t value_size);
int cweb_json_get_int(const char *json, const char *field, int *value);
int cweb_require_json_string(CwebRequest *request, CwebResponse *response,
    const char *field, char *value, size_t value_size);
int cweb_require_json_int(CwebRequest *request, CwebResponse *response,
    const char *field, int *value);
void cweb_run(CwebApp *app, int port);

#endif
