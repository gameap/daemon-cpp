#include <stdio.h>
#include <iostream>
#include <fstream>

#include <csignal>

void sighandler(int signum) {

    if (signum == SIGUSR1) {
        std::cout << "=== GameAP Daemon Status ===" << std::endl;

        /*
        std::ofstream output;

        output.open("/dev/tty");
        output << "=== GameAP Daemon Status ===" << std::endl;
        output.close();
         */
    }

    return;
}