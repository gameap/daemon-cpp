#ifndef GDAEMON_GAME_SERVERS_LIST_H
#define GDAEMON_GAME_SERVERS_LIST_H

#include "classes/game_server.h"

namespace GameAP {

class GameServersList {
private:
    std::map<ulong, std::shared_ptr<GameServer>> servers_list;

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

    GameServer * get_server(ulong server_id);
    void loop();

    void update_all();
    void update_all(bool force);
};

/* End namespace GameAP */
}

#endif //GDAEMON_GAME_SERVERS_LIST_H
