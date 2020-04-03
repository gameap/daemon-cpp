#include "gtest/gtest.h"

#include <fstream>
#include <sstream>
#include <queue>

#include <boost/filesystem.hpp>

#include "classes/game_servers_list.h"
#include "functions/restapi.h"

extern std::queue<std::function<int ()>> restapi_mock_get_token;
extern std::queue<std::function<Json::Value ()>> restapi_mock_get;
extern std::queue<std::function<void ()>> restapi_mock_post;
extern std::queue<std::function<void ()>> restapi_mock_put;
extern std::queue<std::function<void ()>> restapi_mock_patch;

namespace fs = boost::filesystem;

namespace GameAP {
    TEST(game_servers_list, get_server)
    {
        bool restCalled = false;

        restapi_mock_get.push([&]() -> Json::Value
        {
            restCalled = true;
            std::ifstream json_file;
            std::string json_file_path = std::string(TESTS_ROOT_DIR) + std::string("/rest_responses/servers.json");
            json_file.open(json_file_path, std::ios::in);

            std::string path = fs::current_path().string();

            std::stringstream str_stream;
            str_stream << json_file.rdbuf();
            std::string json = str_stream.str();

            Json::Value jvalue;
            Json::Reader jreader(Json::Features::strictMode());

            if (jreader.parse(json, jvalue, false)) {
                return jvalue;
            }

            return Json::Value();
        });

        restapi_mock_get.push([&]() -> Json::Value
          {
              restCalled = true;
              std::ifstream json_file;
              std::string json_file_path = std::string(TESTS_ROOT_DIR) + std::string("/rest_responses/server1.json");
              json_file.open(json_file_path, std::ios::in);

              std::string path = fs::current_path().string();

              std::stringstream str_stream;
              str_stream << json_file.rdbuf();
              std::string json = str_stream.str();

              Json::Value jvalue;
              Json::Reader jreader(Json::Features::strictMode());

              if (jreader.parse(json, jvalue, false)) {
                  return jvalue;
              }

              return Json::Value();
          });

        restapi_mock_get.push([&]() -> Json::Value
          {
              restCalled = true;
              std::ifstream json_file;
              std::string json_file_path = std::string(TESTS_ROOT_DIR) + std::string("/rest_responses/server2.json");
              json_file.open(json_file_path, std::ios::in);

              std::string path = fs::current_path().string();

              std::stringstream str_stream;
              str_stream << json_file.rdbuf();
              std::string json = str_stream.str();

              Json::Value jvalue;
              Json::Reader jreader(Json::Features::strictMode());

              if (jreader.parse(json, jvalue, false)) {
                  return jvalue;
              }

              return Json::Value();
          });

        GameServersList &gslist = GameServersList::getInstance();
        ASSERT_TRUE(restCalled);

        Server * server = gslist.get_server(1);
        ASSERT_EQ(1, server->id);
        ASSERT_EQ("09de788c-6fe1-474b-a704-c7a4163b4217", server->uuid);
        ASSERT_EQ("09de788c", server->uuid_short);
        ASSERT_TRUE(server->enabled);
        ASSERT_EQ(Server::SERVER_INSTALL_IN_PROCESS, server->installed);
        ASSERT_FALSE(server->blocked);
        ASSERT_EQ("test #1", server->name);
        ASSERT_EQ("1.3.3.7", server->ip);
        ASSERT_EQ(25000, server->connect_port);
        ASSERT_EQ(25001, server->query_port);
        ASSERT_EQ(25002, server->rcon_port);
        ASSERT_EQ("servers/minecraft", server->dir);
        ASSERT_EQ("gameap", server->user);
        ASSERT_EQ("java -jar minecraft.jar", server->start_command);
        ASSERT_EQ("", server->stop_command);
        ASSERT_EQ("", server->force_stop_command);
        ASSERT_EQ("", server->restart_command);
        ASSERT_FALSE(server->process_active);

        // Game
        ASSERT_EQ("minecraft", server->game.code);
        ASSERT_EQ("minecraft", server->game.start_code);
        ASSERT_EQ("Minecraft", server->game.name);
        ASSERT_EQ("minecraft", server->game.engine);
        ASSERT_EQ("1", server->game.engine_version);
        ASSERT_EQ(0, server->game.steam_app_id);
        ASSERT_EQ("", server->game.steam_app_set_config);
        ASSERT_EQ("", server->game.remote_repository);
        ASSERT_EQ("", server->game.local_repository);

        // Game mod
        ASSERT_EQ(4, server->game_mod.id);
        ASSERT_EQ("Default", server->game_mod.name);
        ASSERT_EQ("", server->game_mod.remote_repository);
        ASSERT_EQ("", server->game_mod.local_repository);
        ASSERT_EQ("java -jar minecraft.jar", server->game_mod.default_start_cmd_linux);
        ASSERT_EQ("java -jar minecraft.jar", server->game_mod.default_start_cmd_windows);

        server = gslist.get_server(2);
        ASSERT_EQ(2, server->id);
        ASSERT_EQ("9033c214-e08b-49fb-bf7c-6557f8c3633d", server->uuid);
        ASSERT_EQ("9033c214", server->uuid_short);
        ASSERT_TRUE(server->enabled);
        ASSERT_EQ(Server::SERVER_INSTALLED, server->installed);
        ASSERT_FALSE(server->blocked);
        ASSERT_EQ("test", server->name);
        ASSERT_EQ("1.3.3.7", server->ip);
        ASSERT_EQ(27015, server->connect_port);
        ASSERT_EQ(27015, server->query_port);
        ASSERT_EQ(27015, server->rcon_port);
        ASSERT_EQ("servers/test", server->dir);
        ASSERT_EQ("gameap", server->user);
        ASSERT_EQ("./hlds_run -game cstrike +ip {ip} +port {port} +map {default_map} +maxplayers {maxplayers} +sys_ticrate {fps}", server->start_command);
        ASSERT_EQ("", server->stop_command);
        ASSERT_EQ("", server->force_stop_command);
        ASSERT_EQ("", server->restart_command);
        ASSERT_FALSE(server->process_active);

        // Game
        ASSERT_EQ("valve", server->game.code);
        ASSERT_EQ("valve", server->game.start_code);
        ASSERT_EQ("Half-Life 1", server->game.name);
        ASSERT_EQ("GoldSource", server->game.engine);
        ASSERT_EQ("1", server->game.engine_version);
        ASSERT_EQ(90, server->game.steam_app_id);
        ASSERT_EQ("", server->game.steam_app_set_config);
        ASSERT_EQ("", server->game.remote_repository);
        ASSERT_EQ("", server->game.local_repository);

        // Game mod
        ASSERT_EQ(3, server->game_mod.id);
        ASSERT_EQ("ReHLDS", server->game_mod.name);
        ASSERT_EQ("http://files.gameap.ru/half-life/rehlds-amxx-reunion.tar.xz", server->game_mod.remote_repository);
        ASSERT_EQ("", server->game_mod.local_repository);
        ASSERT_EQ("./hlds_run -game cstrike +ip {ip} +port {port} +map {default_map} +maxplayers {maxplayers} +sys_ticrate {fps}", server->game_mod.default_start_cmd_linux);
        ASSERT_EQ("hlds.exe -console -game cstrike +ip {ip} +port {port} +map {default_map} +maxplayers {maxplayers} +sys_ticrate {fps}", server->game_mod.default_start_cmd_windows);

    }

    TEST(game_servers_list, get_server_null)
    {
        bool restCalled = false;
        restapi_mock_get.push([&]() -> Json::Value
        {
            return Json::Value();
        });

        GameServersList &gslist = GameServersList::getInstance();
        ASSERT_EQ(nullptr, gslist.get_server(99999));
    }
}