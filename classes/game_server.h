#ifndef GAME_SERVER_H
#define GAME_SERVER_H

#include <string> 
#include <vector> 
#include <boost/process.hpp>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include "dedicated_server.h"

#define INST_FROM_LOCREP 1
#define INST_FROM_REMREP 2
#define INST_FROM_STEAM 3

#define INST_FILE 1
#define INST_DIR 2

namespace GameAP {

class GameServer {
private:
    std::string work_dir;
    std::string screen_name;

    ulong server_id;
    std::string ip;
    uint server_port;
    uint query_port;
    uint rcon_port;

    std::string user;
    
    std::string start_command;
    std::string stop_command;

    std::string game_localrep;
    std::string game_remrep;
    std::string gt_localrep;
    std::string gt_remrep;

    boost::filesystem::path work_path;

    std::string cmd_output;

    int _unpack_archive(boost::filesystem::path const & archive);
    
    bool _copy_dir(
        boost::filesystem::path const & source,
        boost::filesystem::path const & destination
    );

    void _append_cmd_output(std::string line);
    
public:
    GameServer(ulong mserver_id);
    
    int install_game_server();
    int update_server();
    int delete_game_server();
    int move_game_server();

    int start_server();
    int stop_server();

    int get_game_server_load();

    std::string get_cmd_output() {
        return cmd_output;
    }
};

/* End namespace GameAP */
}

#endif
