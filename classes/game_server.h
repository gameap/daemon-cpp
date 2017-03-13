#ifndef GAME_SERVER_H
#define GAME_SERVER_H

#include <string> 
#include <vector> 
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

struct gs_stats {
    time_t time;

    std::vector<float> cpu_perc;
    uintmax_t cpu_time;
    uintmax_t ram_us;

    std::map<std::string, netstats> netstats;
    uintmax_t drv_us_space;
};

class GameServer {
private:
    std::string screen_name;

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

    pid_t last_pid;

    time_t last_update_vars;

    boost::filesystem::path work_path;

    std::string cmd_output;

    int _unpack_archive(boost::filesystem::path const & archive);
    
    bool _copy_dir(
        boost::filesystem::path const & source,
        boost::filesystem::path const & destination
    );

    int _exec(std::string cmd);
    boost::process::child __exec(std::string cmd, boost::process::pipe &out);

    void _update_vars();
    void _append_cmd_output(std::string line);
    
public:
    GameServer(ulong mserver_id);
    
    ~GameServer() {
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
    void clear_cmd_output();
};

// ---------------------------------------------------------------------

class GameServersList {
private:
    std::map<ulong, GameServer *> servers_list;
    
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
