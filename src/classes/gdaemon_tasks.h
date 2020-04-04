#ifndef GDAEMON_GDAEMON_TASKS_H
#define GDAEMON_GDAEMON_TASKS_H

#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <memory>

#include <boost/thread.hpp>

#include "commands/game_server_cmd.h"
#include "models/gdaemon_task.h"

#define TASK_WAITING    1
#define TASK_WORKING    2
#define TASK_ERROR      3
#define TASK_SUCCESS    4
#define TASK_CANCELED   5

namespace GameAP {
    class GdaemonTasks {
        public:
            static constexpr auto GAME_SERVER_START         = "gsstart";
            static constexpr auto GAME_SERVER_PAUSE         = "gspause"; // NOT Implemented
            static constexpr auto GAME_SERVER_STOP          = "gsstop";
            static constexpr auto GAME_SERVER_KILL          = "gskill"; // NOT Implemented
            static constexpr auto GAME_SERVER_RESTART       = "gsrest";
            static constexpr auto GAME_SERVER_INSTALL       = "gsinst";
            static constexpr auto GAME_SERVER_REINSTALL     = "gsreinst"; // NOT Implemented
            static constexpr auto GAME_SERVER_UPDATE        = "gsupd";
            static constexpr auto GAME_SERVER_DELETE        = "gsdel";
            static constexpr auto GAME_SERVER_MOVE          = "gsmove";
            static constexpr auto GAME_SERVER_EXECUTE       = "cmdexec";

            static GdaemonTasks& getInstance() {
                static GdaemonTasks instance;
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
            std::queue<std::shared_ptr<GdaemonTask>> tasks;
            std::unordered_set<unsigned int> exists_tasks;
            std::unordered_map<unsigned int, std::shared_ptr<Cmd>> active_cmds;

            // TODO: Remove after replace to coroutines
            boost::thread_group cmds_threads;

            void start(std::shared_ptr<GdaemonTask> &task);
            void proceed(std::shared_ptr<GdaemonTask> &task);

            // API
            void api_update_status(std::shared_ptr<GdaemonTask> &task);
            void api_append_output(std::shared_ptr<GdaemonTask> &task, std::string &output);

            GdaemonTasks() {}
            GdaemonTasks( const GdaemonTasks&);
            GdaemonTasks& operator=( GdaemonTasks& );
    };
}

#endif //GDAEMON_GDAEMON_TASKS_H
