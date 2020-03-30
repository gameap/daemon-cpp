#include "config.h"
#include "gtest/gtest.h"
#include "log.h"
#include "functions/restapi.h"
#include "classes/task_list.h"

#include <queue>

#include <plog/Appenders/ConsoleAppender.h>

#ifdef __GNUC__
#include <thread>
#endif

int run_daemon();

extern std::queue<std::function<int ()>> restapi_mock_get_token;
extern std::queue<std::function<Json::Value ()>> restapi_mock_get;
extern std::queue<std::function<void ()>> restapi_mock_post;
extern std::queue<std::function<void ()>> restapi_mock_put;
extern std::queue<std::function<void ()>> restapi_mock_patch;

namespace GameAP {
    TEST(tasks, run_task_test)
    {
        restapi_mock_get.push([&]() -> Json::Value
        {
            Json::Value task_json;
            Json::Value return_json;

            // Valid task #1
            task_json["id"] = 1;
            task_json["task"] = TASK_GAME_SERVER_START;
            task_json["status_num"] = TASK_WAITING;
            return_json.append(task_json);

            // Valid task #2
            task_json["id"] = 2;
            task_json["task"] = TASK_GAME_SERVER_INSTALL;
            task_json["status_num"] = TASK_WAITING;
            return_json.append(task_json);

            // Working task. Must be skipped
            task_json["id"] = 1;
            task_json["task"] = TASK_GAME_SERVER_STOP;
            task_json["status_num"] = TASK_WORKING;
            return_json.append(task_json);

            // Dublicate ID. Must be skipped
            task_json["id"] = 1;
            task_json["task"] = TASK_GAME_SERVER_STOP;
            task_json["status_num"] = TASK_WAITING;
            return_json.append(task_json);

            return return_json;
        });

        TaskList& tasks = TaskList::getInstance();
        tasks.update_list();

        ASSERT_EQ(2, tasks.count());
    }

    TEST(tasks, insert_delete_test)
    {
        Task * task1 = new Task(
            1,
            1,
            1,
            TASK_GAME_SERVER_INSTALL,
            "",
            "",
            TASK_WAITING
        );

        Task * task2 = new Task(
                2,
                2,
                2,
                TASK_GAME_SERVER_INSTALL,
                "",
                "",
                TASK_WAITING
        );

        TaskList& tasks = TaskList::getInstance();

        tasks.insert(task1);
        ASSERT_EQ(1, tasks.count());

        tasks.insert(task2);
        ASSERT_EQ(2, tasks.count());

//        tasks.delete_task(task2);
//        ASSERT_EQ(1, tasks.count());
//
//        tasks.delete_task(task2);
//        ASSERT_EQ(1, tasks.count());
//
//        tasks.delete_task(task1);
//        ASSERT_EQ(0, tasks.count());
    }
}