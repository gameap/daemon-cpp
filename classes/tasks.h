#ifndef TASKS_H
#define TASKS_H

#include <iostream>
#include <vector>

#include "db/db.h"
#include "classes/game_server.h"

#define TASK_WAITING    1
#define TASK_WORKING    2
#define TASK_ERROR      3
#define TASK_SUCCESS    4

namespace GameAP {

class Task {
private:
    ulong task_id;
    ulong task_run_after;
    
    ulong ds_id;
    ulong server_id;
    char task[8];
    
    std::string data;
    std::string cmd;

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

        if (mcsz < 8) {
            task[mcsz] = '\0';
        }
        
        data = mdata;
        cmd = mcmd;
        status = mstatus;

        gserver = nullptr;
        
        cur_outpos = 0;
    }

    ~Task() {
        std::cout << "Task destructor: " << task_id << std::endl;
    }
    
    // void start(ulong task_id, ulong ds_id, ulong server_id, char task[8], char * data, char * cmd);

    void run();
    std::string get_output();

    int get_status()
    {
        return status;
    }

    ulong get_id()
    {
        return task_id;
    }
    
    ulong get_server_id()
    {
        return server_id;
    }

    ulong run_after(ulong aft)
    {
        task_run_after = aft;
        return task_run_after;
    }

    ulong run_after()
    {
        return task_run_after;
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
    std::vector<ulong>taskids;

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

    void check_working_errors();

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
