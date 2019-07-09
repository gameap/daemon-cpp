#include <cstdio>
#include <iostream>
#include <fstream>

#include <csignal>

#include "classes/task_list.h"
#include "classes/game_servers_list.h"
#include "config.h"

using namespace GameAP;

void sighandler(int signum)
{
    if (signum == SIGUSR1) {
        std::cout << "=== GameAP Daemon Status ===" << std::endl;

        /*
        std::ofstream output;

        output.open("/dev/tty");
        output << "=== GameAP Daemon Status ===" << std::endl;
        output.close();
         */
    } else if (signum == SIGHUP) {
        // Reloading
        std::cout << "Handle signal" << std::endl;

        Config& config = Config::getInstance();
        config.parse();
        std::cout << "Config reloaded" << std::endl;

        GameServersList &gslist = GameServersList::getInstance();
        gslist.update_all(true);

        std::cout << "Servers updated" << std::endl;

    } else if (signum == SIGQUIT || signum == SIGTERM || signum == SIGINT) {
        std::cout << "Handle signal" << std::endl;
        std::cout << "Stopping tasks" << std::endl;

        TaskList& tasks = TaskList::getInstance();
        tasks.stop = true;
    }
}