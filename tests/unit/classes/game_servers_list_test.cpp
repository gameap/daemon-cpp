#include "gtest/gtest.h"

#include <queue>

#include "classes/game_servers_list.h"
#include "functions/restapi.h"

extern std::queue<std::function<int ()>> restapi_mock_get_token;
extern std::queue<std::function<Json::Value ()>> restapi_mock_get;
extern std::queue<std::function<void ()>> restapi_mock_post;
extern std::queue<std::function<void ()>> restapi_mock_put;
extern std::queue<std::function<void ()>> restapi_mock_patch;

namespace GameAP {
    TEST(game_servers_list, get_server)
    {
        bool restCalled = false;

        restapi_mock_get_token.push([&]() -> int
        {
            restCalled = true;
            throw Rest::RestapiException("RestClient error");
        });
    }

    TEST(game_servers_list, get_server_null)
    {
        GameServersList &gslist = GameServersList::getInstance();
    }
}