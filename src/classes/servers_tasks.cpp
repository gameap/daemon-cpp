#include "servers_tasks.h"

#include <memory>
#include <json/json.h>

#include "classes/game_servers_list.h"
#include "functions/restapi.h"
#include "functions/gstring.h"
#include "log.h"

#include "models/server.h"
#include "game_server_cmd.h"

using namespace GameAP;

void ServersTasks::run_next()
{
    auto task = this->tasks.top();
    time_t current_time = time(nullptr);

    if (current_time >= task->execute_date) {
        return;
    }

    this->tasks.pop();

    if (task->status == ServerTask::WAITING) {
        this->start(task);
    } else if (task->status == ServerTask::WORKING) {
        this->proceed(task);
    }

    if (task->status != ServerTask::SUCCESS && task->status != ServerTask::FAIL) {
        this->tasks.push(task);
    }
}

void ServersTasks::start(std::shared_ptr<ServerTask> &task)
{
    task->status = ServerTask::WORKING;

    std::shared_ptr<GameServerCmd> game_server_cmd = std::make_shared<GameServerCmd>(task->command, task->server_id);

    this->active_cmds.insert(
        std::pair<unsigned int, std::shared_ptr<GameServerCmd>>(task->id, game_server_cmd)
    );

    std::thread server_cmd_thread([&]() {
        game_server_cmd->execute();
    });
}

void ServersTasks::proceed(std::shared_ptr<ServerTask> &task)
{
    if (this->active_cmds.find(task->id) == this->active_cmds.end()) {
        task->status = ServerTask::FAIL;
        return;
    }

    std::shared_ptr<GameServerCmd> game_server_cmd = this->active_cmds[task->id];

    if (game_server_cmd->is_complete()) {
        task->status = game_server_cmd->result()
                ? ServerTask::SUCCESS
                : ServerTask::FAIL;

        std::string output;
        game_server_cmd->output(&output);

        // TODO: Update api

        return;
    }
}

bool ServersTasks::empty()
{
    return this->tasks.empty();
}

void ServersTasks::update()
{
    Json::Value jvalue;

    try {
        jvalue = Gameap::Rest::get("/gdaemon_api/servers_tasks");
    } catch (Gameap::Rest::RestapiException &exception) {
        // Try later
        GAMEAP_LOG_ERROR << exception.what();
        return;
    }

    for(auto jtask: jvalue) {
        if (this->exists_tasks.find(jtask["id"].asUInt()) != this->exists_tasks.end()) {
            continue;
        }

        this->exists_tasks.insert(jtask["id"].asUInt());

        auto task = std::make_shared<ServerTask>(ServerTask{
            ServerTask::WAITING,
            jtask["id"].asUInt(),
            this->convert_command(jtask["command"].asString()),
            jtask["server_id"].asUInt(),
            static_cast<unsigned short>(jtask["repeat"].asUInt()),
            jtask["repeat_period"].asUInt(),
            jtask["counter"].asUInt(),
            human_to_timestamp(jtask["execute_date"].asString()),
            jtask["payload"].asString()
        });

        this->tasks.push(task);
    }
}

unsigned char ServersTasks::convert_command(const std::string& command)
{
    if (command == "restart") {
        return GameServerCmd::RESTART;
    } else if (command == "start") {
        return GameServerCmd::START;
    } else if (command == "stop") {
        return GameServerCmd::STOP;
    } else if (command == "update") {
        return GameServerCmd::UPDATE;
    } else if (command == "reinstall") {
        return GameServerCmd::REINSTALL;
    } else {
        return 0;
    }
}