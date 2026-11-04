#include "cweb.h"

#include <stdio.h>

void cweb_json(CwebResponse *response, int status, const char *json) {
    response->status = status;
    response->content_type = "application/json";
    snprintf(response->body, sizeof(response->body), "%s", json);
}

void cweb_text(CwebResponse *response, int status, const char *text) {
    response->status = status;
    response->content_type = "text/plain";
    snprintf(response->body, sizeof(response->body), "%s", text);
}

int cweb_json_escape(const char *text, char *escaped, size_t escaped_size) {
    size_t used = 0;

    while (*text != '\0') {
        const char *replacement = NULL;

        if (*text == '"') replacement = "\\\"";
        else if (*text == '\\') replacement = "\\\\";
        else if (*text == '\n') replacement = "\\n";
        else if (*text == '\r') replacement = "\\r";
        else if (*text == '\t') replacement = "\\t";

        if (replacement != NULL) {
            size_t length = 2;
            if (used + length >= escaped_size) return 0;
            snprintf(escaped + used, escaped_size - used, "%s", replacement);
            used += length;
        } else {
            if (used + 1 >= escaped_size) return 0;
            escaped[used++] = *text;
        }
        text++;
    }

    if (escaped_size == 0) return 0;
    escaped[used] = '\0';
    return 1;
}
