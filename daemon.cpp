#include <stdio.h>
#include <iostream>
#include <unistd.h>

#include <sstream>
#include <vector>
#include <map>

#include <thread>
#include <ctime>

#include <boost/format.hpp>

#include "daemon_server.h"
#include "dl.h"

#include "db/db.h"
#include "config.h"

#include "models/tasks.h"
#include "classes/dedicated_server.h"

// #include "functions/gcrypt.h"

// #include <string>

#define TASK_WAITING    1
#define TASK_WORKING    2
#define TASK_ERROR      3
#define TASK_SUCCESS    4

using namespace GameAP;

Db *db;

// ---------------------------------------------------------------------

int run_tasks()
{
    TaskList& tasks = TaskList::getInstance();

    tasks.update_list();

    // for (std::vector<Task *>::iterator it = tasks.begin(); it != tasks.end(); ++it) {
        // (**it).run();
    // }
}

// ---------------------------------------------------------------------

int check_tasks()
{
    TaskList& tasks = TaskList::getInstance();

    while (true) {
        // Delete finished
        for (std::vector<Task *>::iterator it = tasks.begin(); !tasks.is_end(it); it = tasks.next(it)) {
            
            if ((**it).get_status() == TASK_WORKING
                || (**it).get_status() == TASK_ERROR
                || (**it).get_status() == TASK_SUCCESS
            ) {
                std::string output = (**it).get_output();
                output = db->real_escape_string(&output[0]);
                
                std::string qstr = str(
                    boost::format(
                        "UPDATE `{pref}gdaemon_tasks` SET `output` = '%1%' WHERE `id` = %2%"
                    ) % output  % (**it).get_task_id()
                );
                
                db->query(&qstr[0]);
            }

            if ((**it).get_status() == TASK_ERROR || (**it).get_status() == TASK_SUCCESS) {
                tasks.delete_task(it);
            }
        }

        sleep(5);
    }
}

// ---------------------------------------------------------------------

void daemon()
{
    TaskList& tasks = TaskList::getInstance();

    tasks.update_list();
    for (std::vector<Task *>::iterator it = tasks.begin(); !tasks.is_end(it); it = tasks.next(it)) {
        (**it).run();
    }

    // std::thread task_update(check_tasks);

    // Task
}

// ---------------------------------------------------------------------

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

    std::thread thr1(check_tasks);
    std::thread daemon_server(run_server, 6789);
    
    DedicatedServer deds;
    deds.stats_process();

    while (true) {
        std::cout << "CICLE START" << std::endl;
        daemon();
        sleep(5);
    }

    return 0;
}
