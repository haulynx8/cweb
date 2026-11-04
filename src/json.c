#include "cweb.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef enum { J_STRING, J_NUMBER, J_OBJECT, J_ARRAY, J_OTHER } JsonType;
typedef struct { const char *start; const char *end; JsonType type; } JsonValue;

static const char *spaces(const char *p) {
    while (isspace((unsigned char)*p)) p++;
    return p;
}

static int hex(char c) {
    return isdigit((unsigned char)c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static int json_string(const char *p, const char **end) {
    if (*p++ != '"') return 0;
    while (*p) {
        if (*p == '"') { *end = p + 1; return 1; }
        if ((unsigned char)*p < 0x20) return 0;
        if (*p++ == '\\') {
            if (strchr("\"\\/bfnrt", *p)) p++;
            else if (*p == 'u' && hex(p[1]) && hex(p[2]) && hex(p[3]) && hex(p[4])) p += 5;
            else return 0;
        }
    }
    return 0;
}

static int value(const char *p, JsonValue *out, int depth) {
    const char *cur;
    if (depth > 32) return 0;
    p = spaces(p); out->start = p;
    if (*p == '"') { out->type = J_STRING; return json_string(p, &out->end); }
    if (*p == '{') {
        out->type = J_OBJECT; cur = spaces(p + 1);
        if (*cur == '}') { out->end = cur + 1; return 1; }
        while (1) {
            const char *key_end; JsonValue member;
            if (!json_string(cur, &key_end) || *spaces(key_end) != ':') return 0;
            if (!value(spaces(key_end) + 1, &member, depth + 1)) return 0;
            cur = spaces(member.end);
            if (*cur == '}') { out->end = cur + 1; return 1; }
            if (*cur++ != ',') return 0;
            cur = spaces(cur);
        }
    }
    if (*p == '[') {
        out->type = J_ARRAY; cur = spaces(p + 1);
        if (*cur == ']') { out->end = cur + 1; return 1; }
        while (1) {
            JsonValue item;
            if (!value(cur, &item, depth + 1)) return 0;
            cur = spaces(item.end);
            if (*cur == ']') { out->end = cur + 1; return 1; }
            if (*cur++ != ',') return 0;
            cur = spaces(cur);
        }
    }
    if (strncmp(p, "true", 4) == 0) { out->type = J_OTHER; out->end = p + 4; return 1; }
    if (strncmp(p, "false", 5) == 0) { out->type = J_OTHER; out->end = p + 5; return 1; }
    if (strncmp(p, "null", 4) == 0) { out->type = J_OTHER; out->end = p + 4; return 1; }
    if (*p == '-' || isdigit((unsigned char)*p)) {
        char *number_end;
        errno = 0; strtod(p, &number_end);
        if (number_end == p || errno == ERANGE) return 0;
        out->type = J_NUMBER; out->end = number_end; return 1;
    }
    return 0;
}

static int hex_value(const char *p) {
    int answer = 0, i;
    for (i = 0; i < 4; i++) {
        answer *= 16;
        answer += isdigit((unsigned char)p[i]) ? p[i] - '0' :
            (p[i] >= 'a' && p[i] <= 'f' ? p[i] - 'a' + 10 : p[i] - 'A' + 10);
    }
    return answer;
}

static int put_utf8(char *out, size_t size, size_t *used, int code) {
    int count = code < 0x80 ? 1 : (code < 0x800 ? 2 : (code < 0x10000 ? 3 : 4));
    if (*used + (size_t)count >= size) return 0;
    if (count == 1) out[(*used)++] = (char)code;
    else if (count == 2) {
        out[(*used)++] = (char)(0xc0 | (code >> 6));
        out[(*used)++] = (char)(0x80 | (code & 0x3f));
    } else if (count == 3) {
        out[(*used)++] = (char)(0xe0 | (code >> 12));
        out[(*used)++] = (char)(0x80 | ((code >> 6) & 0x3f));
        out[(*used)++] = (char)(0x80 | (code & 0x3f));
    } else {
        out[(*used)++] = (char)(0xf0 | (code >> 18));
        out[(*used)++] = (char)(0x80 | ((code >> 12) & 0x3f));
        out[(*used)++] = (char)(0x80 | ((code >> 6) & 0x3f));
        out[(*used)++] = (char)(0x80 | (code & 0x3f));
    }
    return 1;
}

static int decode(const JsonValue *string, char *out, size_t size) {
    const char *p = string->start + 1, *last = string->end - 1;
    size_t used = 0;
    if (size == 0) return 0;
    while (p < last) {
        char c = *p++;
        if (c != '\\') {
            if (used + 1 >= size) return 0;
            out[used++] = c;
        } else {
            c = *p++;
            if (c == 'u') {
                int code = hex_value(p);
                p += 4;
                if (code >= 0xd800 && code <= 0xdbff) {
                    int low;
                    if (p[0] != '\\' || p[1] != 'u') return 0;
                    low = hex_value(p + 2);
                    if (low < 0xdc00 || low > 0xdfff) return 0;
                    code = 0x10000 + ((code - 0xd800) << 10) + low - 0xdc00;
                    p += 6;
                } else if (code >= 0xdc00 && code <= 0xdfff) {
                    return 0;
                }
                if (!put_utf8(out, size, &used, code)) return 0;
            }
            else {
                if (used + 1 >= size) return 0;
                out[used++] = c == 'b' ? '\b' : c == 'f' ? '\f' : c == 'n' ? '\n' :
                    c == 'r' ? '\r' : c == 't' ? '\t' : c;
            }
        }
    }
    out[used] = '\0'; return 1;
}

static int member(const JsonValue *object, const char *name, JsonValue *out) {
    const char *p = spaces(object->start + 1);
    while (*p != '}') {
        JsonValue key;
        char key_name[128];
        key.start = p;
        if (!json_string(p, &key.end) || !decode(&key, key_name, sizeof(key_name))) return 0;
        p = spaces(key.end);
        if (*p != ':' || !value(p + 1, out, 0)) return 0;
        if (strcmp(key_name, name) == 0) return 1;
        p = spaces(out->end);
        if (*p++ != ',') return 0;
        p = spaces(p);
    }
    return 0;
}

static int item(const JsonValue *array, int wanted, JsonValue *out) {
    const char *p = spaces(array->start + 1);
    int index = 0;
    while (*p != ']') {
        if (!value(p, out, 0)) return 0;
        if (index++ == wanted) return 1;
        p = spaces(out->end);
        if (*p++ != ',') return 0;
        p = spaces(p);
    }
    return 0;
}

static int find(const char *json, const char *path, JsonValue *out) {
    JsonValue current;
    if (!value(json, &current, 0) || *spaces(current.end) != '\0') return 0;
    while (*path) {
        if (current.type == J_OBJECT) {
            char key[128]; size_t used = 0;
            if (*path == '.') path++;
            while (*path && *path != '.' && *path != '[') {
                if (used + 1 >= sizeof(key)) return 0;
                key[used++] = *path++;
            }
            if (used == 0) return 0;
            key[used] = '\0';
            if (!member(&current, key, &current)) return 0;
        } else if (current.type == J_ARRAY) {
            char *end; long index;
            if (*path != '[') return 0;
            errno = 0; index = strtol(path + 1, &end, 10);
            if (errno || index < 0 || index > INT_MAX || *end != ']') return 0;
            path = end + 1;
            if (!item(&current, (int)index, &current)) return 0;
        } else return 0;
    }
    *out = current; return 1;
}

int cweb_json_get_string(const char *json, const char *field, char *value_out, size_t value_size) {
    JsonValue result;
    return find(json, field, &result) && result.type == J_STRING && decode(&result, value_out, value_size);
}

int cweb_json_get_int(const char *json, const char *field, int *value_out) {
    JsonValue result; char number[64], *end; long parsed; size_t length;
    if (!find(json, field, &result) || result.type != J_NUMBER) return 0;
    length = (size_t)(result.end - result.start);
    if (length >= sizeof(number)) return 0;
    memcpy(number, result.start, length); number[length] = '\0';
    errno = 0; parsed = strtol(number, &end, 10);
    if (errno || *end || parsed < INT_MIN || parsed > INT_MAX) return 0;
    *value_out = (int)parsed; return 1;
}
