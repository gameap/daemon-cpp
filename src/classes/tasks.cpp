#include "tasks.h"
#include "config.h"
#include "consts.h"

#include <boost/process.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/format.hpp>

#include <cstring>

#include "functions/restapi.h"
#include "functions/gsystem.h"

using namespace boost::process;
using namespace GameAP;

namespace fs = boost::filesystem;
namespace bp = ::boost::process;

void Task::run()
{
    if (status != waiting) {
        return;
    }

    task_started = time(nullptr);
    status = working;

    std::string qstr;

    try {
        Json::Value jdata;
        jdata["status"] = status;
        jdata["time_stchange"] = (uint) task_started;
        Gameap::Rest::put("/gdaemon_api/tasks/" + std::to_string(task_id), jdata);
    } catch (Gameap::Rest::RestapiException &exception) {
        std::cerr << "Error updating task status [to status code " + std::to_string(status) + "]: "
                  << exception.what()
                  << std::endl;

        return;
    }

    std::cout << "task: " << task << std::endl;

    int result_status;
    GameServersList& gslist = GameServersList::getInstance();
    
    if (! strcmp(task, TASK_GAME_SERVER_START)) {
        try {
            gserver = gslist.get_server(server_id);

            if (gserver == nullptr) {
                throw std::runtime_error("gslist.get_server error");
            }
            
            gserver->clear_cmd_output();
            result_status = gserver->start_server();

            std::this_thread::sleep_for(std::chrono::seconds(1));
            gserver->status_server();
            
            // gserver = nullptr;
        } catch (std::exception &e) {
            status = error;
            std::cerr << "gsstart error: " << e.what() << std::endl;
        }
    }
    else if (! strcmp(task, TASK_GAME_SERVER_STOP)) {
        try {
            gserver = gslist.get_server(server_id);
            
            if (gserver == nullptr) {
                throw std::runtime_error("gslist.get_server error");
            }
            
            gserver->clear_cmd_output();
            result_status = gserver->stop_server();
            
            std::this_thread::sleep_for(std::chrono::seconds(5));
            gserver->status_server();
            
            // gserver = nullptr;
        } catch (std::exception &e) {
            status = error;
            std::cerr << "gsstop error: " << e.what() << std::endl;
        }
    }
    else if (! strcmp(task, TASK_GAME_SERVER_RESTART)) {
        try {
            gserver = gslist.get_server(server_id);
            
            if (gserver == nullptr) {
                throw std::runtime_error("gslist.get_server error");
            }
            
            gserver->clear_cmd_output();
            result_status = gserver->stop_server();
            result_status = gserver->start_server();

            std::this_thread::sleep_for(std::chrono::seconds(1));
            gserver->status_server();
            
            // gserver = nullptr;
        } catch (std::exception &e) {
            status = error;
            std::cerr << "gsstop error: " << e.what() << std::endl;
        }
    }
    else if (!strcmp(task, TASK_GAME_SERVER_INSTALL) || !strcmp(task, TASK_GAME_SERVER_UPDATE)) {
        try {
            gserver = gslist.get_server(server_id);
            
            if (gserver == nullptr) {
                throw std::runtime_error("gslist.get_server error");
            }
            
            gserver->clear_cmd_output();
            result_status = gserver->update_server();
            // gserver = nullptr;
        } catch (std::exception &e) {
            status = error;
            std::cerr << "gsinst error: " << e.what() << std::endl;
        }
    }
    else if (! strcmp(task, TASK_GAME_SERVER_DELETE)) {
        try {
            gserver = gslist.get_server(server_id);
            
            if (gserver == nullptr) {
                throw std::runtime_error("gslist.get_server error");
            }
            
            gserver->clear_cmd_output();
            result_status = gserver->delete_server();
            
            // gserver = nullptr;
        } catch (std::exception &e) {
            status = error;
            std::cerr << "gsdelete error: " << e.what() << std::endl;
        }
    }
    else if (! strcmp(task, TASK_GAME_SERVER_MOVE)) {
        // Move game server to other ds
        gserver = gslist.get_server(server_id);
        gserver->clear_cmd_output();
    }
    else if (! strcmp(task, TASK_GAME_SERVER_EXECUTE)) {
        try {
            if (server_id != 0) {
                gserver = gslist.get_server(server_id);

                if (gserver == nullptr) {
                    throw std::runtime_error("gslist.get_server error");
                }

                gserver->clear_cmd_output();
                result_status = gserver->cmd_exec(cmd);
            } else {
                cmd_output = "";
                result_status = _exec(cmd);
            }
        } catch (std::exception &e) {
            status = error;
            std::cerr << "cmdexec error: " << e.what() << std::endl;
        }
    }
    else {
        // Unknown task
        result_status = ERROR_STATUS_INT;
    }

    if (result_status == SUCCESS_STATUS_INT) {
        status = success;
    }
    else {
        status = error;
    }

    try {
        Json::Value jdata;
        jdata["status"] = status;
        Gameap::Rest::put("/gdaemon_api/tasks/" + std::to_string(task_id), jdata);
    } catch (Gameap::Rest::RestapiException &exception) {
        std::cerr << "Error updating task status [to status code " + std::to_string(status) + "]: "
                  << exception.what()
                  << std::endl;
    }
}

