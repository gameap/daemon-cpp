#include <sstream>
#include <iostream>

#include "db.h"

int Db::load(const char *driver_name)
{
    const char *error;
    dbHandle = DLOPEN("/home/nikita/Git/GameAP_Daemon2/db/mysql.so");

    if (!dbHandle) {
        fprintf (stderr, "%s\n", DLERROR());
        return -1;
    }
    
	db_connect              = (db_connect_sign*)DLSYM(dbHandle, "connect");
	db_real_escape_string   = (db_real_escape_string_sign*)DLSYM(dbHandle, "real_escape_string");
	db_query                = (db_query_sign*)DLSYM(dbHandle, "query");
	db_close                = (db_close_sign*)DLSYM(dbHandle, "conn_close");

    if ((error = DLERROR()) != NULL)  {
        fprintf (stderr, "%s\n", error);
        return -1;
    }

    return 0;
}

int Db::connect(const char *host, const char *user, const char *passwd, const char *db, unsigned int port)
{
    return db_connect(host, user, passwd, db, port);
}

std::string Db::real_escape_string(std::string string)
{
    return db_real_escape_string(string);
}

int Db::query(std::string query, db_elems &results)
{
    return db_query(query, results);
}

int Db::close()
{
    return db_close();
}
