#ifndef GDAEMON_GDAEMON_TASK_H
#define GDAEMON_GDAEMON_TASK_H

#include "server.h"

// TODO: Add STATUS_ prefixes to status enum names
#if _MSC_VER
#undef ERROR
#endif

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
