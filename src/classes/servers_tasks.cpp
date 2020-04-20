#include "servers_tasks.h"

#include <json/json.h>

#include "functions/restapi.h"
#include "functions/gstring.h"

#include "consts.h"
#include "state.h"
#include "log.h"

using namespace GameAP;

void ServersTasks::run_next()
{
    auto task = this->tasks.top();

    time_t current_time = time(nullptr);

    if (this->last_sync.find(task->id) == this->last_sync.end()
        || this->last_sync[task->id] < (current_time - this->cache_ttl)
    ) {
        this->sync_from_api(task);
        this->last_sync.insert(
                std::pair<unsigned int, time_t>(task->id, current_time)
        );
    }

    if (current_time <= task->execute_date) {
        return;
    }

    this->tasks.pop();

    if (task->repeat != 0 && task->counter >= task->repeat) {
        return;
    }

    if (task->status == ServerTask::WAITING) {
        this->start(task);
    } else if (task->status == ServerTask::WORKING) {
        this->proceed(task);
    }

    if (task->status == ServerTask::SUCCESS || task->status == ServerTask::FAIL) {
        // The task is completed

        if (task->repeat == 0 || task->counter < task->repeat) {
            // Repeat task
            task->status = ServerTask::WAITING;
            this->tasks.push(task);
        } else {
            this->exists_tasks.erase(task->id);
        }

        Cmd* game_server_cmd = this->active_cmds[task->id];
        
        game_server_cmd->destroy();
        destroy_shared_map_memory((void *)game_server_cmd, sizeof(*game_server_cmd));

        this->active_cmds.erase(task->id);
        this->last_sync.erase(task->id);

        if (this->cmd_processes.find(task->id) != this->cmd_processes.end()) {
            this->cmd_processes.erase(task->id);
        }

        return;
    }

    // The task is not completed yet
    // Next proceed task in 30 seconds
    task->execute_date = time(nullptr) + 30;
    this->tasks.push(task);
}

void ServersTasks::start(std::shared_ptr<ServerTask> &task)
{
    task->status = ServerTask::WORKING;

    GameServerCmd* game_server_cmd = (GameServerCmd*)shared_map_memory(sizeof(GameServerCmd));
    new (game_server_cmd) GameServerCmd(GameServerCmd::RESTART, task->server_id);

    this->active_cmds.insert(
        std::pair<unsigned int, GameServerCmd*>(task->id, game_server_cmd)
    );

    this->before_cmd(task);

    // TODO: Couroutines here
    pid_t child_pid = run_process([=]() {
        GAMEAP_LOG_VERBOSE << "Executing cmd";
        game_server_cmd->execute();
    });

    this->cmd_processes.insert(
            std::pair<unsigned int, pid_t>(task->id, child_pid)
    );
}

void ServersTasks::proceed(std::shared_ptr<ServerTask> &task)
{
    if (this->active_cmds.find(task->id) == this->active_cmds.end()) {
        task->status = ServerTask::FAIL;
        return;
    }

    GameServerCmd* game_server_cmd = this->active_cmds[task->id];

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
                ? ServerTask::SUCCESS
                : ServerTask::FAIL;

        if (child_status != PROCESS_SUCCESS) {
            GAMEAP_LOG_ERROR << "Child process failure";
        }

        this->after_cmd(task);

        if (task->status == ServerTask::FAIL) {
            std::string output;
            game_server_cmd->output(&output);
            this->save_fail_to_api(task, output);
        }

        task->counter++;
        task->execute_date = time(nullptr) + task->repeat_period;

        this->sync_to_api(task);

        return;
    }
}

void ServersTasks::before_cmd(std::shared_ptr<ServerTask> &task)
{
    GameServersList& gslist = GameServersList::getInstance();

    switch (task->command) {
        case GameServerCmd::INSTALL:
        case GameServerCmd::UPDATE:
            gslist.set_install_status(task->server_id, Server::SERVER_INSTALL_IN_PROCESS);
            break;
        default:
            // Do nothing
            break;
    }
}

