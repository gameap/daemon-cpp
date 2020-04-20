#ifndef GDAEMON_SERVER_TASK_H
#define GDAEMON_SERVER_TASK_H

namespace GameAP {
    struct ServerTask {
        enum status {
            WAITING,
            WORKING,
            SUCCESS,
            FAIL,
        } status;

        unsigned int id;
        unsigned char command;
        unsigned int server_id;
        unsigned short repeat;
        unsigned int repeat_period;
        unsigned int counter;
        time_t execute_date;
        std::string payload;
    };
}

#endif //GDAEMON_SERVER_TASK_H
