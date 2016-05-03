#include <stdio.h>
#include <iostream>
#include <unistd.h>

#include <sstream>
#include <map>

#include <ctime>

#include "daemon_server.h"
#include "dl.h"

#include "db/db.h"
#include "config.h"

#include "classes/dedicated_server.h"

// #include "functions/gcrypt.h"

// #include <string>

using namespace GameAP;

Db *db;

int run_tasks()
{
    db_elems results;
    if (db->query("SELECT * FROM `gameap_gdaemon_tasks`", &results) == -1) {
        fprintf(stdout, "Error query\n");
        return -1;
    }

    for (auto itv = results.rows.begin(); itv != results.rows.end(); ++itv) {
        for (auto itr = itv->row.begin(); itr != itv->row.end(); ++itr) {
            std::cout << (*itr).first << ": " << (*itr).second << std::endl;
        }
    }
}

int main(int argc, char* argv[])
{
    std::cout << "Start" << std::endl;

    Config config;

    if (config.parse() == -1) {
        return -1;
    }

    if (load_db(&db, &config.db_driver[0]) == -1) {
        std::cerr << "Db load error" << std::endl;
        return -1;
    }

    if (db->connect(&config.db_host[0], &config.db_user[0], &config.db_passwd[0], &config.db_name[0], config.db_port) == -1) {
        std::cerr << "Connect to db error" << std::endl;
        return -1;
    }

    run_tasks();

    // unsigned int start_time =  clock();

    // int i = 0;
    // while (i < 1000000) {
        // GCrypt::aes_encrypt("str", "1234567890123456");
        // i++;
    // }
    /*
    char *encrypt;
    encrypt = GCrypt::aes_encrypt("12345678901234561234567890123456", "1234567890123456");
    std::cout << "encrypt: " << encrypt << std::endl;


    char *decrypt;
    decrypt = GCrypt::aes_decrypt(encrypt, "1234567890123456");
    std::cout << "DECrypt: " << decrypt << std::endl;
    */

    // Crypto crypto;

    // unsigned char *encMsg = NULL;
    // unsigned char *decMsg = NULL;
    // int encMsgLen;
    // int decMsgLen;
    
    // char * msg = "12345678901234561234567890123456";
    // char * key = "1234567890123456";

    // crypto.setAESKey((unsigned char*)&key[0], 16);

    // int i = 0;
    // while (i < 1000000) {
        // GCrypt::aes_encrypt("str", "1234567890123456");
        // crypto.aesEncrypt((const unsigned char*)&msg[0], strlen(msg)+1, &encMsg);
        // i++;
    // }


    // std::cout << "TIME: " << clock()-start_time << std::endl;

    run_server(6789);

    DedicatedServer deds;

    deds.stats_process();

    // unsigned int start_time =  clock();
    // int i = 0;
    // while (i < 100000) {
        // deds.stats_process();
        // i++;
    // }
    // std::cout << "TIME: " << (((float)clock()-start_time)/1000000.000) << std::endl;

    return 0;
}
