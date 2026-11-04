# cweb

A tiny learning project for making REST APIs in C feel less repetitive.

It currently has routing, JSON/text responses, and basic request-body reading.
It deliberately stays small: the server handles one request at a time and only
supports simple HTTP requests. It is not production-ready yet.

## Run the example

```bash
make
./users_api
```

In another terminal:

```bash
curl http://localhost:8080/users
curl http://localhost:8080/users/42
curl -X POST http://localhost:8080/users -d '{"name":"Sam"}'
```

## Writing routes

```c
static int users_get_all(CwebRequest *request, CwebResponse *response) {
    (void)request;
    cweb_json(response, 200, "[{\"id\":1,\"name\":\"Amina\"}]");
    return 0;
}

CWEB_APP(app);
GET(app, "/users", users_get_all);
cweb_run(&app, 8080);
```

## Route parameters

Put a colon before a path section to save it as a parameter:

```c
static int users_get_one(CwebRequest *request, CwebResponse *response) {
    const char *id = cweb_param(request, "id");
    char json[128];

    snprintf(json, sizeof(json), "{\"id\":%s}", id);
    cweb_json(response, 200, json);
    return 0;
}

GET(app, "/users/:id", users_get_one);
```

## Middleware

Middleware runs before every route. Return `0` to continue to the route, or a
non-zero value after setting a response to stop the request early.

```c
static int log_requests(CwebRequest *request, CwebResponse *response) {
    (void)response;
    printf("%s %s\n", request->method, request->path);
    return 0;
}

CWEB_APP(app);
cweb_use(&app, log_requests);
```

## JSON validation

The framework can read required string and number fields from JSON. The
validation helper sends a `400` response automatically when a field is missing,
empty, or the wrong type.

```c
static int users_create(CwebRequest *request, CwebResponse *response) {
    char name[64];

    if (!cweb_require_json_string(request, response, "name", name, sizeof(name))) {
        return 1;
    }

    cweb_json(response, 201, "{\"message\":\"User created\"}");
    return 0;
}
```

It supports nested objects, arrays, escaped quotes, and Unicode escape codes.
Use dot paths for objects and `[number]` for arrays:

```c
cweb_json_get_string(request->body, "user.profile.name", name, sizeof(name));
cweb_json_get_string(request->body, "tags[0]", first_tag, sizeof(first_tag));
```

It remains a small in-project parser rather than a full standalone JSON
library, so production projects should still consider a dedicated library.

## SQLite users

The example API stores users in `cweb.db`, using the small database module in
`src/database.c`. It creates the `users` table when the app starts, and uses
prepared statements for the insert and lookup queries.

```bash
curl -X POST http://localhost:8080/users -d '{"name":"Amina"}'
curl http://localhost:8080/users
curl http://localhost:8080/users/1
```

This is one SQLite connection for the example, not a connection pool yet.

## JWT authentication

JWT support uses HS256 signatures through OpenSSL. Set a long random secret
before starting the example:

```bash
CWEB_JWT_SECRET='replace-this-with-a-long-random-secret' ./users_api
```

For learning the request flow, `POST /auth/token` accepts an existing user's
ID and returns a one-hour token. It does **not** verify a password, so it is
not a real login endpoint. Use the returned token with the protected `/me`
route:

```bash
curl -X POST http://localhost:8080/auth/token -d '{"user_id":1}'
curl http://localhost:8080/me -H 'Authorization: Bearer YOUR_TOKEN'
```

C cannot safely create a normal callback function from a code block such as
`GET(app, "/users", { ... })`. So the first version uses named handler
functions. It keeps the framework simple and works with normal C compilers.

## Next features

- JWT authentication
- Database connections
