#include "gdaemon_tasks.h"

#include <thread>

#include <json/json.h>
#include <commands/dedicated_server_cmd.h>

#include "functions/restapi.h"
#include "log.h"

using namespace GameAP;

void GdaemonTasks::run_next()
{
    auto task = this->tasks.front();
    this->tasks.pop();

    if (task->run_aft_id > 0 && this->exists_tasks.find(task->run_aft_id) != this->exists_tasks.end()) {
        // Waiting for another task to complete
        this->tasks.push(task);
        return;
    }

    if (task->status == GdaemonTask::WAITING) {
        GAMEAP_LOG_VERBOSE << "Starting task [" << task->id << "]";
        this->start(task);
    } else if (task->status == GdaemonTask::WORKING) {
        GAMEAP_LOG_VERBOSE << "Proceed task [" << task->id << "]";
        this->proceed(task);
    }

    if (task->status != GdaemonTask::ERROR
        && task->status != GdaemonTask::SUCCESS
        && task->status != GdaemonTask::CANCELED
    ) {
        // The task is not completed yet
        this->tasks.push(task);
    } else {
        // The task is completed
        this->active_cmds.erase(task->id);
        this->exists_tasks.erase(task->id);
    }
}

bool GdaemonTasks::empty()
{
    return this->tasks.empty();
}

void GdaemonTasks::update()
{
    Json::Value jvalue;

    try {
        GAMEAP_LOG_VERBOSE << "Sync gdaemon tasks list from API...";
        jvalue = Rest::get("/gdaemon_api/tasks?filter[status]=waiting&append=status_num");
    } catch (Rest::RestapiException &exception) {
        // Try later
        GAMEAP_LOG_ERROR << exception.what();
        return;
    }

    for (auto jtask: jvalue) {
        if (this->exists_tasks.find(jtask["id"].asUInt()) != this->exists_tasks.end()) {
            continue;
        }

        if (jtask["task"].asString() == "") {
            GAMEAP_LOG_WARNING
                        << "Empty task. TaskID: " << jtask["id"].asUInt()
                        << " StatusNum: " << jtask["status_num"].asUInt();
            continue;
        }

        if (jtask["status_num"].asUInt() != TASK_WAITING) {
            GAMEAP_LOG_WARNING
                        << "Invalid task status num. TaskID: " << jtask["id"].asUInt()
                        << " StatusNum: " << jtask["status_num"].asUInt();
            continue;
        }

        GameServersList& gslist = GameServersList::getInstance();
        Server* server = gslist.get_server(jtask["server_id"].asLargestUInt());

        if (server == nullptr) {
            GAMEAP_LOG_ERROR << "Invalid game server. ID: " << jtask["server_id"].asLargestUInt();
            continue;
        }

        auto task = std::make_shared<GdaemonTask>(GdaemonTask{
                jtask["id"].asUInt(),
                static_cast<unsigned int>(jtask["run_aft_id"].asLargestUInt()),
                static_cast<unsigned int>(jtask["server_id"].asLargestUInt()),
                server,
                jtask["task"].asString(),
                jtask["cmd"].isNull() ? "" : jtask["cmd"].asCString(),
                GdaemonTask::WAITING // (ushort)(*itr)["status"].asUInt()
        });

        this->exists_tasks.insert(jtask["id"].asUInt());

        this->tasks.push(task);
    }
}

void GdaemonTasks::check_after_crash()
{
    Json::Value jvalue;

    try {
        GAMEAP_LOG_VERBOSE << "Get working gdaemon tasks list from API...";
        jvalue = Rest::get("/gdaemon_api/tasks?filter[status]=working&append=status_num");
    } catch (Rest::RestapiException &exception) {
        // Try later
        GAMEAP_LOG_ERROR << exception.what();
        return;
    }

    for (auto jtask: jvalue) {
        if (this->exists_tasks.find(jtask["id"].asUInt()) != this->exists_tasks.end()) {
            continue;
        }

        if (jtask["task"].asString() == "") {
            GAMEAP_LOG_WARNING
                        << "Empty task. TaskID: " << jtask["id"].asUInt()
                        << " StatusNum: " << jtask["status_num"].asUInt();
            continue;
        }

        if (jtask["status_num"].asUInt() != TASK_WORKING) {
            GAMEAP_LOG_WARNING
                        << "Invalid task status num. TaskID: " << jtask["id"].asUInt()
                        << " StatusNum: " << jtask["status_num"].asUInt();
            continue;
        }

        GameServersList& gslist = GameServersList::getInstance();
        Server* server = gslist.get_server(jtask["server_id"].asLargestUInt());

        if (server == nullptr) {
            GAMEAP_LOG_ERROR << "Invalid game server. ID: " << jtask["server_id"].asLargestUInt();
            continue;
        }

        auto task = std::make_shared<GdaemonTask>(GdaemonTask{
                jtask["id"].asUInt(),
                static_cast<unsigned int>(jtask["run_aft_id"].asLargestUInt()),
                static_cast<unsigned int>(jtask["server_id"].asLargestUInt()),
                server,
                jtask["task"].asString(),
                jtask["cmd"].isNull() ? "" : jtask["cmd"].asCString(),
                GdaemonTask::WAITING // Change status to WAITING
        });

        this->exists_tasks.insert(jtask["id"].asUInt());

        this->tasks.push(task);
    }
}

