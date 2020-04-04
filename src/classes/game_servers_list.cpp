#include "consts.h"
#include "config.h"
#include "state.h"
#include "log.h"

#include "game_servers_list.h"

#include "functions/restapi.h"
#include "functions/gstring.h"

#include "commands/game_server_cmd.h"
#include "models/server.h"

using namespace GameAP;

int GameServersList::update_list()
{
    if (this->last_updated > (time(nullptr) - this->cache_ttl)) {
        // Cached
        return SUCCESS_STATUS_INT;
    }

    Json::Value jvalue;

    try {
        jvalue = Rest::get("/gdaemon_api/servers?fields[servers]=id");
    } catch (Rest::RestapiException &exception) {
        // Try later
        GAMEAP_LOG_ERROR << exception.what();
        return ERROR_STATUS_INT;
    }

    for (auto jserver: jvalue) {
        ulong server_id = getJsonUInt(jserver["id"]);

        if (servers_list.find(server_id) == servers_list.end()) {

            // TODO: Improve
            std::shared_ptr<Server> server = std::make_shared<Server>(Server{
                jserver["id"].asUInt(),
                jserver["enabled"].asBool(),
                static_cast<Server::install_status>(jserver["installed"].asUInt()),
                jserver["blocked"].asBool(),
                jserver["name"].asString(),
                jserver["uuid"].asString(),
                jserver["uuid_short"].asString(),
                Game{
                    jserver["game_id"].asString(),
                    "",
                    "",
                    "",
                    "",
                    0,
                    "",
                    "",
                    ""
                },
                GameMod{
                    jserver["game_mod_id"].asUInt(),
                    "",
                    "",
                    "",
                    "",
                    ""
                },
                jserver["server_ip"].asString(),
                static_cast<unsigned short>(jserver["server_port"].asUInt()),
                static_cast<unsigned short>(jserver["query_port"].asUInt()),
                static_cast<unsigned short>(jserver["rcon_port"].asUInt()),
                jserver["dir"].asString(),
                jserver["su_user"].asString(),
                jserver["start_command"].asString(),
                jserver["stop_command"].asString(),
                jserver["force_stop_command"].asString(),
                jserver["restart_command"].asString(),
                jserver["process_active"].asBool(),
                0, // jserver["last_process_check"].asUInt(),
                std::map<std::string, std::string>()
            });

            try {
                servers_list.insert(
                        std::pair<ulong, std::shared_ptr<Server>>(server_id, server)
                );
            } catch (std::exception &e) {
                GAMEAP_LOG_ERROR << "Server #" << server_id << " insert error: " << e.what();
            }
        }
    }

    this->last_updated = time(nullptr);

    return SUCCESS_STATUS_INT;
}

void GameServersList::stats_process()
{
    Json::Value jupdate_data;

    State& state = State::getInstance();
    std::string str_timediff = state.get(STATE_PANEL_TIMEDIFF);

    time_t time_diff = str_timediff.empty()
            ? 0
            : std::stol(str_timediff, 0, 10);

    for (auto& server : servers_list) {
        Json::Value jserver;
        jserver["id"] = static_cast<unsigned long long>(server.second->id);

        if (server.second->installed == Server::SERVER_INSTALLED) {
            GameServerCmd game_server_cmd = GameServerCmd(GameServerCmd::STATUS, server.second->id);
            game_server_cmd.execute();

            server.second->process_active = game_server_cmd.result();
            server.second->last_process_check = time(nullptr);

            time_t last_process_check = server.second->last_process_check - time_diff;
            std::tm * ptm = std::localtime(&last_process_check);
            char buffer[32];
            std::strftime(buffer, 32, "%F %T", ptm);

            jserver["last_process_check"]   = buffer;
            jserver["process_active"]       = server.second->process_active ? 1 : 0;
        }

        jserver["installed"]            = server.second->installed;
        jupdate_data.append(jserver);
    }

    try {
        Rest::patch("/gdaemon_api/servers", jupdate_data);
    } catch (Rest::RestapiException &exception) {
        GAMEAP_LOG_ERROR << exception.what() << '\n';
    }
}

