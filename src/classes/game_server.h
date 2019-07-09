#ifndef GAME_SERVER_H
#define GAME_SERVER_H

#include <string> 
#include <vector>
#include <mutex>
#include <boost/process.hpp>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include "dedicated_server.h"
#include "typedefs.h"

#define INST_NO_SOURCE 0
#define INST_FROM_LOCREP 1
#define INST_FROM_REMREP 2
#define INST_FROM_STEAM 3

#define INST_FILE 1
#define INST_DIR 2

#define SERVER_NOT_INSTALLED        0
#define SERVER_INSTALLED            1
#define SERVER_INSTALL_IN_PROCESS   2

#define TIME_UPDDIFF 120
#define TIME_INSTALL_BLOCK 1800

#if defined(BOOST_POSIX_API)
    #define AFTER_INSTALL_SCRIPT "gdaemon_after_install.sh"

    #define PROC_SHELL "sh"
    #define SHELL_PREF "-c"
#elif defined(BOOST_WINDOWS_API)
    #define AFTER_INSTALL_SCRIPT "gdaemon_after_install.bat"

    #define PROC_SHELL "cmd"
    #define SHELL_PREF "/c"
#endif 

namespace GameAP {

namespace fs = boost::filesystem;

class GameServer {
private:
    std::string m_uuid;
    std::string m_uuid_short;

    ulong m_server_id;
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
    std::string m_gt_localrep;
    std::string m_gt_remrep;

    std::string m_steam_app_id;
    std::string m_steam_app_set_config;

    unsigned int m_install_status_changed;

    pid_t m_last_pid;

    time_t m_last_update_vars;

    fs::path m_work_path;

    std::mutex m_cmd_output_mutex;
    std::shared_ptr<std::string> m_cmd_output;

    int _unpack_archive(fs::path const & archive);
    
    bool _copy_dir(
        fs::path const & source,
        fs::path const & destination
    );

    int _exec(std::string cmd, bool not_append);
    int _exec(std::string cmd);

    bool _server_status_cmd();

    void _update_vars();
    void _update_vars(bool force);
    void _append_cmd_output(std::string line);

    void _set_installed(unsigned int status);
    void _try_unblock();

    void _error(std::string msg);
    
public:
    bool m_active;
    std::time_t m_last_process_check;
    bool m_install_process;
    unsigned int m_installed;

    GameServer(ulong mserver_id);
    
    ~GameServer() {
        //delete cmd_output;
        std::cout << "Game Server Destruct" << std::endl;
    }
    
    int install_game_server();
    int update_server();
    int delete_server();
    int move_game_server();
    int cmd_exec(std::string cmd);

    void replace_shortcodes(std::string &cmd);
    int start_server();
    void start_if_need();
    int stop_server();
    bool status_server();

    int get_game_server_load();
    
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
};

/* End namespace GameAP */
}

#endif
