#ifndef CWEB_DB_H
#define CWEB_DB_H

#include <sqlite3.h>

#define CWEB_MAX_USERS 100

typedef struct {
    int id;
    char name[64];
} CwebUser;

typedef struct {
    CwebUser users[CWEB_MAX_USERS];
    int count;
} CwebUserList;

typedef struct {
    sqlite3 *connection;
} CwebDatabase;

int cweb_db_open(CwebDatabase *database, const char *file_name);
void cweb_db_close(CwebDatabase *database);
int cweb_db_setup_users(CwebDatabase *database);
int cweb_user_create(CwebDatabase *database, const char *name, int *new_id);
int cweb_users_get_all(CwebDatabase *database, CwebUserList *users);
int cweb_user_get(CwebDatabase *database, int id, CwebUser *user);

#endif
