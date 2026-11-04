#define _POSIX_C_SOURCE 200809L
#include "cweb_internal.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>

void cweb_add_route(CwebApp *app, const char *method, const char *path, CwebHandler handler) {
    if (app->route_count >= CWEB_MAX_ROUTES) {
        printf("Too many routes. Increase CWEB_MAX_ROUTES if needed.\n");
        return;
    }

    app->routes[app->route_count].method = method;
    app->routes[app->route_count].path = path;
    app->routes[app->route_count].handler = handler;
    app->route_count++;
}

static int path_matches(const char *route_path, const char *request_path, CwebRequest *request) {
    char route_copy[256];
    char request_copy[256];
    char *route_part;
    char *request_part;
    char *route_save;
    char *request_save;

    snprintf(route_copy, sizeof(route_copy), "%s", route_path);
    snprintf(request_copy, sizeof(request_copy), "%s", request_path);
    route_part = strtok_r(route_copy, "/", &route_save);
    request_part = strtok_r(request_copy, "/", &request_save);

    while (route_part != NULL && request_part != NULL) {
        if (route_part[0] == ':') {
            if (request->param_count >= CWEB_MAX_PARAMS) return 0;
            snprintf(request->param_names[request->param_count],
                sizeof(request->param_names[request->param_count]), "%s", route_part + 1);
            snprintf(request->param_values[request->param_count],
                sizeof(request->param_values[request->param_count]), "%s", request_part);
            request->param_count++;
        } else if (strcmp(route_part, request_part) != 0) {
            return 0;
        }

        route_part = strtok_r(NULL, "/", &route_save);
        request_part = strtok_r(NULL, "/", &request_save);
    }

    return route_part == NULL && request_part == NULL;
}

CwebRoute *cweb_find_route(CwebApp *app, CwebRequest *request) {
    int i;

    for (i = 0; i < app->route_count; i++) {
        request->param_count = 0;
        if (strcmp(app->routes[i].method, request->method) == 0 &&
            path_matches(app->routes[i].path, request->path, request)) {
            return &app->routes[i];
        }
    }
    return NULL;
}

const char *cweb_param(CwebRequest *request, const char *name) {
    int i;

    for (i = 0; i < request->param_count; i++) {
        if (strcmp(request->param_names[i], name) == 0) return request->param_values[i];
    }
    return NULL;
}

const char *cweb_header(CwebRequest *request, const char *name) {
    int i;

    for (i = 0; i < request->header_count; i++) {
        if (strcasecmp(request->header_names[i], name) == 0) return request->header_values[i];
    }
    return NULL;
}
