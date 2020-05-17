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
        GAMEAP_LOG_VERBOSE << "Updating game servers list...";
        jvalue = Rest::get("/gdaemon_api/servers?fields[servers]=id");
    } catch (Rest::RestapiException &exception) {
        // Try later
        GAMEAP_LOG_ERROR << exception.what();
        return ERROR_STATUS_INT;
    }

    for (auto const& jserver: jvalue) {
        ulong server_id = getJsonUInt(jserver["id"]);

        if (servers_list.find(server_id) == servers_list.end()) {

            // TODO: Improve
            std::shared_ptr<Server> server = std::make_shared<Server>(Server{
                getJsonUInt(jserver["id"]),
                jserver["enabled"].asBool(),
                static_cast<Server::install_status>(getJsonUInt(jserver["installed"])),
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
                    getJsonUInt(jserver["game_mod_id"]),
                    "",
                    "",
                    "",
                    "",
                    ""
                },
                jserver["server_ip"].asString(),
                static_cast<unsigned short>(getJsonUInt(jserver["server_port"])),
                static_cast<unsigned short>(getJsonUInt(jserver["query_port"])),
                static_cast<unsigned short>(getJsonUInt(jserver["rcon_port"])),
                jserver["dir"].asString(),
                jserver["su_user"].asString(),
                jserver["start_command"].asString(),
                jserver["stop_command"].asString(),
                jserver["force_stop_command"].asString(),
                jserver["restart_command"].asString(),
                jserver["process_active"].asBool(),
                0, // Last process check
                std::unordered_map<std::string, std::string>(), // vars
                std::unordered_map<std::string, std::string>(), // settings
                0                                               // updated_at
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

    for (auto const& server : this->servers_list) {
        Json::Value jserver;
        jserver["id"] = static_cast<Json::UInt64>(server.second->id);

        if (server.second->installed == Server::SERVER_INSTALLED) {
            GameServerCmd game_server_cmd = GameServerCmd(GameServerCmd::STATUS, server.second->id);
            game_server_cmd.execute();

            server.second->process_active = game_server_cmd.result();
            server.second->last_process_check = time(nullptr);

            time_t last_process_check = server.second->last_process_check - time_diff;
            std::tm * ptm = std::gmtime(&last_process_check);
            char buffer[32];
            std::strftime(buffer, 32, "%F %T", ptm);

            jserver["last_process_check"]   = buffer;
            jserver["process_active"]       = server.second->process_active ? 1 : 0;
        }

        jserver["installed"]                = server.second->installed;
        jupdate_data.append(jserver);

        server.second->updated_at = time(nullptr);
    }

    if (! jupdate_data.empty()) {
        try {
            GAMEAP_LOG_VERBOSE << "Insert servers information...";
            Rest::patch("/gdaemon_api/servers", jupdate_data);
        } catch (Rest::RestapiException &exception) {
            GAMEAP_LOG_ERROR << exception.what() << '\n';
        }
    }
}

void GameServersList::sync_all()
{
    Json::Value jupdate_data;

    State& state = State::getInstance();
    std::string str_timediff = state.get(STATE_PANEL_TIMEDIFF);

    time_t time_diff = str_timediff.empty()
       ? 0
       : std::stol(str_timediff, 0, 10);

    for (auto const& server : this->servers_list) {
        Json::Value jserver;
        jserver["id"] = static_cast<Json::UInt64>(server.second->id);

        time_t last_process_check = server.second->last_process_check - time_diff;

        if (last_process_check > 0) {
            std::tm *ptm = std::gmtime(&last_process_check);
            char buffer[32];
            std::strftime(buffer, 32, "%F %T", ptm);

            jserver["last_process_check"] = buffer;
            jserver["process_active"] = server.second->process_active ? 1 : 0;
        }

        jserver["installed"]            = server.second->installed;
        jupdate_data.append(jserver);

        server.second->updated_at = time(nullptr);
    }

    if (! jupdate_data.empty()) {
        try {
            GAMEAP_LOG_VERBOSE << "Insert servers information...";
            Rest::patch("/gdaemon_api/servers", jupdate_data);
        } catch (Rest::RestapiException &exception) {
            GAMEAP_LOG_ERROR << exception.what() << '\n';
        }
    }
}

void GameServersList::loop()
{
    stats_process();
    start_down_servers();
}

void GameServersList::start_down_servers()
{
    for (auto& server : this->servers_list) {
        if (! server.second->autostart()
            || server.second->process_active
            || server.second->installed != Server::SERVER_INSTALLED
        ) {
            continue;
        }

        GAMEAP_LOG_DEBUG << "Starting crashed server";
        GameServerCmd start_cmd = GameServerCmd(GameServerCmd::START, server.second->id);
        start_cmd.execute();

        if (! start_cmd.result()) {
            GAMEAP_LOG_ERROR << "Unable to autostart server";

            std::string output;
            start_cmd.output(&output);
            GAMEAP_LOG_VERBOSE_ERROR << output;
        }
    }
}

Server * GameServersList::get_server(ulong server_id)
{
    if (this->servers_list.find(server_id) == this->servers_list.end()) {
        // Forget cache and try update list
        this->last_updated = 0;
        if (this->update_list() == ERROR_STATUS_INT) {
            return nullptr;
        }

        if (this->servers_list.find(server_id) == this->servers_list.end()) {
            return nullptr;
        }
    }

    this->sync_from_api(server_id);
    return this->servers_list[server_id].get();
}

void GameServersList::set_install_status(unsigned long server_id, Server::install_status status)
{
    if (this->servers_list.find(server_id) == this->servers_list.end()) {
        return;
    }

    this->servers_list[server_id]->installed = status;
    this->sync_all();
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
        GAMEAP_LOG_VERBOSE << "Sync game server [" << server_id << "]";
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

    time_t updated_at   = human_to_timestamp(jserver["updated_at"].asString());

    if (updated_at > server->updated_at) {
        server->enabled     = jserver["enabled"].asBool();
        server->installed   = static_cast<Server::install_status>(getJsonUInt(jserver["installed"]));
        server->blocked     = jserver["blocked"].asBool();
    }

    server->updated_at  = updated_at;

    server->name        = jserver["name"].asString();
    server->uuid        = jserver["uuid"].asString();
    server->uuid_short  = jserver["uuid_short"].asString();

    server->start_command           = jserver["start_command"].asString();
    server->stop_command            = jserver["stop_command"].asString();
    server->force_stop_command      = jserver["force_stop_command"].asString();
    server->restart_command         = jserver["restart_command"].asString();

    server->game = Game{
        jserver["game"]["code"].asString(),
        jserver["game"]["start_code"].asString(),
        jserver["game"]["name"].asString(),
        jserver["game"]["engine"].asString(),
        jserver["game"]["engine_version"].asString(),
        getJsonUInt(jserver["game"]["steam_app_id"]),
        jserver["game"]["steam_app_set_config"].asString(),
        jserver["game"]["remote_repository"].asString(),
        jserver["game"]["local_repository"].asString()
    };

    server->game_mod = GameMod{
        getJsonUInt(jserver["game_mod"]["id"]),
        jserver["game_mod"]["name"].asString(),
        jserver["game_mod"]["remote_repository"].asString(),
        jserver["game_mod"]["local_repository"].asString(),
        jserver["game_mod"]["default_start_cmd_linux"].asString(),
        jserver["game_mod"]["default_start_cmd_windows"].asString()
    };

    Json::Value jvars = jserver["game_mod"]["vars"];
    std::unordered_map<std::string, std::string> vars;

    for (auto const& jvar: jvars) {
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

    std::unordered_map<std::string, std::string> settings;

    // Settings
    for (auto const& jsetting: jserver["settings"]) {
        server->set_setting(jsetting["name"].asString(), jsetting["value"].asString());
    }
}

GameServersListStats GameServersList::stats()
{
    unsigned int active_servers_count = 0;
    for(auto const & server: this->servers_list) {
        if (server.second->process_active) {
            active_servers_count++;
        }
    }

    return GameServersListStats{
            active_servers_count
    };
}