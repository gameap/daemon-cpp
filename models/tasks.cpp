#include "db/db.h"
#include "tasks.h"

#include <cstring>

using namespace GameAP;

void Task::run()
{
    std::cout << "Run task" << std::endl;
    std::cout << "this task: " << task << std::endl;

    task_started = time(0);
    
    if (! strcmp(task, "gsstart")) {
        // GameServer gserver(server_id);
        // gserver.start_server();
        // output = gserver.get_results_last_cmd();
    }
    else if (! strcmp(task, "gsstop")) {
        
    }
    else if (! strcmp(task, "gsrest")) {
        
    }
    else if (! strcmp(task, "gsupdate")) {
        // GameServer gserver(server_id);
        // gserver.update_server();

        // std::cout << "Update result: " << gserver.get_cmd_output() << std::endl;
    }
    else if (! strcmp(task, "gsinst")) {
        try {
            gserver = new GameServer(server_id);
            gserver->update_server();

            // std::cout << "Output: " << get_output() << std::endl;
        } catch (boost::filesystem::filesystem_error &e) {
            std::cerr << "gsinst error: " << e.what() << std::endl;
        }
    }
    else if (! strcmp(task, "gsdelete")) {
        
    }
    else if (! strcmp(task, "cmdexec")) {
    }
}

// ---------------------------------------------------------------------

std::string Task::get_output()
{
    if (server_id != 0 && gserver != nullptr) {
        return gserver->get_cmd_output();
    }

    return "";
}

// ---------------------------------------------------------------------

int TaskList::update_list()
{
    db_elems results;

    if (db->query("SELECT * FROM `{pref}gdaemon_tasks`", &results) == -1) {
        fprintf(stdout, "Error query\n");
        return -1;
    }

    // Task tasks;
    for (auto itv = results.rows.begin(); itv != results.rows.end(); ++itv) {
            std::cout << "CMD: " << itv->row["cmd"] << std::endl;
            
            Task * task = new Task(
                (ulong)atoi(itv->row["id"].c_str()),
                (ulong)atoi(itv->row["ds_id"].c_str()),
                (ulong)atoi(itv->row["server_id"].c_str()),
                &itv->row["task"][0],
                &itv->row["data"][0],
                &itv->row["cmd"][0]
            );
            
            insert(*task);
    }

    // it = tasklist.begin();

    return 0;
}

// ---------------------------------------------------------------------

void TaskList::insert(Task task)
{
    tasklist.push_back(task);
    // it = tasklist.begin();
}

// ---------------------------------------------------------------------

std::vector<Task>::iterator TaskList::begin()
{
    return tasklist.begin();
}

// ---------------------------------------------------------------------

std::vector<Task>::iterator TaskList::end()
{
    return tasklist.end();
}