// ---------------------------------------------------------------------

int Task::_exec(std::string cmd)
{
    std::vector<std::string> split_lines;
    boost::split(split_lines, cmd, boost::is_any_of("\n\r"));

    for (std::vector<std::string>::iterator itl = split_lines.begin(); itl != split_lines.end(); ++itl) {
        if (*itl == "") continue;
        _single_exec(*itl);
    }

    return 0;
}

// ---------------------------------------------------------------------

int Task::_single_exec(std::string cmd)
{
    std::cout << "CMD Exec: " << cmd << std::endl;
    _append_cmd_output(fs::current_path().string() + "# " + cmd);

    int exit_code = exec(cmd, [this](std::string line) {
        cmd_output.append(line + "\n");
    });

    return exit_code;
}

// ---------------------------------------------------------------------

void Task::_append_cmd_output(std::string line)
{
    cmd_output.append(line + "\n");
}

// ---------------------------------------------------------------------

std::string Task::get_output()
{
    if (server_id != 0 && gserver != nullptr) {
        std::string output;

        output = gserver->get_cmd_output();

        if (output.size()-cur_outpos > 0) {
            std::string output_part = output.substr(cur_outpos, output.size());
            cur_outpos += (output.size() - cur_outpos);

            return output_part;
        } else {
            return "";
        }
    }

    if (server_id == 0) {
        if (cmd_output.size()-cur_outpos > 0) {
            std::string output_part = cmd_output.substr(cur_outpos, cmd_output.size());
            cur_outpos += (cmd_output.size() - cur_outpos);
            return output_part;
        }
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
    ulong task_id = (*it)->get_id();
    
    std::vector<ulong>::iterator idit = std::find(taskids.begin(), taskids.end(), task_id);
    if (idit != taskids.end()) {
        taskids.erase(idit);
    }

    delete *it;
    tasklist.erase(it);

    return 0;
}

// ---------------------------------------------------------------------

int TaskList::update_list()
{
    Json::Value jvalue;

    try {
        jvalue = Gameap::Rest::get("/gdaemon_api/tasks?filter[status]=waiting&append=status_num");
    } catch (Gameap::Rest::RestapiException &exception) {
        // Try later
        std::cerr << exception.what() << std::endl;
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

// ---------------------------------------------------------------------

void TaskList::check_working_errors()
{
    Json::Value jvalue;

    try {
        jvalue = Gameap::Rest::get("/gdaemon_api/tasks?filter[status]=working&append=status_num");
    } catch (Gameap::Rest::RestapiException &exception) {
        // Try later
        std::cerr << exception.what() << std::endl;
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
                std::cerr << "Error updating task status [to status code " + std::to_string(error) + "]: "
                          << exception.what()
                          << std::endl;
            }
        }
    }
}

// ---------------------------------------------------------------------

void TaskList::insert(Task * task)
{
    tasklist.push_back(task);
    taskids.push_back(task->get_id());
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
