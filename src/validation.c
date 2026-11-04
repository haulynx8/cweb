#include "cweb.h"

#include <stdio.h>

static void missing_field_response(CwebResponse *response, const char *field, const char *type) {
    char message[256];

    snprintf(message, sizeof(message),
        "{\"error\":\"%s is required and must be a %s\"}", field, type);
    cweb_json(response, 400, message);
}

int cweb_require_json_string(CwebRequest *request, CwebResponse *response,
    const char *field, char *value, size_t value_size) {
    if (!cweb_json_get_string(request->body, field, value, value_size) || value[0] == '\0') {
        missing_field_response(response, field, "string");
        return 0;
    }
    return 1;
}

int cweb_require_json_int(CwebRequest *request, CwebResponse *response,
    const char *field, int *value) {
    if (!cweb_json_get_int(request->body, field, value)) {
        missing_field_response(response, field, "number");
        return 0;
    }
    return 1;
}
