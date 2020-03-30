#include "servers_tasks.h"

#include <memory>
#include <json/json.h>

#include "classes/game_servers_list.h"
#include "functions/restapi.h"
#include "functions/gstring.h"
#include "log.h"

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

    GameServersList& gslist = GameServersList::getInstance();

    if (task->task == TASK_START) {
        return;
    }
}

void ServersTasks::proceed(std::shared_ptr<ServerTask> &task)
{
    task->status = ServerTask::SUCCESS;
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
            jtask["task"].asString(),
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