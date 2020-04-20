#ifndef GDAEMON_STATUS_H
#define GDAEMON_STATUS_H

extern bool status_active;
extern time_t status_started_time;

void sighandler(int signum);

#endif //GDAEMON_STATUS_H
