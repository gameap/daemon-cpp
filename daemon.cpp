#include <stdio.h>
#include <iostream>

#ifdef __GNUC__
#include <unistd.h>
#include <thread>
#endif

#include <sstream>
#include <vector>
#include <map>

#include <ctime>

#include <boost/thread.hpp>
#include <boost/format.hpp>

#include "typedefs.h"

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

                if (output != "") {
                    output = db->real_escape_string(&output[0]);

                    std::string qstr = str(
                        boost::format(
                            "UPDATE `{pref}gdaemon_tasks` SET `output` = '%1%' WHERE `id` = %2%"
                        ) % output  % (**it).get_task_id()
                    );

                    db->query(&qstr[0]);
                }
            }

            if ((**it).get_status() == TASK_ERROR || (**it).get_status() == TASK_SUCCESS) {
                tasks.delete_task(it);
            }
        }

		#ifdef _WIN32
			Sleep(5000);
		#else
			sleep(5);
		#endif
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
}

// ---------------------------------------------------------------------

int main(int argc, char* argv[])
{
    std::cout << "Start" << std::endl;

    Config& config = Config::getInstance();

    if (config.parse() == -1) {
		std::cout << "Config parse error" << std::endl;
        return -1;
    }

    if (load_db(&db, &config.db_driver[0]) == -1) {
        std::cerr << "Db load error" << std::endl;
        return -1;
    }

    db->set_prefix(config.db_prefix);

    if (db->connect(&config.db_host[0], &config.db_user[0], &config.db_passwd[0], &config.db_name[0], config.db_port) == -1) {
        std::cerr << "Connect to db error" << std::endl;
        return -1;
    }

    boost::thread thr1(check_tasks);
    boost::thread daemon_server(run_server, config.listen_port);

    DedicatedServer& deds = DedicatedServer::getInstance();

    boost::thread_group daemon_thrs;

    while (true) {
        std::cout << "CICLE START" << std::endl;
        // daemon();

        daemon_thrs.create_thread([&]() {
            daemon();
        });

        deds.stats_process();
        deds.update_db();

		#ifdef _WIN32
			Sleep(5000);
		#else
			sleep(5);
		#endif
    }

    return 0;
}
