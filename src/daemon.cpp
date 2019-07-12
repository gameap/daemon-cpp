#include <stdio.h>
#include <iostream>

#ifdef __GNUC__
#include <unistd.h>
#include <thread>
#endif

#include <sstream>
#include <vector>
#include <map>
#include <chrono>
#include <thread>
#include <ctime>

#include <boost/format.hpp>

#include "typedefs.h"

#include "daemon_server.h"
#include "dl.h"

#include "config.h"

#include "classes/tasks.h"
#include "classes/task_list.h"
#include "classes/dedicated_server.h"
#include "classes/game_servers_list.h"

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
    tasks.stop = false;

    tasks.check_working_errors();

    std::vector<ulong>      servers_working;
    std::vector<ulong>      tasks_ended;
    std::vector<ulong>      tasks_runned;

    boost::thread_group     tasks_thrs;

    std::string output = "";

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
                    (**it).run();
                });

                tasks_runned.push_back((**it).get_id());
            }

            // Check task
            if ((**it).get_status() == TASK_WORKING
                || (**it).get_status() == TASK_ERROR
                || (**it).get_status() == TASK_SUCCESS
            ) {
                if (output.empty()) {
                    try {
                        output = (**it).get_output();
                    } catch (std::exception &e) {
                        std::cerr << "get_output() error: " << e.what() << std::endl;
                    }
                }

                if (output.length() > 0) {

                    try {
                        Json::Value jdata;
                        jdata["output"] = output;
                        Gameap::Rest::put("/gdaemon_api/tasks/" + std::to_string((**it).get_id()) + "/output", jdata);
                        output.erase();
                    } catch (Gameap::Rest::RestapiException &exception) {
                        std::cerr << "Output updating error: "
                                  << exception.what()
                                  << std::endl;
                    }
                }
            }

            // End task. Erase data
            if ((**it).get_status() == TASK_ERROR || (**it).get_status() == TASK_SUCCESS || (**it).get_status() == TASK_CANCELED) {
                if (output.empty()) {
                    try {
                        output = (**it).get_output();
                    } catch (std::exception &e) {
                        std::cerr << "get_output() error: " << e.what() << std::endl;
                    }
                }

                if (output.empty()) {
                    tasks_ended.push_back((**it).get_id());
                    tasks_runned.erase(run_it);
                    tasks.delete_task(it);

                    if (swork_it != servers_working.end()) {
                        servers_working.erase(swork_it);
                    }
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    return 0;
}

// ---------------------------------------------------------------------

int run_daemon()
{
    std::cout << "Starting..." << std::endl << std::endl;

    #ifdef __linux__
        std::signal(SIGHUP, sighandler);
        std::signal(SIGUSR1, sighandler);
        std::signal(SIGQUIT, sighandler);
        std::signal(SIGINT, sighandler);
        std::signal(SIGTERM, sighandler);
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

    auto run_daemon_server = [&]() {
        std::cout << "Running Daemon Server" << std::endl;
        run_server(config.listen_ip, config.listen_port);
        std::cout << "Daemon Server stopped" << std::endl;
    };

    std::thread daemon_server(run_daemon_server);

    if (config.ds_id == 0) {
        std::cout << "Empty Dedicated Server ID" << std::endl;
        std::cout << "Tasks feature disabled" << std::endl;
    } else {
        DedicatedServer &deds = DedicatedServer::getInstance();
        GameServersList &gslist = GameServersList::getInstance();

        TaskList& tasks = TaskList::getInstance();
        tasks.stop = false;

        if (!boost::filesystem::exists(deds.get_work_path())) {
            boost::filesystem::create_directory(deds.get_work_path());
        }

        // Change working directory
        boost::filesystem::current_path(deds.get_work_path());

        boost::thread ctsk_thr(check_tasks);

        while (!tasks.stop) {
            deds.stats_process();
            deds.update_db();

            try {
                gslist.loop();
            } catch (std::exception &e) {
                std::cerr << "Game servers stats process error: " << e.what() << std::endl;
            }

            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    daemon_server.join();

    return 0;
}
