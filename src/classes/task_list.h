#ifndef GDAEMON_TASK_LIST_H
#define GDAEMON_TASK_LIST_H

#include <iostream>
#include <vector>

#include "tasks.h"

namespace GameAP {

class TaskList {
    public:
        static TaskList& getInstance() {
            static TaskList instance;
            return instance;
        }

        bool stop;

        int update_list();
        void check_working_errors();

        // Task management
        void insert(Task * task);
        int delete_task(std::vector<Task *>::iterator it);

        // Statistics

        /**
         * Get the number of all tasks
         * @return
         */
        unsigned long count();

        // Iterator
        std::vector<Task *>::iterator begin();
        int run_task();
        std::vector<Task *>::iterator end();

        std::vector<Task *>::iterator next(std::vector<Task *>::iterator curit);
        bool is_end(std::vector<Task *>::iterator curit);
    private:
        enum st {waiting = 1, working, error, success};

        std::vector<Task *>tasklist;
        std::vector<ulong>taskids;

        void _clear_tasklist();

        TaskList() {}
        TaskList( const TaskList&);
        TaskList& operator=( TaskList& );
};

/* End namespace GameAP */
}

#endif //GDAEMON_TASK_LIST_H
