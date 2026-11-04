#define _POSIX_C_SOURCE 200809L
#include "cweb_internal.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static const char *status_message(int status) {
    if (status == 200) return "OK";
    if (status == 201) return "Created";
    if (status == 400) return "Bad Request";
    if (status == 401) return "Unauthorized";
    if (status == 404) return "Not Found";
    return "Internal Server Error";
}

static void parse_request(char *raw, CwebRequest *request) {
    char *body_start;
    char *line;

    memset(request, 0, sizeof(*request));
    sscanf(raw, "%7s %255s", request->method, request->path);

    /* Ignore query strings for now. Query parsing will be a later feature. */
    char *query_start = strchr(request->path, '?');
    if (query_start != NULL) *query_start = '\0';

    body_start = strstr(raw, "\r\n\r\n");
    if (body_start != NULL) {
        snprintf(request->body, sizeof(request->body), "%s", body_start + 4);
    }

    line = strstr(raw, "\r\n");
    if (line == NULL) return;
    line += 2;
    while (strncmp(line, "\r\n", 2) != 0 && *line != '\0') {
        char *line_end = strstr(line, "\r\n");
        char *colon;

        if (line_end == NULL || request->header_count >= CWEB_MAX_REQUEST_HEADERS) break;
        *line_end = '\0';
        colon = strchr(line, ':');
        if (colon != NULL) {
            char *header_value = colon + 1;
            *colon = '\0';
            while (*header_value == ' ' || *header_value == '\t') header_value++;
            snprintf(request->header_names[request->header_count],
                sizeof(request->header_names[request->header_count]), "%s", line);
            snprintf(request->header_values[request->header_count],
                sizeof(request->header_values[request->header_count]), "%s", header_value);
            request->header_count++;
        }
        line = line_end + 2;
    }
}

static void send_response(int client, CwebResponse *response) {
    char header[CWEB_MAX_HEADERS];
    int length;

    length = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n\r\n",
        response->status, status_message(response->status), response->content_type,
        strlen(response->body));

    send(client, header, (size_t)length, 0);
    send(client, response->body, strlen(response->body), 0);
}

void cweb_run(CwebApp *app, int port) {
    int server;
    struct sockaddr_in address;

    server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) {
        perror("socket");
        return;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons((unsigned short)port);

    if (bind(server, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        close(server);
        return;
    }

    if (listen(server, 10) < 0) {
        perror("listen");
        close(server);
        return;
    }

    printf("cweb is running at http://localhost:%d\n", port);

    while (1) {
        int client = accept(server, NULL, NULL);
        char raw[CWEB_MAX_BODY + CWEB_MAX_HEADERS] = {0};
        CwebRequest request;
        CwebResponse response = {500, "text/plain", "Server error"};
        CwebRoute *route;

        if (client < 0) continue;
        recv(client, raw, sizeof(raw) - 1, 0);
        parse_request(raw, &request);
        int middleware_stopped = 0;
        int i;

        for (i = 0; i < app->middleware_count; i++) {
            if (app->middleware[i](&request, &response) != 0) {
                middleware_stopped = 1;
                break;
            }
        }

        route = cweb_find_route(app, &request);

        if (middleware_stopped) {
            /* Middleware already filled in the response. */
        } else if (route == NULL) {
            cweb_json(&response, 404, "{\"error\":\"Route not found\"}");
        } else {
            route->handler(&request, &response);
        }

        send_response(client, &response);
        close(client);
    }
}
