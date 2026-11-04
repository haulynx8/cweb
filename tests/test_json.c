#include "cweb.h"

#include <assert.h>
#include <string.h>

int main(void) {
    const char *json =
        "{\"user\":{\"profile\":{\"name\":\"A\\\"mina\",\"city\":\"Nai\\u0072obi\"}},"
        "\"tags\":[\"student\",{\"name\":\"c programmer\"}],\"emoji\":\"\\uD83D\\uDE80\",\"age\":21}";
    char text[64];
    int age;

    assert(cweb_json_get_string(json, "user.profile.name", text, sizeof(text)));
    assert(strcmp(text, "A\"mina") == 0);
    assert(cweb_json_get_string(json, "user.profile.city", text, sizeof(text)));
    assert(strcmp(text, "Nairobi") == 0);
    assert(cweb_json_get_string(json, "tags[0]", text, sizeof(text)));
    assert(strcmp(text, "student") == 0);
    assert(cweb_json_get_string(json, "tags[1].name", text, sizeof(text)));
    assert(strcmp(text, "c programmer") == 0);
    assert(cweb_json_get_string(json, "emoji", text, sizeof(text)));
    assert(strcmp(text, "🚀") == 0);
    assert(cweb_json_get_int(json, "age", &age));
    assert(age == 21);
    assert(!cweb_json_get_string("{\"name\":}", "name", text, sizeof(text)));

    return 0;
}
