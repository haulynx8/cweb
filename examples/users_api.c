#include "cweb.h"
#include "cweb_auth.h"
#include "cweb_db.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static CwebDatabase database;
static const char *jwt_secret;

static int log_requests(CwebRequest *request, CwebResponse *response) {
    (void)response;
    printf("%s %s\n", request->method, request->path);
    return 0;
}

static int require_jwt_for_me(CwebRequest *request, CwebResponse *response) {
    if (strcmp(request->path, "/me") == 0) return cweb_jwt_require(request, response, jwt_secret);
    return 0;
}

static int send_user(CwebResponse *response, int status, CwebUser *user) {
    char escaped_name[128];
    char json[256];

    if (!cweb_json_escape(user->name, escaped_name, sizeof(escaped_name))) return 0;
    snprintf(json, sizeof(json), "{\"id\":%d,\"name\":\"%s\"}", user->id, escaped_name);
    cweb_json(response, status, json);
    return 1;
}

static int users_get_all(CwebRequest *request, CwebResponse *response) {
    CwebUserList users;
    char json[CWEB_MAX_BODY] = "[";
    size_t used = 1;
    int i;

    (void)request;
    if (!cweb_users_get_all(&database, &users)) {
        cweb_json(response, 500, "{\"error\":\"Could not read users\"}");
        return 1;
    }

    for (i = 0; i < users.count; i++) {
        char escaped_name[128];
        int written;

        if (!cweb_json_escape(users.users[i].name, escaped_name, sizeof(escaped_name))) break;
        written = snprintf(json + used, sizeof(json) - used, "%s{\"id\":%d,\"name\":\"%s\"}",
            i == 0 ? "" : ",", users.users[i].id, escaped_name);
        if (written < 0 || (size_t)written >= sizeof(json) - used) break;
        used += (size_t)written;
    }

    if (i != users.count) {
        cweb_json(response, 500, "{\"error\":\"Too many users to return\"}");
        return 1;
    }

    snprintf(json + used, sizeof(json) - used, "]");
    cweb_json(response, 200, json);
    return 0;
}

static int users_create(CwebRequest *request, CwebResponse *response) {
    char name[64];
    char json[128];
    int id;

    if (!cweb_require_json_string(request, response, "name", name, sizeof(name))) {
        return 1;
    }

    if (!cweb_user_create(&database, name, &id)) {
        cweb_json(response, 500, "{\"error\":\"Could not create user\"}");
        return 1;
    }

    snprintf(json, sizeof(json), "{\"message\":\"User created\",\"id\":%d}", id);
    cweb_json(response, 201, json);
    return 0;
}

static int users_get_one(CwebRequest *request, CwebResponse *response) {
    const char *id = cweb_param(request, "id");
    char *end;
    long number;
    CwebUser user;

    number = strtol(id, &end, 10);
    if (*end != '\0' || number < 1 || number > 2147483647L) {
        cweb_json(response, 400, "{\"error\":\"User id must be a positive number\"}");
        return 1;
    }
    if (!cweb_user_get(&database, (int)number, &user)) {
        cweb_json(response, 404, "{\"error\":\"User not found\"}");
        return 1;
    }
    if (!send_user(response, 200, &user)) cweb_json(response, 500, "{\"error\":\"Could not create JSON\"}");
    return 0;
}

static int users_get_me(CwebRequest *request, CwebResponse *response) {
    CwebUser user;

    if (!cweb_user_get(&database, request->user_id, &user)) {
        cweb_json(response, 404, "{\"error\":\"User not found\"}");
        return 1;
    }
    if (!send_user(response, 200, &user)) cweb_json(response, 500, "{\"error\":\"Could not create JSON\"}");
    return 0;
}

/* This is for learning the JWT flow. A real login needs passwords or another identity provider. */
static int auth_create_token(CwebRequest *request, CwebResponse *response) {
    int user_id;
    CwebUser user;
    char token[CWEB_JWT_MAX_SIZE];
    char json[CWEB_JWT_MAX_SIZE + 32];

    if (!cweb_require_json_int(request, response, "user_id", &user_id)) return 1;
    if (!cweb_user_get(&database, user_id, &user)) {
        cweb_json(response, 404, "{\"error\":\"User not found\"}");
        return 1;
    }
    if (!cweb_jwt_create(jwt_secret, user_id, 3600, token, sizeof(token))) {
        cweb_json(response, 500, "{\"error\":\"Could not create token\"}");
        return 1;
    }
    snprintf(json, sizeof(json), "{\"token\":\"%s\"}", token);
    cweb_json(response, 200, json);
    return 0;
}

int main(void) {
    CWEB_APP(app);

    jwt_secret = getenv("CWEB_JWT_SECRET");
    if (jwt_secret == NULL || jwt_secret[0] == '\0') {
        fprintf(stderr, "Set CWEB_JWT_SECRET before starting the app.\n");
        return 1;
    }
    if (!cweb_db_open(&database, "cweb.db") || !cweb_db_setup_users(&database)) return 1;

    cweb_use(&app, log_requests);
    cweb_use(&app, require_jwt_for_me);
    GET(app, "/users", users_get_all);
    GET(app, "/users/:id", users_get_one);
    GET(app, "/me", users_get_me);
    POST(app, "/users", users_create);
    POST(app, "/auth/token", auth_create_token);

    cweb_run(&app, 8080);
    return 0;
}
