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

#include "classes/tasks.h"
#include "classes/dedicated_server.h"
#include "classes/game_server.h"

#include "functions/gcrypt.h"
#include "functions/restapi.h"

#include "status.h"

#ifdef __linux__
#include <csignal>
#endif

using namespace GameAP;

// ---------------------------------------------------------------------

int check_tasks()
{
    TaskList& tasks = TaskList::getInstance();

    tasks.check_working_errors();

    std::vector<ulong>      servers_working;
    std::vector<ulong>      tasks_ended;
    std::vector<ulong>      tasks_runned;

    boost::thread_group     tasks_thrs;

    std::string output = "";

    tasks.stop = false;
    while (!tasks.stop) {
        tasks.update_list();
        for (std::vector<Task *>::iterator it = tasks.begin(); !tasks.is_end(it); it = tasks.next(it)) {

            std::vector<ulong>::iterator run_it     = std::find(tasks_runned.begin(), tasks_runned.end(), (**it).get_id());
            std::vector<ulong>::iterator swork_it    = std::find(servers_working.begin(), servers_working.end(), (**it).get_server_id());

            // Run task
            if ((**it).get_status() == TASK_WAITING
                && swork_it == servers_working.end()
                && run_it == tasks_runned.end()
                && ((**it).run_after() == 0
                    || ((**it).run_after() != 0
                        && std::find(tasks_ended.begin(), tasks_ended.end(), (**it).run_after()) != tasks_ended.end()
                    )
                )
            ) {
                if ((**it).get_server_id() != 0) {
                    servers_working.push_back((**it).get_server_id());
                }

                tasks_thrs.create_thread([=]() {
                    std::cout << "Task ptr: " << *it << std::endl;
                    (**it).run();
                });

                tasks_runned.push_back((**it).get_id());
            }

            // Check task
            if ((**it).get_status() == TASK_WORKING
                || (**it).get_status() == TASK_ERROR
                || (**it).get_status() == TASK_SUCCESS
            ) {
                if (output == "") {
                    try {
                        output = (**it).get_output();
                    } catch (std::exception &e) {
                        std::cerr << "get_output() error: " << e.what() << std::endl;
                    }
                }

                if (output != "") {
                    try {
                        Json::Value jdata;
                        jdata["output"] = output;
                        Gameap::Rest::put("/gdaemon_api/tasks/" + std::to_string((**it).get_id()) + "/output", jdata);
                        output = "";
                    } catch (Gameap::Rest::RestapiException &exception) {
                        std::cerr << "Output updating error: "
                                  << exception.what()
                                  << std::endl;
                    }
                }
            }

            // End task. Erase data
            if ((**it).get_status() == TASK_ERROR || (**it).get_status() == TASK_SUCCESS) {
                if (output == "") {
                    try {
                        output = (**it).get_output();
                    } catch (std::exception &e) {
                        std::cerr << "get_output() error: " << e.what() << std::endl;
                    }
                }

                if (output == "") {
                    tasks_ended.push_back((**it).get_id());
                    tasks_runned.erase(run_it);
                    tasks.delete_task(it);

                    if (swork_it != servers_working.end()) {
                        servers_working.erase(swork_it);
                    }
                }
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

int run_daemon()
{
    std::cout << "Start" << std::endl;

    #ifdef __linux__
        /*
        sigset_t mask;
        struct sigaction act;

        sigaddset(&mask, SIGUSR1);

        act.sa_handler = sighandler;
        act.sa_flags = SA_SIGINFO;
        act.sa_mask = mask;

        sigaction(SIGUSR1, &act, nullptr);
         */
        std::signal(SIGUSR1, sighandler);
    #endif



    Config& config = Config::getInstance();

    if (config.parse() == -1) {
		std::cerr << "Config parse error" << std::endl;
        return -1;
    }

    try {
        Gameap::Rest::get_token();
    } catch (Gameap::Rest::RestapiException &exception) {
        std::cerr << exception.what() << std::endl;
        return -1;
    }

    boost::thread daemon_server(run_server, config.listen_port);

    DedicatedServer& deds = DedicatedServer::getInstance();
    GameServersList& gslist = GameServersList::getInstance();

    // Change working directory
    boost::filesystem::current_path(deds.get_work_path());

    boost::thread ctsk_thr(check_tasks);

    while (true) {
        deds.stats_process();
        deds.update_db();

        try {
            gslist.stats_process();
        } catch (std::exception &e) {
            std::cerr << "Game servers stats process error: " << e.what() << std::endl;
        }

		#ifdef _WIN32
			Sleep(5000);
		#else
			sleep(5);
		#endif
    }

    return 0;
}
