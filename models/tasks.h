#ifndef TASKS_H
#define TASKS_H

#include <iostream>
#include <vector>

#include "db/db.h"
#include "classes/game_server.h"

namespace GameAP {

class Task {
private:
    ulong task_id;
    ulong ds_id;
    ulong server_id;
    char task[8];
    const char * data;
    const char * cmd;

    size_t cur_outpos;
    
    enum st {waiting = 1, working, error, success};
    ushort status;

    time_t task_started;

    GameServer *gserver;
public:
    Task(ulong mtask_id, ulong mds_id, ulong mserver_id, const char * mtask, const char * mdata, const char * mcmd, ushort mstatus) {
        task_id = mtask_id;
        ds_id = mds_id;
        server_id = mserver_id;

        ushort mcsz = strlen(mtask);
        if (mcsz > 8) {
            mcsz = 8;
        }
        memcpy(task, mtask, mcsz);
        
        data = mdata;
        cmd = mcmd;
        status = mstatus;

        cur_outpos = 0;
    }

    ~Task() {
        std::cout << "Task destructor" << std::endl;

        if (gserver != nullptr) {
            delete gserver;
        }
    }
    
    // void start(ulong task_id, ulong ds_id, ulong server_id, char task[8], char * data, char * cmd);

    void run();
    std::string get_output();
    
    ulong get_task_id() {
        return task_id;
    }

    int get_status()
    {
        return status;
    }
    
    time_t get_time_started() {
        return task_started;
    }
};

// ---------------------------------------------------------------------

class TaskList {
private:
    enum st {waiting = 1, working, error, success};
    
    std::vector<Task *>tasklist;

    void _clear_tasklist();
    
    TaskList() {}
    TaskList( const TaskList&);  
    TaskList& operator=( TaskList& );
public:
    static TaskList& getInstance() {
        static TaskList instance;
        return instance;
    }
    
    int update_list();
    int delete_task(std::vector<Task *>::iterator it);
    
    void insert(Task * task);
    std::vector<Task *>::iterator begin();
    int run_task();
    std::vector<Task *>::iterator end();

    std::vector<Task *>::iterator next(std::vector<Task *>::iterator curit);
    bool is_end(std::vector<Task *>::iterator curit);
};



/* End namespace GameAP */
}

#endif
