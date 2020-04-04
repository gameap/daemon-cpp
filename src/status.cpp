#include <iostream>

#include <csignal>

#include "log.h"

#include "status.h"
#include "classes/game_servers_list.h"
#include "config.h"

bool status_active;

using namespace GameAP;

void sighandler(int signum)
{
    if (signum == SIGUSR1) {
        GAMEAP_LOG_DEBUG << "=== GameAP Daemon Status ===";

        /*
        std::ofstream output;

        output.open("/dev/tty");
        output << "=== GameAP Daemon Status ===" << std::endl;
        output.close();
         */
    } else if (signum == SIGHUP) {
        // Reloading
        GAMEAP_LOG_INFO << "Handle reload signal";

        Config& config = Config::getInstance();
        config.parse();
        GAMEAP_LOG_INFO << "Config reloaded";

        GameServersList &gslist = GameServersList::getInstance();
        gslist.sync_all();

        GAMEAP_LOG_INFO << "Servers updated";
    } else if (signum == SIGQUIT || signum == SIGTERM || signum == SIGINT) {
        GAMEAP_LOG_INFO << "Handle quit signal";
        GAMEAP_LOG_INFO << "Stopping tasks";

        status_active = false;
    }
}