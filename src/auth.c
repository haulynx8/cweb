#include "cweb_auth.h"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static const char *jwt_header = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

static int base64url_encode(const unsigned char *input, size_t length, char *output, size_t output_size) {
    size_t input_pos = 0;
    size_t output_pos = 0;

    while (input_pos < length) {
        size_t remaining = length - input_pos;
        unsigned int group = (unsigned int)input[input_pos++] << 16;

        if (remaining > 1) group |= (unsigned int)input[input_pos++] << 8;
        if (remaining > 2) group |= input[input_pos++];
        if (output_pos + 4 >= output_size) return 0;

        output[output_pos++] = alphabet[(group >> 18) & 63];
        output[output_pos++] = alphabet[(group >> 12) & 63];
        if (remaining > 1) output[output_pos++] = alphabet[(group >> 6) & 63];
        if (remaining > 2) output[output_pos++] = alphabet[group & 63];
    }

    output[output_pos] = '\0';
    return 1;
}

static int base64url_value(char character) {
    const char *found = strchr(alphabet, character);
    return found == NULL ? -1 : (int)(found - alphabet);
}

static int base64url_decode(const char *input, unsigned char *output, size_t output_size, size_t *length) {
    unsigned int group = 0;
    int bits = 0;
    size_t used = 0;

    while (*input != '\0') {
        int value = base64url_value(*input++);
        if (value < 0) return 0;
        group = (group << 6) | (unsigned int)value;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            if (used >= output_size) return 0;
            output[used++] = (unsigned char)(group >> bits);
        }
    }

    if (bits >= 6) return 0;
    *length = used;
    return 1;
}

static int sign(const char *secret, const char *message, unsigned char signature[EVP_MAX_MD_SIZE], unsigned int *length) {
    return HMAC(EVP_sha256(), secret, (int)strlen(secret), (const unsigned char *)message,
        strlen(message), signature, length) != NULL;
}

int cweb_jwt_create(const char *secret, int user_id, int expires_in_seconds,
    char *token, size_t token_size) {
    char payload[128];
    char encoded_payload[256];
    char signing_input[384];
    char encoded_signature[128];
    unsigned char signature[EVP_MAX_MD_SIZE];
    unsigned int signature_length;
    long long expiry = (long long)time(NULL) + expires_in_seconds;

    if (user_id < 1 || expires_in_seconds < 1) return 0;
    snprintf(payload, sizeof(payload), "{\"sub\":%d,\"exp\":%lld}", user_id, expiry);
    if (!base64url_encode((const unsigned char *)payload, strlen(payload), encoded_payload, sizeof(encoded_payload))) return 0;
    snprintf(signing_input, sizeof(signing_input), "%s.%s", jwt_header, encoded_payload);
    if (!sign(secret, signing_input, signature, &signature_length)) return 0;
    if (!base64url_encode(signature, signature_length, encoded_signature, sizeof(encoded_signature))) return 0;
    if (snprintf(token, token_size, "%s.%s", signing_input, encoded_signature) >= (int)token_size) return 0;
    return 1;
}

int cweb_jwt_verify(const char *secret, const char *token, int *user_id) {
    char copy[CWEB_JWT_MAX_SIZE];
    char *payload_part;
    char *signature_part;
    char payload[256];
    char signing_input[CWEB_JWT_MAX_SIZE];
    unsigned char expected_signature[EVP_MAX_MD_SIZE];
    char expected_encoded_signature[128];
    size_t payload_length;
    size_t header_length;
    size_t encoded_payload_length;
    unsigned int expected_length;
    int id;
    long long expiry;
    int used;

    if (strlen(token) >= sizeof(copy)) return 0;
    snprintf(copy, sizeof(copy), "%s", token);
    payload_part = strchr(copy, '.');
    if (payload_part == NULL) return 0;
    *payload_part++ = '\0';
    signature_part = strchr(payload_part, '.');
    if (signature_part == NULL || strchr(signature_part + 1, '.') != NULL) return 0;
    *signature_part++ = '\0';
    if (strcmp(copy, jwt_header) != 0) return 0;
    if (!base64url_decode(payload_part, (unsigned char *)payload, sizeof(payload) - 1, &payload_length)) return 0;
    payload[payload_length] = '\0';
    if (sscanf(payload, "{\"sub\":%d,\"exp\":%lld}%n", &id, &expiry, &used) != 2 ||
        payload[used] != '\0' || id < 1 || expiry <= (long long)time(NULL)) return 0;
    if (!base64url_decode(signature_part, expected_signature, sizeof(expected_signature), &payload_length)) return 0;
    header_length = strlen(copy);
    encoded_payload_length = strlen(payload_part);
    if (header_length + 1 + encoded_payload_length >= sizeof(signing_input)) return 0;
    memcpy(signing_input, copy, header_length);
    signing_input[header_length] = '.';
    memcpy(signing_input + header_length + 1, payload_part, encoded_payload_length + 1);
    if (!sign(secret, signing_input, expected_signature, &expected_length)) return 0;
    if (!base64url_encode(expected_signature, expected_length, expected_encoded_signature,
        sizeof(expected_encoded_signature))) return 0;
    if (strlen(signature_part) != strlen(expected_encoded_signature) ||
        CRYPTO_memcmp(signature_part, expected_encoded_signature, strlen(signature_part)) != 0) return 0;

    *user_id = id;
    return 1;
}

int cweb_jwt_require(CwebRequest *request, CwebResponse *response, const char *secret) {
    const char *authorization = cweb_header(request, "Authorization");
    int user_id;

    if (authorization == NULL || strncmp(authorization, "Bearer ", 7) != 0 ||
        !cweb_jwt_verify(secret, authorization + 7, &user_id)) {
        cweb_json(response, 401, "{\"error\":\"A valid Bearer token is required\"}");
        return 1;
    }
    request->user_id = user_id;
    return 0;
}
