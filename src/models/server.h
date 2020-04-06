#ifndef GDAEMON_SERVER_H
#define GDAEMON_SERVER_H

#include <unordered_map>
#include <ctime>

#include "models/game.h"
#include "models/game_mod.h"

namespace GameAP {
    struct Server {
        unsigned long id;

        bool enabled;
        enum install_status {
            SERVER_NOT_INSTALLED        = 0,
            SERVER_INSTALLED            = 1,
            SERVER_INSTALL_IN_PROCESS   = 2,
        } installed;
        bool blocked;

        std::string name;
        std::string uuid;
        std::string uuid_short;

        Game game;
        GameMod game_mod;

        std::string ip;
        unsigned short connect_port;
        unsigned short query_port;
        unsigned short rcon_port;

        std::string dir;
        std::string user;

        std::string start_command;
        std::string stop_command;
        std::string force_stop_command;
        std::string restart_command;

        // Status
        bool process_active;
        std::time_t last_process_check;

        // Server variables
        std::unordered_map<std::string, std::string> vars;

        // Settings
        std::unordered_map<std::string, std::string> settings;

        std::string get_setting(const std::string & name) {
            return (settings.find(name) != settings.end())
               ? settings[name]
               : "";
        }

        // TODO: Replace value to std::variant type after C++17 changing
        void set_setting(const std::string & name, const std::string & value)
        {
            auto it = settings.find(name);

            if (it != settings.end()) {
                it->second = value;
            } else {
                settings.insert(
                        std::make_pair(
                                name,
                                value
                        )
                );
            }
        }

        // Autostart setting
        bool autostart()
        {
            std::string autostart_str = get_setting("autostart_current");

            if (autostart_str.empty()) {
                autostart_str = get_setting("autostart");
            }

            if (autostart_str.empty()) {
                return false;
            }

            return (autostart_str == "1" || autostart_str == "true");
        }

    };
}

#endif //GDAEMON_SERVER_H
