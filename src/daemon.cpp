#include <iostream>

#ifdef __GNUC__
#include <thread>
#endif

#include <vector>
#include <chrono>

#include "log.h"
#include "daemon_server.h"
#include "config.h"

#include "classes/game_servers_list.h"
#include "classes/gdaemon_tasks.h"
#include "classes/servers_tasks.h"
#include "classes/dedicated_server.h"

#include "functions/restapi.h"
#include "status.h"

using namespace GameAP;

void check_tasks()
{
    // TODO: Coroutines here
    std::thread server_tasks([&]() {
        ServersTasks &servers_tasks = ServersTasks::getInstance();

        while (status_active) {
            servers_tasks.update();

            while (!servers_tasks.empty()) {
                servers_tasks.run_next();
            }

            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    });

    GdaemonTasks& gdaemon_tasks = GdaemonTasks::getInstance();
    while (status_active) {
        gdaemon_tasks.update();

        while (!gdaemon_tasks.empty()) {
            gdaemon_tasks.run_next();
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    if (server_tasks.joinable()) {
        server_tasks.join();
    }

    GAMEAP_LOG_INFO << "Tasks Stopped";
}

int run_daemon()
{
    GAMEAP_LOG_INFO << "Starting...";
    status_active = true;

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
        Rest::get_token();
    } catch (Rest::RestapiException &exception) {
        GAMEAP_LOG_ERROR << exception.what();
        return -1;
    }

    // Run Daemon Server
    std::thread daemon_server([&]() {
        int server_exit_status;

        while (status_active) {
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

        if (!boost::filesystem::exists(deds.get_work_path())) {
            boost::filesystem::create_directory(deds.get_work_path());
        }

        // Change working directory
        boost::filesystem::current_path(deds.get_work_path());

        boost::thread ctsk_thr(check_tasks);

        while (status_active) {
            deds.stats_process();
            deds.update_db();

            try {daemon_server.join();
                gslist.loop();
            } catch (std::exception &e) {
                GAMEAP_LOG_ERROR << "Game servers stats process error: " << e.what();
            }

            std::this_thread::sleep_for(std::chrono::seconds(30));
        }
    }

    if (daemon_server.joinable()) {
        daemon_server.join();
    }

    return 0;
}
