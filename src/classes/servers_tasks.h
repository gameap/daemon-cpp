#ifndef GDAEMON_SERVERS_TASKS_H
#define GDAEMON_SERVERS_TASKS_H

#include <queue>
#include <string>
#include <memory>
#include <unordered_set>
#include <unordered_map>

#include "models/server_task.h"
#include "game_server_cmd.h"

namespace GameAP {
    struct CompareTask {
        bool operator()(std::shared_ptr<ServerTask> const& t1, std::shared_ptr<ServerTask> const& t2)
        {
            return t1->execute_date > t2->execute_date;
        }
    };

    class ServersTasks {
        public:
            static constexpr auto TASK_START     = "start";
            static constexpr auto TASK_STOP      = "stop";
            static constexpr auto TASK_RESTART   = "restart";
            static constexpr auto TASK_UPDATE    = "update";
            static constexpr auto TASK_REINSTALL = "reinstall";

            static ServersTasks& getInstance() {
                static ServersTasks instance;
                return instance;
            }

            /**
             * Run next queue task
             */
            void run_next();

            /**
             * Return if queue empty
             * @return bool
             */
            bool empty();

            /**
             * Update tasks list from api
             */
            void update();
        private:
            std::priority_queue<std::shared_ptr<ServerTask>, std::vector<std::shared_ptr<ServerTask>>, CompareTask> tasks;
            std::unordered_set<unsigned int> exists_tasks;
            std::unordered_map<unsigned int, std::shared_ptr<GameServerCmd>> active_cmds;

            void start(std::shared_ptr<ServerTask> &task);
            void proceed(std::shared_ptr<ServerTask> &task);

            unsigned char convert_command(const std::string& command);

            ServersTasks() {}
            ServersTasks( const ServersTasks&);
            ServersTasks& operator=( ServersTasks& );
    };
}

#endif //GDAEMON_SERVERS_TASKS_H
