#include "cweb_auth.h"

#include <assert.h>
#include <string.h>

int main(void) {
    char token[CWEB_JWT_MAX_SIZE];
    int user_id;

    assert(cweb_jwt_create("test-secret", 42, 3600, token, sizeof(token)));
    assert(cweb_jwt_verify("test-secret", token, &user_id));
    assert(user_id == 42);
    assert(!cweb_jwt_verify("wrong-secret", token, &user_id));
    token[strlen(token) - 1] = token[strlen(token) - 1] == 'a' ? 'b' : 'a';
    assert(!cweb_jwt_verify("test-secret", token, &user_id));
    assert(cweb_jwt_create("test-secret", 42, -1, token, sizeof(token)) == 0);

    return 0;
}
