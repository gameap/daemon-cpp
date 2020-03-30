#include <iostream>

#ifdef __GNUC__
#include <thread>
#endif

#include <vector>
#include <chrono>

#include "log.h"
#include "daemon_server.h"
#include "config.h"

#include "classes/tasks.h"
#include "classes/task_list.h"
#include "classes/dedicated_server.h"
#include "classes/game_servers_list.h"

#include "functions/restapi.h"

#include "status.h"

#ifdef __linux__
#include <csignal>
#endif

using namespace GameAP;

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
        for (auto it = tasks.begin(); !tasks.is_end(it); it = tasks.next(it)) {

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
                    try {
                        (**it).run();
                    } catch (std::exception &e) {
                        GAMEAP_LOG_ERROR << "Run tasks error: " << e.what();
                    }
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
                        GAMEAP_LOG_ERROR << "get_output() error: " << e.what();
                    }
                }

                if (output.length() > 0) {

                    // Try to update output.
                    // Erase output after successful update or 5 unsuccessful attempts to update

                    ushort output_update_tries = 0;
                    while (output_update_tries < 5) {

                        output_update_tries++;

                        try {
                            Json::Value jdata;
                            jdata["output"] = output;
                            Gameap::Rest::put("/gdaemon_api/tasks/" + std::to_string((**it).get_id()) + "/output", jdata);
                            break;
                        } catch (Gameap::Rest::RestapiException &exception) {
                            GAMEAP_LOG_ERROR << "Output updating error: " << exception.what();

                            std::this_thread::sleep_for(std::chrono::seconds(10));
                        }
                    }

                    output.erase();
                }
            }

            // End task. Erase data
            if ((**it).get_status() == TASK_ERROR || (**it).get_status() == TASK_SUCCESS || (**it).get_status() == TASK_CANCELED) {
                if (output.empty()) {
                    try {
                        output = (**it).get_output();
                    } catch (std::exception &e) {
                        GAMEAP_LOG_ERROR << "get_output() error: " << e.what();
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

int run_daemon()
{
    GAMEAP_LOG_INFO << "Starting...";

    #ifdef __linux__
        std::signal(SIGHUP, sighandler);
        std::signal(SIGUSR1, sighandler);
        std::signal(SIGQUIT, sighandler);
        std::signal(SIGINT, sighandler);
        std::signal(SIGTERM, sighandler);
    #endif

    Config& config = Config::getInstance();

    if (config.parse() == -1) {
        GAMEAP_LOG_ERROR << "Config parse error";
        return -1;
    }

    plog::get<GameAP::MainLog>()->setMaxSeverity(
        plog::severityFromString(config.log_level.c_str())
    );

    plog::get<GameAP::ErrorLog>()->setMaxSeverity(
        plog::severityFromString(config.log_level.c_str())
    );

    try {
        Gameap::Rest::get_token();
    } catch (Gameap::Rest::RestapiException &exception) {
        GAMEAP_LOG_ERROR << exception.what();
        return -1;
    }

    // Run Daemon Server
    std::thread daemon_server([&]() {
        // TODO: Check fails
        // TODO: Replace tasks.stop to some status checker class or something else
        TaskList& tasks = TaskList::getInstance();

        int server_exit_status;

        while (!tasks.stop) {
            GAMEAP_LOG_INFO << "Running Daemon Server";
            server_exit_status = run_server(config.listen_ip, config.listen_port);
            GAMEAP_LOG_INFO << "Daemon Server stopped (Exit status: " << server_exit_status << ")";
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    });

    // Run Tasks
    if (config.ds_id == 0) {
        GAMEAP_LOG_WARNING << "Empty Dedicated Server ID";
        GAMEAP_LOG_WARNING << "Tasks feature disabled";
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
                GAMEAP_LOG_ERROR << "Game servers stats process error: " << e.what();
            }

            std::this_thread::sleep_for(std::chrono::seconds(30));
        }
    }

    daemon_server.join();

    return 0;
}
