#ifndef CWEB_AUTH_H
#define CWEB_AUTH_H

#include "cweb.h"

#define CWEB_JWT_MAX_SIZE 512

int cweb_jwt_create(const char *secret, int user_id, int expires_in_seconds,
    char *token, size_t token_size);
int cweb_jwt_verify(const char *secret, const char *token, int *user_id);
int cweb_jwt_require(CwebRequest *request, CwebResponse *response, const char *secret);

#endif
