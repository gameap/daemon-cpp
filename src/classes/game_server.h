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

#define TIME_UPDDIFF 120

#if defined(BOOST_POSIX_API)
    #define PROC_SHELL "sh"
    #define SHELL_PREF "-c"
#elif defined(BOOST_WINDOWS_API)
    #define PROC_SHELL "cmd"
    #define SHELL_PREF "/c"
#endif 

namespace GameAP {

namespace fs = boost::filesystem;

class GameServer {
private:
    std::string uuid;
    std::string uuid_short;

    ulong server_id;
    std::string ip;
    uint server_port;
    uint query_port;
    uint rcon_port;

    bool staft_crash_disabled;
    bool staft_crash;

    std::string user;
    std::map<std::string, std::string> aliases;
    
    std::string start_command;
    std::string game_scode;

    std::string game_localrep;
    std::string game_remrep;
    std::string gt_localrep;
    std::string gt_remrep;

    std::string steam_app_id;
    std::string steam_app_set_config;

    pid_t last_pid;

    time_t last_update_vars;

    fs::path work_path;

    //std::string *cmd_output;
    std::mutex cmd_output_mutex;
    std::shared_ptr<std::string> cmd_output;

    int _unpack_archive(std::string archive);
    
    bool _copy_dir(
        fs::path const & source,
        fs::path const & destination
    );

    int _exec(std::string cmd, bool not_append);
    int _exec(std::string cmd);

    bool _server_status_cmd();

    void _update_vars();
    void _append_cmd_output(std::string line);

    void _set_installed(unsigned int status);
    
public:
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
};

// ---------------------------------------------------------------------

class GameServersList {
private:
    std::map<ulong, std::shared_ptr<GameServer>> servers_list;
    
    GameServersList() {}
    GameServersList( const GameServersList&);  
    GameServersList& operator=( GameServersList& );

    int update_list();
public:
    static GameServersList& getInstance() {
        static GameServersList instance;
        return instance;
    }

    GameServer * get_server(ulong server_id);
    
    void stats_process();
};

/* End namespace GameAP */
}

#endif
