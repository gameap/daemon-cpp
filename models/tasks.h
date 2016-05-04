#ifndef TASKS_H
#define TASKS_H

#include <iostream>
#include <vector>

#include "db/db.h"
#include "classes/game_server.h"

namespace GameAP {

/*
struct gtask {
    ulong id;
    ulong ds_id;
    char task[8];
    char * data;
    char * cmd;
    char * output;
    // enum status {"waiting", "working", "error", "success"};
};
*/

class Task {
private:
    ulong task_id;
    ulong ds_id;
    ulong server_id = 0;
    char * task;
    char * data;
    char * cmd;
    char * output;
    enum status {waiting = 1, working, error, success};

    time_t task_started;

    GameServer *gserver;
public:
    Task(ulong mtask_id, ulong mds_id, ulong mserver_id, char * mtask, char * mdata, char * mcmd) {
        task_id = mtask_id;
        ds_id = mds_id;
        server_id = mserver_id;
        task = mtask;
        data = mdata;
        cmd = mcmd;
    }
    // void start(ulong task_id, ulong ds_id, ulong server_id, char task[8], char * data, char * cmd);

    void run();
    std::string get_output();
    
    time_t get_time_started() {
        return task_started;
    }
};

// ---------------------------------------------------------------------

class TaskList {
private:
    std::vector<Task>tasklist;

    TaskList() {}
    TaskList( const TaskList&);  
    TaskList& operator=( TaskList& );
public:
    static TaskList& getInstance() {
        static TaskList instance;
        return instance;
    }
    
    int update_list();
    
    void insert(Task task);
    std::vector<Task>::iterator begin();
    int run_task();
    std::vector<Task>::iterator end();
};



/* End namespace GameAP */
}

#endif
