#ifndef TASKS_H
#define TASKS_H

#include <iostream>
#include <vector>

#include "classes/game_server.h"

#define TASK_WAITING    1
#define TASK_WORKING    2
#define TASK_ERROR      3
#define TASK_SUCCESS    4

#define TASK_GAME_SERVER_START      "gsstart"
#define TASK_GAME_SERVER_PAUSE      "gspause" // NOT implemented
#define TASK_GAME_SERVER_STOP       "gsstop"
#define TASK_GAME_SERVER_KILL       "gskill" // NOT implemented
#define TASK_GAME_SERVER_RESTART    "gsrest"
#define TASK_GAME_SERVER_INSTALL    "gsinst"
#define TASK_GAME_SERVER_REINSTALL  "gsreinst" // NOT implemented
#define TASK_GAME_SERVER_UPDATE     "gsupd"
#define TASK_GAME_SERVER_DELETE     "gsdel"
#define TASK_GAME_SERVER_MOVE       "gsmove"
#define TASK_GAME_SERVER_EXECUTE    "cmdexec"

namespace GameAP {

class Task {
private:
    ulong m_task_id;
    ulong m_task_run_after;
    
    ulong m_ds_id;
    ulong m_server_id;
    char m_task[8];
    
    std::string m_data;
    std::string m_cmd;
    std::string m_cmd_output;
    
    size_t m_cur_outpos;
    
    enum st {waiting = 1, working, error, success};
    ushort m_status;

    time_t m_task_started;

    GameServer *m_gserver;

    int _exec(std::string cmd);
    int _single_exec(std::string cmd);
    void _append_cmd_output(std::string line);
public:
    Task(ulong task_id, ulong ds_id, ulong server_id, const char * task, const char * data, const char * cmd, ushort status) {
        m_task_id = task_id;
        m_ds_id = ds_id;
        m_server_id = server_id;

        ushort mcsz = strlen(task);
        if (mcsz > 8) {
            mcsz = 8;
        }
        memcpy(m_task, task, mcsz);

        if (mcsz < 8) {
            m_task[mcsz] = '\0';
        }
        
        m_data = data;
        m_cmd = cmd;
        m_status = status;

        m_gserver = nullptr;
        
        m_cur_outpos = 0;
    }

    ~Task() {
        std::cout << "Task destructor: " << m_task_id << std::endl;
    }
    
    // void start(ulong m_task_id, ulong m_ds_id, ulong m_server_id, char m_task[8], char * m_data, char * m_cmd);

    void run();
    std::string get_output();

    int get_status()
    {
        return m_status;
    }

    ulong get_id()
    {
        return m_task_id;
    }
    
    ulong get_server_id()
    {
        return m_server_id;
    }

    ulong run_after(ulong aft)
    {
        m_task_run_after = aft;
        return m_task_run_after;
    }

    ulong run_after()
    {
        return m_task_run_after;
    }
    
    time_t get_time_started() {
        return m_task_started;
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

    bool stop;
};



/* End namespace GameAP */
}

#endif
