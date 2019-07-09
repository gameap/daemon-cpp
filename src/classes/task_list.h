#ifndef GDAEMON_TASK_LIST_H
#define GDAEMON_TASK_LIST_H

#include <iostream>
#include <vector>

#include "tasks.h"

namespace GameAP {

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

#endif //GDAEMON_TASK_LIST_H
