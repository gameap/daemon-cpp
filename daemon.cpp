#ifdef WIN32
#define _WIN32_WINNT 0x0501
#include <stdio.h>
#endif

#include <sstream>
#include <iostream>
#include <map>

#include "dl.h"

#include "db/db.h"
#include "config.h"

int main(int argc, char* argv[])
{
    std::cout << "Start" << std::endl;

    Config config;

    if (config.parse() == -1) {
        return -1;
    }

    Db* db;
    load_db(db);

    if (db->connect(config.db_host.c_str(), config.db_user.c_str(), config.db_passwd.c_str(), config.db_name.c_str(), config.db_port) == -1) {
        fprintf(stdout, "Error connect\n");
        return -1;
    }

    db_elems results;

    if (db->query("SELECT * FROM `gameap_users`", results) == -1) {
        fprintf(stdout, "Error query\n");
        return -1;
    }

    std::cout << "NUM ROWS:" << results.num_rows << std::endl;
    
    for (int i = 0; i < results.num_rows; i++) {
        for (auto it = results.rows[i].begin(); it != results.rows[i].end(); ++it) {
            std::cout << it->first << " -:- "<< it->second << std::endl;
        }

        // auto it = results.rows[i].find("password");
        // std::cout << results.rows[i]["password"] << std::endl;
    }

    return 0;
}
