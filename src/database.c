#include "cweb_db.h"

#include <stdio.h>
#include <string.h>

int cweb_db_open(CwebDatabase *database, const char *file_name) {
    database->connection = NULL;
    if (sqlite3_open(file_name, &database->connection) != SQLITE_OK) {
        fprintf(stderr, "Could not open database: %s\n", sqlite3_errmsg(database->connection));
        cweb_db_close(database);
        return 0;
    }
    return 1;
}

void cweb_db_close(CwebDatabase *database) {
    if (database->connection != NULL) sqlite3_close(database->connection);
    database->connection = NULL;
}

int cweb_db_setup_users(CwebDatabase *database) {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL"
        ");";
    char *error = NULL;
    int result = sqlite3_exec(database->connection, sql, NULL, NULL, &error);

    if (result != SQLITE_OK) {
        fprintf(stderr, "Could not create users table: %s\n", error);
        sqlite3_free(error);
        return 0;
    }
    return 1;
}

int cweb_user_create(CwebDatabase *database, const char *name, int *new_id) {
    sqlite3_stmt *statement;
    const char *sql = "INSERT INTO users (name) VALUES (?);";

    if (sqlite3_prepare_v2(database->connection, sql, -1, &statement, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_text(statement, 1, name, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(statement) != SQLITE_DONE) {
        sqlite3_finalize(statement);
        return 0;
    }

    *new_id = (int)sqlite3_last_insert_rowid(database->connection);
    sqlite3_finalize(statement);
    return 1;
}

int cweb_users_get_all(CwebDatabase *database, CwebUserList *users) {
    sqlite3_stmt *statement;
    const char *sql = "SELECT id, name FROM users ORDER BY id;";
    int status;

    users->count = 0;
    if (sqlite3_prepare_v2(database->connection, sql, -1, &statement, NULL) != SQLITE_OK) return 0;

    while ((status = sqlite3_step(statement)) == SQLITE_ROW && users->count < CWEB_MAX_USERS) {
        CwebUser *user = &users->users[users->count];
        user->id = sqlite3_column_int(statement, 0);
        snprintf(user->name, sizeof(user->name), "%s", sqlite3_column_text(statement, 1));
        users->count++;
    }

    sqlite3_finalize(statement);
    return status == SQLITE_DONE || users->count == CWEB_MAX_USERS;
}

int cweb_user_get(CwebDatabase *database, int id, CwebUser *user) {
    sqlite3_stmt *statement;
    const char *sql = "SELECT id, name FROM users WHERE id = ?;";
    int found = 0;

    if (sqlite3_prepare_v2(database->connection, sql, -1, &statement, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_int(statement, 1, id);
    if (sqlite3_step(statement) == SQLITE_ROW) {
        user->id = sqlite3_column_int(statement, 0);
        snprintf(user->name, sizeof(user->name), "%s", sqlite3_column_text(statement, 1));
        found = 1;
    }
    sqlite3_finalize(statement);
    return found;
}
