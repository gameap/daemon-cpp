#ifndef GAME_SERVER_H
#define GAME_SERVER_H

#include <string> 
#include <vector>
#include <mutex>
#include <boost/process.hpp>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include "cmd_output.h"
#include "log.h"
#include "dedicated_server.h"
#include "typedefs.h"

#define SERVER_NOT_INSTALLED        0
#define SERVER_INSTALLED            1
#define SERVER_INSTALL_IN_PROCESS   2

#define TIME_UPDDIFF 60
#define TIME_INSTALL_BLOCK 1800

#define TIME_CACHE_STATUS 30

namespace GameAP {

namespace fs = boost::filesystem;

class GameServer {
    public:
        bool m_active;
        std::time_t m_last_process_check;
        bool m_install_process;
        unsigned int m_installed;

        GameServer(ulong mserver_id);

        ~GameServer() {
            // TODO: Implement waiting until the operations (installation and other) finishing successfully
            GAMEAP_LOG_VERBOSE << "Game Server Destruct";
        }

        // int install_game_server(); // TODO: Implement
        int update_server();
        int delete_files();
        // int move_game_server(); // TODO: Implement
        int cmd_exec(std::string command);

        int start_server();
        void start_if_need();
        int stop_server();
        bool status_server();

        // int get_game_server_load(); // TODO: Implement

        // size_t get_cmd_output(std::string * output, size_t position);
        int get_cmd_output(std::string * str_out);
        std::string get_cmd_output();
        void clear_cmd_output();

        void loop();

        void update();
        void update(bool force);

        unsigned int get_id() {
            return m_server_id;
        }

    private:
        std::string m_uuid;
        std::string m_uuid_short;

        unsigned int m_server_id;
        std::string m_ip;

        uint m_server_port;
        uint m_query_port;
        uint m_rcon_port;

        bool m_staft_crash_disabled;
        bool m_staft_crash;

        std::string m_user;
        std::map<std::string, std::string> m_aliases;

        std::string m_start_command;
        std::string m_game_scode;

        std::string m_game_localrep;
        std::string m_game_remrep;
        std::string m_mod_localrep;
        std::string m_mod_remrep;

        std::string m_steam_app_id;
        std::string m_steam_app_set_config;

        unsigned int m_install_status_changed;

        pid_t m_last_pid;

        time_t m_last_update_vars;

        fs::path m_work_path;

        std::shared_ptr<CmdOutput> m_cmd_output;

        void _replace_shortcodes(std::string &command);

        int _exec(const std::string &command, bool not_append);
        int _exec(const std::string &command);

        bool _server_status_cmd();

        void _update_vars();
        void _update_vars(bool force);
        void _append_cmd_output(const std::string &line);

        int _update_server();

        void _set_installed(unsigned int status);
        void _try_unblock();

        void _error(const std::string &msg);
};

/* End namespace GameAP */
}

#endif