void ServersTasks::after_cmd(std::shared_ptr<ServerTask> &task)
{
    GameServersList& gslist = GameServersList::getInstance();

    switch (task->command) {
        case GameServerCmd::INSTALL:
        case GameServerCmd::UPDATE:

            if (task->status == ServerTask::SUCCESS) {
                gslist.set_install_status(task->server_id, Server::SERVER_INSTALLED);
            } else if (task->status == ServerTask::FAIL) {
                gslist.set_install_status(task->server_id, Server::SERVER_NOT_INSTALLED);
            }

            break;
        case GameServerCmd::DELETE:
            if (task->status == ServerTask::SUCCESS) {
                gslist.set_install_status(task->server_id, Server::SERVER_NOT_INSTALLED);
            }
            break;
        default:
            // Do nothing
            break;
    }
}

/**
 * @return true if there are no tasks to run. Otherwise false
 */
bool ServersTasks::empty()
{
    if (this->tasks.empty()) {
        return true;
    }

    return (this->tasks.top()->execute_date > time(nullptr));
}

void ServersTasks::update()
{
    Json::Value jvalue;

    try {
        GAMEAP_LOG_VERBOSE << "Sync servers tasks from API...";
        jvalue = Rest::get("/gdaemon_api/servers_tasks");
    } catch (Rest::RestapiException &exception) {
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

void ServersTasks::sync_from_api(std::shared_ptr<ServerTask> &task)
{
    Json::Value jtask;

    try {
        GAMEAP_LOG_VERBOSE << "Syncing server task [" << task->id << "] from API...";
        jtask = Rest::get("/gdaemon_api/servers_tasks/" + std::to_string(task->id));
    } catch (Rest::RestapiException &exception) {
        // Try later
        GAMEAP_LOG_ERROR << exception.what();
        return;
    }

    task->command       = this->convert_command(jtask["command"].asString());
    task->server_id     = jtask["server_id"].asUInt();
    task->repeat        = static_cast<unsigned short>(jtask["repeat"].asUInt());
    task->repeat_period = jtask["repeat_period"].asUInt();
    task->counter       = jtask["counter"].asUInt();
    task->execute_date  = human_to_timestamp(jtask["execute_date"].asString());
    task->payload       = jtask["payload"].asString();
}

void ServersTasks::sync_to_api(std::shared_ptr<ServerTask> &task) {
    Json::Value jtask;

    jtask["counter"] = task->counter;
    jtask["repeat"] = task->repeat;

    State &state = State::getInstance();
    time_t time_diff = std::stoi(state.get(STATE_PANEL_TIMEDIFF));

    std::time_t time = task->execute_date - time_diff;
    std::tm *ptm = std::gmtime(&time);
    char buffer[32];
    std::strftime(buffer, 32, "%F %T", ptm);

    jtask["execute_date"] = buffer;

    try {
        GAMEAP_LOG_VERBOSE << "Updating server task [" << task->id << "] on API...";
        Rest::put("/gdaemon_api/servers_tasks/" + std::to_string(task->id), jtask);
    } catch (Rest::RestapiException &exception) {
        GAMEAP_LOG_ERROR << "Error sync server tasks: " << exception.what();
    }
}

void ServersTasks::save_fail_to_api(std::shared_ptr<ServerTask> &task, const std::string& output)
{
    Json::Value jfail;
    jfail["output"] = output;

    try {
        GAMEAP_LOG_VERBOSE << "Saving server task fail information [" << task->id << "] on API...";
        Rest::put("/gdaemon_api/servers_tasks/" + std::to_string(task->id) + "/fail", jfail);
    } catch (Rest::RestapiException &exception) {
        GAMEAP_LOG_ERROR << "Error sync server tasks: " << exception.what();
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
    } else if (command == "update" || command == "install") {
        return GameServerCmd::UPDATE;
    } else if (command == "reinstall") {
        return GameServerCmd::REINSTALL;
    } else {
        return 0;
    }
}