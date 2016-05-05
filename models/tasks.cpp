#include "db/db.h"
#include "tasks.h"

#include <boost/format.hpp>
#include <cstring>

using namespace GameAP;

void Task::run()
{
    std::cout << "Run task" << std::endl;
    std::cout << "this task: " << task << std::endl;

    if (status != waiting) {
        return;
    }

    task_started = time(0);
    status = working;

    std::string qstr;

    qstr = str(boost::format("UPDATE `{pref}gdaemon_tasks` SET `status` = 'working' WHERE `id` = %1%") % task_id);
    db->query(&qstr[0]);
    
    // if (db->query(
        // "SELECT `id`, `ds_id`, `server_id`, `task`, `data`, `cmd`, status+0 AS status\
            // FROM `{pref}gdaemon_tasks`\
            // WHERE `status` = 'waiting'",
        // &results) == -1
    // ){
        // fprintf(stdout, "Error query\n");
        // return -1;
    // }

    int result_status;
    
    if (! strcmp(task, "gsstart")) {
        // GameServer gserver(server_id);
        // gserver.start_server();
        // output = gserver.get_results_last_cmd();
    }
    else if (! strcmp(task, "gsstop")) {
        
    }
    else if (! strcmp(task, "gsrest")) {
        
    }
    else if (!strcmp(task, "gsinst") || !strcmp(task, "gsupdate")) {
        try {
            gserver = new GameServer(server_id);
            result_status = gserver->update_server();
            delete gserver;
            gserver = nullptr;
        } catch (boost::filesystem::filesystem_error &e) {
            status = error;
            std::cerr << "gsinst error: " << e.what() << std::endl;
        }
    }
    else if (! strcmp(task, "gsdelete")) {
        
    }
    else if (! strcmp(task, "cmdexec")) {
        
    }

    if (result_status == -1) {
        status = error;
        qstr = str(boost::format("UPDATE `{pref}gdaemon_tasks` SET `status` = 'error' WHERE `id` = %1%") % task_id);
        db->query(&qstr[0]);
    }
    else {
        status = success;
        qstr = str(boost::format("UPDATE `{pref}gdaemon_tasks` SET `status` = 'success' WHERE `id` = %1%") % task_id);
        db->query(&qstr[0]);
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

void TaskList::_clear_tasklist()
{
    size_t cur = 0;
    while (tasklist.size() != 0) {
        delete tasklist[cur];
        tasklist.erase(tasklist.begin()+cur);
        cur++;
    }
}

// ---------------------------------------------------------------------

int TaskList::delete_task(std::vector<Task *>::iterator it)
{
    delete *it;
    tasklist.erase(it);

    return 0;
}

// ---------------------------------------------------------------------

int TaskList::update_list()
{
    db_elems results;

    if (db->query(
        "SELECT `id`, `ds_id`, `server_id`, `task`, `data`, `cmd`, status+0 AS status\
            FROM `{pref}gdaemon_tasks`\
            WHERE `status` = 'waiting'",
        &results) == -1
    ){
        fprintf(stdout, "Error query\n");
        return -1;
    }

    // _clear_tasklist();

    // Task tasks;
    for (auto itv = results.rows.begin(); itv != results.rows.end(); ++itv) {
            std::cout << "CMD: " << itv->row["cmd"] << std::endl;
            
            Task * task = new Task(
                (ulong)atoi(itv->row["id"].c_str()),
                (ulong)atoi(itv->row["ds_id"].c_str()),
                (ulong)atoi(itv->row["server_id"].c_str()),
                &itv->row["task"][0],
                &itv->row["data"][0],
                &itv->row["cmd"][0],
                (ushort)atoi(itv->row["status"].c_str())
            );

            insert(task);
    }
    return 0;
}

// ---------------------------------------------------------------------

void TaskList::insert(Task * task)
{
    tasklist.push_back(task);
    // it = tasklist.begin();
}

// ---------------------------------------------------------------------

std::vector<Task *>::iterator TaskList::begin()
{
    return tasklist.begin();
}


// ---------------------------------------------------------------------

std::vector<Task *>::iterator TaskList::next(std::vector<Task *>::iterator curit)
{
    if (curit == tasklist.end()) {
        return tasklist.begin();
    }

    return ++curit;
}

// ---------------------------------------------------------------------

bool TaskList::is_end(std::vector<Task *>::iterator curit)
{
    if (tasklist.size() <= 0) {
        return true;
    }

    if (curit == tasklist.end()) {
        return true;
    }

    return false;
}

// ---------------------------------------------------------------------

std::vector<Task *>::iterator TaskList::end()
{
    std::cout << "Tasklist size: " << tasklist.size() << std::endl;
    std::cout << "Tasklist end: " << *tasklist.end() << std::endl;
    return tasklist.end();
}
