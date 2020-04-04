#ifndef GDAEMON_GDAEMON_TASK_H
#define GDAEMON_GDAEMON_TASK_H

#include "server.h"

namespace GameAP {
    struct GdaemonTask {
        unsigned int id;
        unsigned int run_aft_id;
        unsigned int server_id;
        Server* server;
        std::string task;
        std::string cmd;

        enum status {
            WAITING = 1,
            WORKING,
            ERROR,
            SUCCESS,
            CANCELED,
        } status;
    };
}

#endif //GDAEMON_GDAEMON_TASK_H