void GdaemonTasks::api_update_status(std::shared_ptr<GdaemonTask> &task)
{
    try {
        Json::Value jdata;
        jdata["status"] = task->status;
        GAMEAP_LOG_VERBOSE << "Updating tasks status...";
        Rest::put("/gdaemon_api/tasks/" + std::to_string(task->id), jdata);
    } catch (Rest::RestapiException &exception) {
        GAMEAP_LOG_ERROR << "Error updating task status [to status code " + std::to_string(task->status) + "]: "
                         << exception.what();

        return;
    }
}

void GdaemonTasks::api_append_output(std::shared_ptr<GdaemonTask> &task, std::string &output)
{
    try {
        Json::Value jdata;
        jdata["output"] = output;

        GAMEAP_LOG_VERBOSE << "Appending gdaemon task output...";
        Rest::put("/gdaemon_api/tasks/" + std::to_string(task->id) + "/output", jdata);
    } catch (Rest::RestapiException &exception) {
        GAMEAP_LOG_ERROR << "Output updating error: " << exception.what();

        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

void GdaemonTasks::start(std::shared_ptr<GdaemonTask> &task)
{
    Cmd* cmd;

    if (! strcmp(task->task.c_str(), GAME_SERVER_START)) {
        cmd = (GameServerCmd*)shared_map_memory(sizeof(GameServerCmd));
        new (cmd) GameServerCmd(GameServerCmd::START, task->server_id);
    }
    else if (! strcmp(task->task.c_str(), GAME_SERVER_STOP)) {
        cmd = (GameServerCmd*)shared_map_memory(sizeof(GameServerCmd));
        new (cmd) GameServerCmd(GameServerCmd::STOP, task->server_id);
    }
    else if (! strcmp(task->task.c_str(), GAME_SERVER_RESTART)) {
        cmd = (GameServerCmd*)shared_map_memory(sizeof(GameServerCmd));
        new (cmd) GameServerCmd(GameServerCmd::RESTART, task->server_id);
    }
    else if (! strcmp(task->task.c_str(), GAME_SERVER_INSTALL)) {
        cmd = (GameServerCmd*)shared_map_memory(sizeof(GameServerCmd));
        new (cmd) GameServerCmd(GameServerCmd::INSTALL, task->server_id);
    }
    else if (! strcmp(task->task.c_str(), GAME_SERVER_UPDATE) || ! strcmp(task->task.c_str(), GAME_SERVER_INSTALL)) {
        cmd = (GameServerCmd*)shared_map_memory(sizeof(GameServerCmd));
        new (cmd) GameServerCmd(GameServerCmd::UPDATE, task->server_id);
    }
    else if (! strcmp(task->task.c_str(), GAME_SERVER_DELETE)) {
        cmd = (GameServerCmd*)shared_map_memory(sizeof(GameServerCmd));
        new (cmd) GameServerCmd(GameServerCmd::DELETE, task->server_id);
    }
    else if (! strcmp(task->task.c_str(), GAME_SERVER_EXECUTE)) {
        cmd = new DedicatedServerCmd{DedicatedServerCmd::CMD_EXECUTE};
        cmd->set_option(DedicatedServerCmd::OPTION_SHELL_COMMAND, task->cmd);
    }
    else {
        // Unknown task
        std::string output = "Unknown task";
        this->api_append_output(task, output);
    }

    if (cmd != nullptr) {
        this->active_cmds.insert(
            std::pair<unsigned int, Cmd*>(task->id, cmd)
        );

        // TODO: Couroutines here
        pid_t child_pid = run_process([=]() {
            GAMEAP_LOG_VERBOSE << "Executing cmd";
            cmd->execute();
        });

        this->cmd_processes.insert(
            std::pair<unsigned int, pid_t>(task->id, child_pid)
        );

        task->status = GdaemonTask::WORKING;
        this->api_update_status(task);
    } else {
        task->status = GdaemonTask::ERROR;

        std::string output = "Empty cmd";
        this->api_append_output(task, output);
        this->api_update_status(task);
    }
}

void GdaemonTasks::proceed(std::shared_ptr<GdaemonTask> &task)
{
    if (this->active_cmds.find(task->id) == this->active_cmds.end()) {
        // Fail
        return;
    }

    Cmd* game_server_cmd = this->active_cmds[task->id];

    std::string output;
    game_server_cmd->output(&output);

    int child_status;
    auto cmd_process_itr = this->cmd_processes.find(task->id);

    if (cmd_process_itr == this->cmd_processes.end()) {
        child_status = PROCESS_FAIL;
    } else {
        child_status = child_process_status(cmd_process_itr->second);
    }

    bool cmd_completed = game_server_cmd->is_complete()
                         || (child_status == PROCESS_SUCCESS || child_status == PROCESS_FAIL);

    if (cmd_completed) {
        task->status = (game_server_cmd->result() && child_status == PROCESS_SUCCESS)
                   ? GdaemonTask::SUCCESS
                   : GdaemonTask::ERROR;

        destroy_shared_map_memory((void *)game_server_cmd, sizeof(*game_server_cmd));
        this->active_cmds.erase(task->id);

        this->api_update_status(task);
    }

    if (!output.empty()) {
        this->api_append_output(task, output);
    }
}

GdaemonTasksStats GdaemonTasks::stats()
{
    return GdaemonTasksStats{
            static_cast<unsigned int>(this->active_cmds.size()),
            static_cast<unsigned int>(this->tasks.size()),
    };
}