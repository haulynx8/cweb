#include "cweb.h"

#include <stdio.h>

void cweb_use(CwebApp *app, CwebMiddleware middleware) {
    if (app->middleware_count >= CWEB_MAX_MIDDLEWARE) {
        printf("Too much middleware. Increase CWEB_MAX_MIDDLEWARE if needed.\n");
        return;
    }

    app->middleware[app->middleware_count] = middleware;
    app->middleware_count++;
}
