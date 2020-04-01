#ifndef GDAEMON_GAME_SERVERS_LIST_H
#define GDAEMON_GAME_SERVERS_LIST_H

#include <memory>
#include <unordered_map>

#include "models/server.h"

namespace GameAP {

class GameServersList {
private:
    std::unordered_map<ulong, std::shared_ptr<Server>> servers_list;

    GameServersList() {
        update_list();
    }

    GameServersList( const GameServersList&);
    GameServersList& operator=( GameServersList& );

    int update_list();
    void stats_process();
public:
    static GameServersList& getInstance() {
        static GameServersList instance;
        return instance;
    }

    Server * get_server(ulong server_id);
    void delete_server(ulong server_id);
    void loop();

    void update_all();
    void update_all(bool force);

    void sync_from_api(ulong server_id);
};

/* End namespace GameAP */
}

#endif //GDAEMON_GAME_SERVERS_LIST_H
