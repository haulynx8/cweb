CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -Iinclude
LDLIBS = -lsqlite3 -lcrypto

all: users_api

users_api: src/cweb.c src/router.c src/response.c src/middleware.c src/json.c src/validation.c src/database.c src/auth.c examples/users_api.c include/cweb.h include/cweb_db.h include/cweb_auth.h
	$(CC) $(CFLAGS) src/cweb.c src/router.c src/response.c src/middleware.c src/json.c src/validation.c src/database.c src/auth.c examples/users_api.c -o users_api $(LDLIBS)

test_json: src/json.c tests/test_json.c include/cweb.h
	$(CC) $(CFLAGS) src/json.c tests/test_json.c -o test_json

test_database: src/database.c tests/test_database.c include/cweb_db.h
	$(CC) $(CFLAGS) src/database.c tests/test_database.c -o test_database $(LDLIBS)

test_auth: src/auth.c src/router.c src/response.c tests/test_auth.c include/cweb_auth.h
	$(CC) $(CFLAGS) src/auth.c src/router.c src/response.c tests/test_auth.c -o test_auth -lcrypto

test: test_json test_database test_auth
	./test_json
	./test_database
	./test_auth

clean:
	rm -f users_api test_json test_database test_auth cweb.db
