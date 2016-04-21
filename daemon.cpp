#ifdef WIN32
#define _WIN32_WINNT 0x0501
#include <stdio.h>
#endif

#include <sstream>
#include <iostream>
#include <map>

#include "db.h"
#include "config.h"

int main(int argc, char* argv[])
{
    std::cout << "Start" << std::endl;

    Config config;

    if (config.parse() == -1) {
        
    }

    Db db;
    db.load("mysql");

    if (db.connect("localhost", "root", "1231943", "gameap_empire", 3306) == -1) {
        fprintf(stdout, "Error connect\n");
    }

    db_elems results;

    std::string query;

    // query = std::string("SELECT * FROM `gameap_users` WHERE `name` = '")
        // + db.real_escape_string("nikita")
        // + std::string("'");

    if (db.query("SELECT * FROM `gameap_users`", results) == -1) {
        fprintf(stdout, "Error connect\n");
    }
    
    // fprintf(stdout, "results.num_fields: %i\n", results.num_fields);

    for (int i = 0; i < results.num_rows; i++) {
        // for (int j = 0; j < results.num_fields; j++) {
        for (auto it = results.rows[i].begin(); it != results.rows[i].end(); ++it) {
            // fprintf(stdout, "Field (%i): %s\n", i, results.rows[i][it].c_str());
            // fprintf(stdout, "Field (%i): %s\n", i, it->first);
            std::cout << it->first << " -:- "<< it->second << std::endl;
        }
    }
}
