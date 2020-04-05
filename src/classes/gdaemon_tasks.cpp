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
        return;
    }

    if (task->status == GdaemonTask::WAITING) {
        this->start(task);
    } else if (task->status == GdaemonTask::WORKING) {
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
            continue;
        }

        if (jtask["status_num"].asUInt() != TASK_WAITING) {
            continue;
        }

        GameServersList& gslist = GameServersList::getInstance();
        Server* server = gslist.get_server(jtask["server_id"].asLargestUInt());

        auto task = std::make_shared<GdaemonTask>(GdaemonTask{
                jtask["id"].asUInt(),
                jtask["run_aft_id"].asLargestUInt(),
                jtask["server_id"].asLargestUInt(),
                server,
                jtask["task"].asString(),
                jtask["cmd"].isNull() ? "" : jtask["cmd"].asCString(),
                GdaemonTask::WAITING // (ushort)(*itr)["status"].asUInt()
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
    std::shared_ptr<Cmd> cmd;

    if (! strcmp(task->task.c_str(), GAME_SERVER_START)) {
        cmd = std::make_shared<GameServerCmd>(GameServerCmd{GameServerCmd::START, task->server_id});
    }
    else if (! strcmp(task->task.c_str(), GAME_SERVER_STOP)) {
        cmd = std::make_shared<GameServerCmd>(GameServerCmd{GameServerCmd::STOP, task->server_id});
    }
    else if (! strcmp(task->task.c_str(), GAME_SERVER_RESTART)) {
        cmd = std::make_shared<GameServerCmd>(GameServerCmd{GameServerCmd::RESTART, task->server_id});
    }
    else if (! strcmp(task->task.c_str(), GAME_SERVER_INSTALL)) {
        cmd = std::make_shared<GameServerCmd>(GameServerCmd{GameServerCmd::INSTALL, task->server_id});
    }
    else if (! strcmp(task->task.c_str(), GAME_SERVER_UPDATE) || ! strcmp(task->task.c_str(), GAME_SERVER_INSTALL)) {
        cmd = std::make_shared<GameServerCmd>(GameServerCmd{GameServerCmd::UPDATE, task->server_id});
    }
    else if (! strcmp(task->task.c_str(), GAME_SERVER_DELETE)) {
        cmd = std::make_shared<GameServerCmd>(GameServerCmd{GameServerCmd::DELETE, task->server_id});
    }
    else if (! strcmp(task->task.c_str(), GAME_SERVER_EXECUTE)) {
        cmd = std::make_shared<DedicatedServerCmd>(DedicatedServerCmd{DedicatedServerCmd::CMD_EXECUTE});
        cmd->set_option(DedicatedServerCmd::OPTION_SHELL_COMMAND, task->cmd);
    }
    else {
        // Unknown task
        std::string output = "Unknown task";
        this->api_append_output(task, output);
    }

    if (cmd != nullptr) {
        this->active_cmds.insert(
            std::pair<unsigned int, std::shared_ptr<Cmd>>(task->id, cmd)
        );

        // TODO: Couroutines here
        this->cmds_threads.create_thread([=]() {
            cmd->execute();
        });

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

    std::shared_ptr<Cmd> game_server_cmd = this->active_cmds[task->id];

    if (game_server_cmd->is_complete()) {
        task->status = game_server_cmd->result()
                   ? GdaemonTask::SUCCESS
                   : GdaemonTask::ERROR;

        this->active_cmds.erase(task->id);

        this->api_update_status(task);
    }

    std::string output;
    game_server_cmd->output(&output);

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