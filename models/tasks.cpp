#include "db/db.h"
#include "tasks.h"

#include <boost/format.hpp>
#include <cstring>

using namespace GameAP;

void Task::run()
{
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

    GameServersList& gslist = GameServersList::getInstance();
    
    if (! strcmp(task, "gsstart")) {
        try {
            gserver = gslist.get_server(server_id);

            if (gserver == nullptr) {
                throw std::runtime_error("gslist.get_server error");
            }
            
            gserver->clear_cmd_output();
            result_status = gserver->start_server();

            sleep(1);
            gserver->status_server();
            
            gserver = nullptr;
        } catch (std::exception &e) {
            status = error;
            std::cerr << "gsstart error: " << e.what() << std::endl;
        }
    }
    else if (! strcmp(task, "gsstop")) {
        try {
            gserver = gslist.get_server(server_id);
            
            if (gserver == nullptr) {
                throw std::runtime_error("gslist.get_server error");
            }
            
            gserver->clear_cmd_output();
            result_status = gserver->stop_server();
            
            sleep(1);
            gserver->status_server();
            
            gserver = nullptr;
        } catch (std::exception &e) {
            status = error;
            std::cerr << "gsstop error: " << e.what() << std::endl;
        }
    }
    else if (! strcmp(task, "gsrest")) {
        try {
            gserver = gslist.get_server(server_id);
            
            if (gserver == nullptr) {
                throw std::runtime_error("gslist.get_server error");
            }
            
            gserver->clear_cmd_output();
            result_status = gserver->stop_server();
            result_status = gserver->start_server();

            sleep(1);
            gserver->status_server();
            
            gserver = nullptr;
        } catch (std::exception &e) {
            status = error;
            std::cerr << "gsstop error: " << e.what() << std::endl;
        }
    }
    else if (!strcmp(task, "gsinst") || !strcmp(task, "gsupdate")) {
        try {
            gserver = gslist.get_server(server_id);
            
            if (gserver == nullptr) {
                throw std::runtime_error("gslist.get_server error");
            }
            
            gserver->clear_cmd_output();
            result_status = gserver->update_server();
            gserver = nullptr;
        } catch (std::exception &e) {
            status = error;
            std::cerr << "gsinst error: " << e.what() << std::endl;
        }
    }
    else if (! strcmp(task, "gsdelete")) {
        try {
            gserver = gslist.get_server(server_id);
            
            if (gserver == nullptr) {
                throw std::runtime_error("gslist.get_server error");
            }
            
            gserver->clear_cmd_output();
            result_status = gserver->delete_server();
            
            gserver = nullptr;
        } catch (std::exception &e) {
            status = error;
            std::cerr << "gsinst error: " << e.what() << std::endl;
        }
    }
    else if (! strcmp(task, "gsmove")) {
        // Move game server to other ds
        gserver = gslist.get_server(server_id);
        gserver->clear_cmd_output();
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
        std::string output;
        cur_outpos += gserver->get_cmd_output(&output, cur_outpos);
        return output;
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
            Task * task = new Task(
                (ulong)atoi(itv->row["id"].c_str()),
                (ulong)atoi(itv->row["ds_id"].c_str()),
                (ulong)atoi(itv->row["server_id"].c_str()),
                itv->row["task"].c_str(),
                itv->row["data"].c_str(),
                itv->row["cmd"].c_str(),
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
    if (tasklist.size() <= 0) {
        return tasklist.end();
    } else {
        return tasklist.begin();
    }
}


// ---------------------------------------------------------------------

std::vector<Task *>::iterator TaskList::next(std::vector<Task *>::iterator curit)
{
    if (curit == tasklist.end()) {
        return begin();
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
