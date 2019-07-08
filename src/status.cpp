#include <cstdio>
#include <iostream>
#include <fstream>

#include <csignal>

#include "classes/task_list.h"

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
    } else if (signum == SIGQUIT || signum == SIGTERM || signum == SIGINT) {
        std::cout << "Handle signal" << std::endl;
        std::cout << "Stopping tasks" << std::endl;

        TaskList& tasks = TaskList::getInstance();
        tasks.stop = true;
    }
}