void GameServersList::loop()
{
    stats_process();
}

void GameServersList::sync_all()
{
    for (auto& server : this->servers_list) {
        this->sync_from_api(server.second->id);
    }
}

Server * GameServersList::get_server(ulong server_id)
{
    if (servers_list.find(server_id) == servers_list.end()) {
        if (update_list() == ERROR_STATUS_INT) {
            return nullptr;
        }

        if (servers_list.find(server_id) == servers_list.end()) {
            return nullptr;
        }
    }

    this->sync_from_api(server_id);
    return servers_list[server_id].get();
}

void GameServersList::delete_server(ulong server_id)
{
    auto it = servers_list.find(server_id);

    if (it != servers_list.end()) {
        servers_list.erase(it);
    }
}

void GameServersList::sync_from_api(ulong server_id)
{
    Json::Value jserver;

    try {
        jserver = Rest::get("/gdaemon_api/servers/" + std::to_string(server_id));
    } catch (Rest::RestapiException &exception) {
        // Try later
        GAMEAP_LOG_ERROR << exception.what();
        return;
    }

    if (jserver.empty()) {
        return;
    }

    Server *server = servers_list[server_id].get();

    server->enabled     = jserver["enabled"].asBool();
    server->installed   = static_cast<Server::install_status>(jserver["installed"].asUInt());
    server->blocked     = jserver["blocked"].asBool();
    server->name        = jserver["name"].asString();
    server->uuid        = jserver["uuid"].asString();
    server->uuid_short  = jserver["uuid_short"].asString();

    server->game = Game{
        jserver["game"]["code"].asString(),
        jserver["game"]["start_code"].asString(),
        jserver["game"]["name"].asString(),
        jserver["game"]["engine"].asString(),
        jserver["game"]["engine_version"].asString(),
        jserver["game"]["steam_app_id"].asUInt(),
        jserver["game"]["steam_app_set_config"].asString(),
        jserver["game"]["remote_repository"].asString(),
        jserver["game"]["local_repository"].asString()
    };

    Json::Value jvars = jserver["game_mod"]["vars"];
    std::map<std::string, std::string> vars;

    for (auto jvar: jvars) {
        if (jvar["default"].isNull()) {
            vars.insert(std::pair<std::string, std::string>(jvar["var"].asString(), ""));
        }
        else if (jvar["default"].isString()) {
            vars.insert(std::pair<std::string, std::string>(jvar["var"].asString(), jvar["default"].asString()));
        }
        else if (jvar["default"].isInt()) {
            vars.insert(std::pair<std::string, std::string>(jvar["var"].asString(), std::to_string(jvar["default"].asInt())));
        }
        else {
            GAMEAP_LOG_ERROR << "Unknown alias type: " << jvar["default"];
        }
    }

    Json::Value jserver_vars = jserver["vars"];
    for (auto const& key: jserver_vars.getMemberNames()) {
        auto jvar = jserver_vars[key];

        if (jvar.isString()) {
            if (vars.find(key) == vars.end()) {
                vars.insert(std::pair<std::string, std::string>(key, jvar.asString()));
            } else {
                vars[key] = jvar.asString();
            }
        }
        else if (jvar.isInt()) {
            if (vars.find(key) == vars.end()) {
                vars.insert(std::pair<std::string, std::string>(key, std::to_string(jvar.asInt())));
            } else {
                vars[key] = std::to_string(jvar.asInt());
            }
        } else {
            GAMEAP_LOG_ERROR << "Unknown alias type: " << jvar;
        }
    }

    server->vars = vars;

    server->game_mod = GameMod{
        jserver["game_mod"]["id"].asUInt(),
        jserver["game_mod"]["name"].asString(),
        jserver["game_mod"]["remote_repository"].asString(),
        jserver["game_mod"]["local_repository"].asString(),
        jserver["game_mod"]["default_start_cmd_linux"].asString(),
        jserver["game_mod"]["default_start_cmd_windows"].asString()
    };
}