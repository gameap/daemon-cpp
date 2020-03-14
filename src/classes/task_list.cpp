#include "tasks.h"
#include "task_list.h"
#include "consts.h"

#include <json/json.h>
#include <boost/iostreams/stream.hpp>

#include "functions/restapi.h"

using namespace GameAP;

void TaskList::insert(Task * task)
{
    tasklist.push_back(task);
    taskids.push_back(task->get_id());
}


int TaskList::delete_task(std::vector<Task *>::iterator it)
{
    ulong task_id = (*it)->get_id();

    auto idit = std::find(taskids.begin(), taskids.end(), task_id);
    if (idit != taskids.end()) {
        taskids.erase(idit);
    }

    delete *it;
    tasklist.erase(it);

    return 0;
}


int TaskList::update_list()
{
    Json::Value jvalue;

    try {
        jvalue = Gameap::Rest::get("/gdaemon_api/tasks?filter[status]=waiting&append=status_num");
    } catch (Gameap::Rest::RestapiException &exception) {
        // Try later
        GAMEAP_LOG_ERROR << exception.what();
        return ERROR_STATUS_INT;
    }

    for( Json::ValueIterator itr = jvalue.begin() ; itr != jvalue.end() ; itr++ ) {
        ulong task_id = (*itr)["id"].asLargestUInt();
        if (std::find(taskids.begin(), taskids.end(), task_id) != taskids.end()) {
            continue;
        }

        if ((*itr)["task"].asString() == "") {
            continue;
        }

        if ((*itr)["status_num"].asUInt() != TASK_WAITING) {
            continue;
        }

        Task * task = new Task(
                task_id,
                (*itr)["dedicated_server_id"].asLargestUInt(),
                (*itr)["server_id"].asLargestUInt(),
                (*itr)["task"].asCString(),
                (*itr)["data"].isNull() ? "" : (*itr)["data"].asCString(),
                (*itr)["cmd"].isNull() ? "" : (*itr)["cmd"].asCString(),
                TASK_WAITING // (ushort)(*itr)["status"].asUInt()
        );

        if ((*itr)["run_aft_id"].asUInt() != 0) {
            task->run_after((ulong)(*itr)["run_aft_id"].asUInt());
        } else {
            task->run_after(0);
        }

        insert(task);
    }

    return SUCCESS_STATUS_INT;
}


void TaskList::check_working_errors()
{
    Json::Value jvalue;

    try {
        jvalue = Gameap::Rest::get("/gdaemon_api/tasks?filter[status]=working&append=status_num");
    } catch (Gameap::Rest::RestapiException &exception) {
        // Try later
        GAMEAP_LOG_ERROR << exception.what();
        return;
    }

    for (Json::ValueIterator itr = jvalue.begin(); itr != jvalue.end(); itr++) {
        ulong task_id = (*itr)["id"].asLargestUInt();

        if ((*itr)["status_num"].asUInt() != TASK_WORKING) {
            continue;
        }

        if (std::find(taskids.begin(), taskids.end(), task_id) == taskids.end()) {

            try {
                Json::Value jdata;
                jdata["status"] = error;
                Gameap::Rest::put("/gdaemon_api/tasks/" + std::to_string(task_id), jdata);
            } catch (Gameap::Rest::RestapiException &exception) {
                GAMEAP_LOG_ERROR << "Error updating task status [to status code " + std::to_string(error) + "]: "
                          << exception.what();
            }
        }
    }
}


unsigned long TaskList::count()
{
    return this->tasklist.size();
}


std::vector<Task *>::iterator TaskList::begin()
{
    if (tasklist.size() <= 0) {
        return tasklist.end();
    } else {
        return tasklist.begin();
    }
}


std::vector<Task *>::iterator TaskList::next(std::vector<Task *>::iterator curit)
{
    if (curit == tasklist.end()) {
        return begin();
    }

    return ++curit;
}


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


std::vector<Task *>::iterator TaskList::end()
{
    return tasklist.end();
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