#ifndef GDAEMON_GAME_MOD_H
#define GDAEMON_GAME_MOD_H

namespace GameAP {
    struct GameMod {
        unsigned int id;
        std::string name;
        std::string remote_repository;
        std::string local_repository;
        std::string default_start_cmd_linux;
        std::string default_start_cmd_windows;
    };
}

#endif //GDAEMON_GAME_MOD_H
