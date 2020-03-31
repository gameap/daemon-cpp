#ifndef GDAEMON_GAME_H
#define GDAEMON_GAME_H

namespace GameAP {
    struct Game {
        std::string code;
        std::string start_code;
        std::string name;
        std::string engine;
        std::string engine_version;
        unsigned int steam_app_id;
        std::string steam_app_set_config;
        std::string remote_repository;
        std::string local_repository;
    };
}

#endif //GDAEMON_GAME_H
