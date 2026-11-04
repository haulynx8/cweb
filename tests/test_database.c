#include "cweb_db.h"

#include <assert.h>
#include <string.h>

int main(void) {
    CwebDatabase database;
    CwebUserList users;
    CwebUser user;
    int first_id;
    int second_id;

    assert(cweb_db_open(&database, ":memory:"));
    assert(cweb_db_setup_users(&database));
    assert(cweb_user_create(&database, "Amina", &first_id));
    assert(cweb_user_create(&database, "Sam", &second_id));
    assert(first_id == 1);
    assert(second_id == 2);
    assert(cweb_users_get_all(&database, &users));
    assert(users.count == 2);
    assert(strcmp(users.users[0].name, "Amina") == 0);
    assert(cweb_user_get(&database, second_id, &user));
    assert(strcmp(user.name, "Sam") == 0);
    assert(!cweb_user_get(&database, 99, &user));

    cweb_db_close(&database);
    return 0;
}
