#include "consts.h"
#include "config.h"

#include "game_servers_list.h"

#include "functions/restapi.h"
#include "functions/gstring.h"

#include <json/json.h>

using namespace GameAP;

// ---------------------------------------------------------------------

int GameServersList::update_list()
{
    Config& config = Config::getInstance();

    Json::Value jvalue;

    try {
        jvalue = Gameap::Rest::get("/gdaemon_api/servers?fields[servers]=id");
    } catch (Gameap::Rest::RestapiException &exception) {
        // Try later
        std::cerr << exception.what() << std::endl;
        return ERROR_STATUS_INT;
    }

    for (Json::ValueIterator itr = jvalue.begin(); itr != jvalue.end(); ++itr) {
        ulong server_id = getJsonUInt((*itr)["id"]);

        if (servers_list.find(server_id) == servers_list.end()) {

            std::shared_ptr<GameServer> gserver = std::make_shared<GameServer>(server_id);

            try {
                servers_list.insert(
                        servers_list.end(),
                        std::pair<ulong, std::shared_ptr<GameServer>>(server_id, gserver)
                );
            } catch (std::exception &e) {
                std::cerr << "GameServer #" << server_id << " insert error: " << e.what() << std::endl;
            }
        }
    }

    return SUCCESS_STATUS_INT;
}

// ---------------------------------------------------------------------

void GameServersList::stats_process()
{
    Json::Value jupdate_data;

    for (auto& server : servers_list) {
        Json::Value jserver_data;

        jserver_data["id"] = server.second->get_id();
        jserver_data["installed"] = server.second->m_installed;

        if (server.second->m_last_process_check > 0 && server.second->m_installed == SERVER_INSTALLED) {
            std::tm * ptm = std::localtime(&server.second->m_last_process_check);
            char buffer[32];
            std::strftime(buffer, 32, "%F %T", ptm);

            jserver_data["last_process_check"] = buffer;
            jserver_data["process_active"] = static_cast<int>(server.second->m_active);
        }

        jupdate_data.append(jserver_data);
    }

    try {
        Gameap::Rest::patch("/gdaemon_api/servers", jupdate_data);
    } catch (Gameap::Rest::RestapiException &exception) {
        std::cerr << exception.what() << '\n';
    }
}

// ---------------------------------------------------------------------

void GameServersList::loop()
{
    for (auto& server : servers_list) {
        server.second->loop();
    }

    stats_process();
}

// ---------------------------------------------------------------------

void GameServersList::update_all(bool force)
{
    for (auto& server : servers_list) {
        server.second->update(true);
    }
}

// ---------------------------------------------------------------------

GameServer * GameServersList::get_server(ulong server_id)
{
    if (servers_list.find(server_id) == servers_list.end()) {
        if (update_list() == ERROR_STATUS_INT) {
            return nullptr;
        }

        if (servers_list.find(server_id) == servers_list.end()) {
            return nullptr;
        }
    }

    if (servers_list[server_id] == nullptr) {
        return nullptr;
    }

    return servers_list[server_id].get();
}
