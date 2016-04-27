#include <stdio.h>
#include <iostream>
#include <unistd.h>

#include <sstream>
#include <map>

#include <ctime>

#include "file_server.h"
#include "dl.h"

#include "db/db.h"
#include "config.h"

// #include "functions/gcrypt.h"

// #include <string>

int main(int argc, char* argv[])
{
    std::cout << "Start" << std::endl;

    Config config;

    if (config.parse() == -1) {
        return -1;
    }

    Db *db;
    // db->setDriver(&config.db_driver[0]);
    if (load_db(&db, &config.db_driver[0]) == -1) {
        return -1;
    }

    // Бесконечный цикл работы демона
    // for (;;) {
        // printf("Cicle\n");
        // usleep(1000);
    // }

    /*
    if (db->connect(&config.db_host[0], &config.db_user[0], &config.db_passwd[0], &config.db_name[0], config.db_port) == -1) {
        fprintf(stdout, "Error connect\n");
        return -1;
    }

    db_elems results;

    if (db->query("SELECT * FROM `gameap_users`", results) == -1) {
        fprintf(stdout, "Error query\n");
        return -1;
    }

    for (int i = 0; i < results.num_rows; i++) {
        for (auto it = results.rows[i].begin(); it != results.rows[i].end(); ++it) {
            std::cout << it->first << " -:- "<< it->second << std::endl;
        }

        // auto it = results.rows[i].find("password");
        // std::cout << results.rows[i]["password"] << std::endl;
    }
    */

    unsigned int start_time =  clock();

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

    run_file_server(6789);
    

    return 0;
}
