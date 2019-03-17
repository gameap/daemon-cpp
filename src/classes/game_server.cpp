#define BOOST_NO_CXX11_SCOPED_ENUMS
#include "consts.h"

#include <boost/process.hpp>
#include <boost/iostreams/stream.hpp>

#include <json/json.h>

#include "config.h"
#include "game_server.h"

#include <boost/filesystem.hpp>
#undef BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/format.hpp>

#include "functions/restapi.h"
#include "functions/gsystem.h"
#include "functions/gstring.h"

#include "classes/tasks.h"

using namespace GameAP;
using namespace boost::process;

namespace fs = boost::filesystem;
namespace bp = ::boost::process;

#ifdef __linux__
    #define STEAMCMD "steamcmd.sh"
#elif _WIN32
    #define STEAMCMD "steamcmd.exe"
#endif

// ---------------------------------------------------------------------

GameServer::GameServer(ulong mserver_id)
{
    server_id = mserver_id;
    last_update_vars = 0;

    staft_crash_disabled = false;
    cmd_output = std::make_shared<std::string> ("");

    try {
        _update_vars();
    } catch (std::exception &e) {
        std::cerr << "Server update vars error: " << e.what() << std::endl;
    }
}

// ---------------------------------------------------------------------

void GameServer::_update_vars()
{
    if (time(nullptr) - last_update_vars < TIME_UPDDIFF) {
        return;
    }

    DedicatedServer& dedicatedServer = DedicatedServer::getInstance();

    Json::Value jvalue;

    try {
        jvalue = Gameap::Rest::get("/gdaemon_api/servers/" + std::to_string(server_id));
    } catch (Gameap::Rest::RestapiException &exception) {
        // Try later
        std::cerr << exception.what() << std::endl;
        return;
    }

    if (jvalue.empty()) {
        throw("Empty API response");
    }

    work_path = dedicatedServer.get_work_path();

    work_path /= jvalue["dir"].isString()
                 ? jvalue["dir"].asString()
                 : throw("Empty game server directory");

    uuid            = jvalue["uuid"].asString();
    uuid_short      = jvalue["uuid_short"].asString();

    user            = jvalue["su_user"].asString();

    ip              = jvalue["server_ip"].asString();

    server_port     = jvalue["server_port"].asUInt();

    query_port      = jvalue["query_port"].isUInt()
                        ? jvalue["query_port"].asUInt()
                        : server_port;

    rcon_port       = jvalue["rcon_port"].isUInt()
                        ? jvalue["rcon_port"].asUInt()
                        : server_port;

    start_command   = jvalue["start_command"].asString();
    game_scode      = jvalue["code_name"].asString();

    game_localrep  = jvalue["game"]["local_repository"].asString();
    game_remrep    = jvalue["game"]["remote_repository"].asString();

    steam_app_id            = jvalue["game"]["steam_app_id"].asString();
    steam_app_set_config    = jvalue["game"]["steam_app_set_config"].asString();

    gt_localrep    = jvalue["game_mod"]["local_repository"].asString();
    gt_remrep      = jvalue["game_mod"]["remote_repository"].asString();

    // Replace os shortcode
    game_localrep   = str_replace("{os}", OS, game_localrep);
    game_remrep     = str_replace("{os}", OS, game_remrep);
    gt_localrep     = str_replace("{os}", OS, gt_localrep);
    gt_remrep       = str_replace("{os}", OS, gt_remrep);

    staft_crash = jvalue["start_after_crash"].asBool();

    aliases.clear();

    try {
        Json::Value jaliases;

        Json::Reader jreader(Json::Features::strictMode());

        jaliases = jvalue["game_mod"]["vars"];

        for( Json::ValueIterator itr = jaliases.begin() ; itr != jaliases.end() ; itr++ ) {

            if ((*itr)["default"].isNull()) {
                aliases.insert(std::pair<std::string, std::string>((*itr)["var"].asString(), ""));
            }
            else if ((*itr)["default"].isString()) {
                aliases.insert(std::pair<std::string, std::string>((*itr)["var"].asString(), (*itr)["default"].asString()));
            }
            else if ((*itr)["default"].isInt()) {
                aliases.insert(std::pair<std::string, std::string>((*itr)["var"].asString(), std::to_string((*itr)["default"].asInt())));
            }
            else {
                std::cerr << "Unknown alias type: " << (*itr)["default"] << std::endl;
            }
        }

        Json::Value server_vars = jvalue["vars"];;

        for( Json::ValueIterator itr = server_vars.begin() ; itr != server_vars.end() ; itr++ ) {
            if ((*itr).isString()) {
                if (aliases.find(itr.key().asString()) == aliases.end()) {
                    aliases.insert(std::pair<std::string, std::string>(itr.key().asString(), (*itr).asString()));
                } else {
                    aliases[itr.key().asString()] = (*itr).asString();
                }
            }
            else if ((*itr).isInt()) {
                if (aliases.find(itr.key().asString()) == aliases.end()) {
                    aliases.insert(std::pair<std::string, std::string>(itr.key().asString(), std::to_string((*itr).asInt())));
                } else {
                    aliases[itr.key().asString()] = std::to_string((*itr).asInt());
                }
            } else {
                std::cerr << "Unknown alias type: " << (*itr) << std::endl;
            }
        }
    }
    catch (std::exception &e) {
        std::cerr << "Aliases error: " << e.what() << std::endl;
    }

    last_update_vars = time(nullptr);
}

// ---------------------------------------------------------------------

void GameServer::_append_cmd_output(std::string line)
{
    cmd_output_mutex.lock();

    line.append("\n");
    (*cmd_output).append(line);

    cmd_output_mutex.unlock();
}

// ---------------------------------------------------------------------

// size_t GameServer::get_cmd_output(std::string * output, size_t position)
int GameServer::get_cmd_output(std::string * str_out)
{
    *str_out = *cmd_output;
    return 0;
}

// ---------------------------------------------------------------------

std::string GameServer::get_cmd_output()
{
    return *cmd_output;
}

// ---------------------------------------------------------------------

void GameServer::clear_cmd_output()
{
    cmd_output_mutex.lock();
    (*cmd_output).clear();
    cmd_output_mutex.unlock();
}

// ---------------------------------------------------------------------

void GameServer::replace_shortcodes(std::string &cmd)
{
    cmd = str_replace("{dir}", work_path.string(), cmd);

    cmd = str_replace("{uuid}", uuid, cmd);
    cmd = str_replace("{uuid_short}", uuid_short, cmd);

    cmd = str_replace("{host}", ip, cmd);
    cmd = str_replace("{ip}", ip, cmd);
    cmd = str_replace("{game}", game_scode, cmd);

    cmd = str_replace("{id}", std::to_string(server_id), cmd);
    cmd = str_replace("{port}", std::to_string(server_port), cmd);
    cmd = str_replace("{query_port}", std::to_string(query_port), cmd);
    cmd = str_replace("{rcon_port}", std::to_string(rcon_port), cmd);
    
    cmd = str_replace("{user}", user, cmd);

    // Aliases
    for (std::map<std::string, std::string>::iterator itr = aliases.begin(); itr != aliases.end(); ++itr) {
        cmd = str_replace("{" + itr->first + "}", itr->second, cmd);
    }
}

// ---------------------------------------------------------------------

int GameServer::start_server()
{
    if (status_server()) {
        // Server online
        return 0;
    }

    DedicatedServer& deds = DedicatedServer::getInstance();
    std::string cmd  = str_replace("{command}", start_command, deds.get_script_cmd(DS_SCRIPT_START));
    replace_shortcodes(cmd);

    staft_crash_disabled = false;

    int result = _exec(cmd);
    
    if (result == -1) {
        return -1;
    } else {
        return 0;
    }
}

// ---------------------------------------------------------------------

void GameServer::start_if_need()
{
    bool cur_status = status_server();
    
    if (!staft_crash) {
        return;
    }

    if (staft_crash_disabled) {
        return;
    }

    if (!cur_status) {
        start_server();
    }
}

// ---------------------------------------------------------------------

int GameServer::stop_server()
{
    if (!status_server()) {
        // Server offline
        //return 0;
    }

    DedicatedServer& deds = DedicatedServer::getInstance();
    // std::string cmd  = str_replace("{command}", "", deds.get_script_cmd(DS_SCRIPT_STOP));
    std::string cmd  = deds.get_script_cmd(DS_SCRIPT_STOP);
    replace_shortcodes(cmd);

    staft_crash_disabled = true;
    
    int result = _exec(cmd);

    if (result == -1) {
        return -1;
    } else {
        return 0;
    }
}

// ---------------------------------------------------------------------

int GameServer::update_server()
{
    if (status_server()) {
        if (stop_server() == -1) {
            return -1;
        }
    }

    // Update installed = 2. In process
    _set_installed(2);

    DedicatedServer& deds = DedicatedServer::getInstance();
    std::string update_cmd = deds.get_script_cmd(DS_SCRIPT_UPDATE);
    std::string steamcmd_path = deds.get_steamcmd_path();

    if (update_cmd.length() > 0) {
        replace_shortcodes(update_cmd);
        int result = _exec(update_cmd);

        if (result == 0) {
            _set_installed(1);
            return 0;
        } else {
            _set_installed(0);
            return -1;
        }
    }

    std::cout << "Update Start" << std::endl;

    ushort game_install_from =  INST_NO_SOURCE;
    ushort game_mod_install_from =    INST_NO_SOURCE;

    // Install game
    if (!game_localrep.empty()) {
        game_install_from = INST_FROM_LOCREP;
    }
    else if (!game_remrep.empty()) {
        game_install_from = INST_FROM_REMREP;
    }
    else if (!steam_app_id.empty() && !steamcmd_path.empty()) {
        game_install_from = INST_FROM_STEAM;
    }
    else {
        // No Source to install =(
        _append_cmd_output("No source to install game");
        std::cerr << "No source to install" << std::endl;
        _set_installed(0);
        return -1;
    }

    if (!gt_localrep.empty()) {
        game_mod_install_from = INST_FROM_LOCREP;
    }
    else if (!gt_remrep.empty()) {
        game_mod_install_from = INST_FROM_REMREP;
    }
    // else if (false)                  game_mod_install_from = INST_FROM_STEAM;
    else {
        // No Source to install. No return -1
    }

    ushort game_source =    INST_NO_SOURCE;
    ushort gt_source =      INST_NO_SOURCE;

    fs::path source_path;
    
    if (game_install_from == INST_FROM_LOCREP) {

        if (fs::is_regular_file(game_localrep)) {
            game_source = INST_FILE;
            source_path = game_localrep;
        }
        else if (fs::is_directory(game_localrep)) {
            game_source = INST_DIR;
            source_path = game_localrep;
        } else {
            game_install_from = INST_FROM_REMREP;
        }

        if (!fs::exists(source_path)) {
            std::cerr << "Local rep not found: " << source_path << std::endl;
            game_install_from = INST_FROM_REMREP;
            source_path = game_remrep;
        }
    }

    if (game_install_from == INST_FROM_REMREP) {
        // Check rep available
        // TODO ...
        game_source = INST_FILE;
        source_path = game_remrep;
    }

    if (game_install_from == INST_FROM_STEAM) {
        std::string steamcmd_fullpath = steamcmd_path + "/" + STEAMCMD;

        if (!fs::exists(steamcmd_fullpath)) {
            std::string error = "SteamCMD not found: " + steamcmd_fullpath;
            _append_cmd_output(error);
            std::cerr << error << std::endl;
            _set_installed(0);
            return -1;
        }

        std::string additional_parameters = "";

        if (!steam_app_set_config.empty()) {
            additional_parameters += "+app_set_config \"" + steam_app_set_config + "\"";
        }

        std::string steam_cmd_install = boost::str(boost::format("%1% +login anonymous +force_install_dir %2% +app_update \"%3%\" %4% validate +quit")
                                             % steamcmd_fullpath // 1
                                             % work_path // 2
                                             % steam_app_id // 3
                                             % additional_parameters // 4
        );

        bool steamcmd_install_success = false;
        uint tries = 0;
        while (tries <= 3) {
            _append_cmd_output("\nSteamCMD installation. Attempt #" + std::to_string(tries+1) + "\n");
            int result = _exec(steam_cmd_install);

            if (result == 0) {
                steamcmd_install_success = true;
                break;
            }

            // Exit code 8: Error! App 'xx' state is 0xE after update job.
            if (result != 1 && result != 8) {
                steamcmd_install_success = false;
                break;
            }

            ++tries;
        }

        if (!steamcmd_install_success) {
            std::string error = "Game installation via SteamCMD failed";
            _append_cmd_output(error);
            std::cerr << error << std::endl;

            _set_installed(0);
            return -1;
        }
    }

    // Mkdir
    if (!fs::exists(work_path)) {
        fs::create_directories(work_path);
    }

    // Wget/Copy and unpack
    if (game_install_from == INST_FROM_LOCREP && game_source == INST_FILE) {
        _unpack_archive(game_localrep);
    }
    else if (game_install_from == INST_FROM_LOCREP && game_source == INST_DIR) {
        _copy_dir(source_path, work_path);
    }
    else if (game_install_from == INST_FROM_REMREP) {
        std::string cmd = boost::str(boost::format("wget -N -c %1% -P %2% ") % source_path.string() % work_path.string());

        if (_exec(cmd) == -1) {
            _set_installed(0);
            return -1;
        }
        
        std::string archive = boost::str(boost::format("%1%/%2%") % work_path.string() % source_path.filename().string());
        
        _unpack_archive(archive);
        fs::remove(archive);
    }

    // Game Type Install

    if (game_mod_install_from == INST_FROM_LOCREP) {

        if (fs::is_regular_file(gt_localrep)) {
            gt_source = INST_FILE;
            source_path = gt_localrep;
        }
        else if (fs::is_directory(gt_localrep)) {
            gt_source = INST_DIR;
            source_path = gt_localrep;
        } else {
            game_mod_install_from = INST_FROM_REMREP;
        }

        if (!fs::exists(source_path)) {
            std::cerr << "Local rep not found: " << source_path << std::endl;
            game_mod_install_from = INST_FROM_REMREP;
            source_path = gt_remrep;
        }
    }

    if (game_mod_install_from == INST_FROM_REMREP) {
        // Check rep available
        // TODO ...
        gt_source = INST_FILE;
        source_path = gt_remrep;
    }

    // Wget/Copy and unpack
    if (game_mod_install_from == INST_FROM_LOCREP && gt_source == INST_FILE) {
        _unpack_archive(game_localrep);
    }
    else if (game_mod_install_from == INST_FROM_LOCREP && gt_source == INST_DIR) {
        _copy_dir(source_path, work_path);
    }
    else if (game_mod_install_from == INST_FROM_REMREP) {
        std::string cmd = boost::str(boost::format("wget -N -c %1% -P %2% ") % source_path.string() % work_path.string());

        if (_exec(cmd) == -1) {
            _set_installed(0);
            return -1;
        }
        
        std::string archive = boost::str(boost::format("%1%/%2%") % work_path.string() % source_path.filename().string());
        
        _unpack_archive(archive);
        fs::remove(archive);
    }

    #ifdef __linux__
        if (user != "") {
            std::string cmd = boost::str(boost::format("chown -R %1% %2%") % user % work_path.string());
            _exec(cmd);
        }
    #endif

    // Update installed = 1
    _set_installed(1);

    return 0;
}

void GameServer::_set_installed(unsigned int status)
{
    Json::Value jdata;
    jdata["installed"] = status;

    Gameap::Rest::put("/gdaemon_api/servers/" + std::to_string(server_id), jdata);
}

// ---------------------------------------------------------------------

int GameServer::_unpack_archive(fs::path const & archive)
{
    std::string cmd;

    #ifdef __linux__
        if (archive.extension().string() == ".xz")           cmd = boost::str(boost::format("tar -xpvJf %1% -C %2%") % archive.string() % work_path.string());
        else if (archive.extension().string() == ".gz")      cmd = boost::str(boost::format("tar -xvf %1% -C %2%") % archive.string() % work_path.string());
        else if (archive.extension().string() == ".bz2")     cmd = boost::str(boost::format("tar -xvf %1% -C %2%") % archive.string() % work_path.string());
        else if (archive.extension().string() == ".tar")     cmd = boost::str(boost::format("tar -xvf %1% -C %2%") % archive.string() % work_path.string());
        else cmd = boost::str(boost::format("unzip -o %1% -d %2%") % archive.string() % work_path.string());
    #elif _WIN32
        cmd = str(boost::format("7z x %1% -aoa -o%2%") % archive.string() % work_path.string());
    #endif

    if (_exec(cmd) == -1) {
        return -1;
    }
}

// ---------------------------------------------------------------------

int GameServer::_exec(std::string cmd, bool not_append)
{
    if (!not_append) {
        _append_cmd_output(fs::current_path().string() + "# " + cmd);
    }

    std::string out;
    int exit_code = exec(cmd, out);

    if (!not_append) {
        _append_cmd_output(out);
        _append_cmd_output(boost::str(boost::format("Exited with %1%") % exit_code));
    }

    return exit_code;
}

// ---------------------------------------------------------------------

int GameServer::_exec(std::string cmd)
{
    return _exec(cmd, false);
}

// ---------------------------------------------------------------------

bool GameServer::_copy_dir(
    fs::path const & source,
    fs::path const & destination
) {
    try {
        // Check whether the function call is valid
        if(!fs::exists(source) || !fs::is_directory(source)) {
            _append_cmd_output("Source dir not found:  " + source.string());
            std::cerr << "Source directory " << source.string()
                << " does not exist or is not a directory." << '\n';
            return false;
        }
        
        if (!fs::exists(destination)) {
            // Create the destination directory
            if (!fs::create_directory(destination)) {
                _append_cmd_output("Create failed:  " + destination.string());
                std::cerr << "Unable to create destination directory"
                    << destination.string() << '\n';
                return false;
            }
        }
        
    } catch(fs::filesystem_error const & e) {
        std::cerr << e.what() << std::endl;
        return false;
    }

    // Iterate through the source directory
    for(fs::directory_iterator file(source);
        file != fs::directory_iterator(); ++file
    ) {
        try {
            fs::path current(file->path());
            if(fs::is_directory(current)) {
                // Found directory: Recursion
                if(!_copy_dir(current, destination / current.filename())) {
                    return false;
                }
            } else {
                // Found file: Copy
                //_append_cmd_output("Copy " + current.string() + "  " + destination.string() + "/" + current.filename().string());
                
                if (fs::is_regular_file(current)) {
                    fs::copy_file(current, destination / current.filename(), fs::copy_option::overwrite_if_exists);
                }
                else {
                    fs::copy(current, destination / current.filename());
                }
            }
        } catch(fs::filesystem_error const & e) {
            std:: cerr << e.what() << std::endl;
        }
    }
    
    return true;
}

// ---------------------------------------------------------------------

/**
 * Delete Game serve
 *
 * @return 0 success, -1 error
 */
int GameServer::delete_server()
{
    if (status_server()) {
        if (stop_server() == -1) {
            return -1;
        }
    }

    // installed = 0.
    {
        Json::Value jdata;
        jdata["installed"] = 0;
        Gameap::Rest::put("/gdaemon_api/servers/" + std::to_string(server_id), jdata);
    }

    DedicatedServer& deds = DedicatedServer::getInstance();
    std::string delete_cmd  = deds.get_script_cmd(DS_SCRIPT_DELETE);

    if (delete_cmd.length() > 0) {
        replace_shortcodes(delete_cmd);
        int result = _exec(delete_cmd);
        return result;
    } else {
        try {
            std::cout << "Remove path: " << work_path << std::endl;
            fs::remove_all(work_path);
        }
        catch (fs::filesystem_error &e) {
            std::cerr << "Error remove: " << e.what() << std::endl;
            return -1;
        }
    }

    return 0;
}

// ---------------------------------------------------------------------

int GameServer::cmd_exec(std::string cmd)
{
    std::vector<std::string> split_lines;
    boost::split(split_lines, cmd, boost::is_any_of("\n\r"));

    for (std::vector<std::string>::iterator itl = split_lines.begin(); itl != split_lines.end(); ++itl) {
        if (*itl == "") continue;
        _exec(*itl);
    }

    return 0;
}

// ---------------------------------------------------------------------

/**
 * Check server status.
 *
 * @return  True if process active, False if process inactive
 */
bool GameServer::_server_status_cmd()
{
    DedicatedServer& deds = DedicatedServer::getInstance();
    std::string status_cmd  = deds.get_script_cmd(DS_SCRIPT_STATUS);
    replace_shortcodes(status_cmd);

    int result = _exec(status_cmd, true);

    return (result == 0);
}

// ---------------------------------------------------------------------

/**
 * Check server status.
 *
 * @return  True if process active, False if process inactive
 */
bool GameServer::status_server()
{
    try {
        _update_vars();
    } catch (std::exception &e) {
        std::cerr << "Server update vars error: " << e.what() << std::endl;
    }

    bool active = false;

    DedicatedServer& deds = DedicatedServer::getInstance();
    std::string status_cmd  = deds.get_script_cmd(DS_SCRIPT_STATUS);

    if (status_cmd.length() > 0) {
        active = _server_status_cmd();
    } else {
        fs::path p(work_path);
        p /= "pid.txt";

        char bufread[32];

        std::ifstream pidfile;
        pidfile.open(p.string(), std::ios::in);

        if (pidfile.good()) {
            pidfile.getline(bufread, 32);
            pidfile.close();

            uint pid = static_cast<uint>(atol(bufread));
            if (pid != 0) {
#ifdef __linux__
                active = (kill(pid, 0) == 0);
#elif _WIN32
                HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, pid);
            DWORD ret = WaitForSingleObject(process, 0);
            CloseHandle(process);
            active = (ret == WAIT_TIMEOUT);
#endif
            }
        } else {
            active = false;
        }
    }

    Json::Value jdata;
    jdata["process_active"] = (int)active;

    std::time_t now = std::time(nullptr);
    std::tm * ptm = std::localtime(&now);
    char buffer[32];
    std::strftime(buffer, 32, "%F %T", ptm);

    jdata["last_process_check"] = buffer;

    Gameap::Rest::put("/gdaemon_api/servers/" + std::to_string(server_id), jdata);

    return (bool)active;
}

// ---------------------------------------------------------------------

int GameServersList::update_list()
{
    Config& config = Config::getInstance();

    Json::Value jvalue;

    try {
        jvalue = Gameap::Rest::get("/gdaemon_api/servers?fields[servers]=id");
    } catch (Gameap::Rest::RestapiException &exception) {
        // Try later
        std::cerr << exception.what() << std::endl;
        return -1;
    }

    for (Json::ValueIterator itr = jvalue.begin(); itr != jvalue.end(); ++itr) {
        ulong server_id = (*itr)["id"].asUInt();

        if (servers_list.find(server_id) == servers_list.end()) {

            std::shared_ptr<GameServer> gserver = std::make_shared<GameServer>(server_id);

            try {
                servers_list.insert(
                    servers_list.end(),
                    std::pair<ulong, std::shared_ptr<GameServer>>(server_id, gserver)
                );
            } catch (std::exception &e) {
                std::cerr << "GameServer #" << server_id << " insert error: " << e.what() << std::endl;
            }
        }
    }

    return 0;
}

// ---------------------------------------------------------------------

void GameServersList::stats_process()
{
    for (std::map<ulong, std::shared_ptr<GameServer>>::iterator it = servers_list.begin(); it != servers_list.end(); ++it) {
        // Check status and start if server not active
        it->second->start_if_need();
    }
}

// ---------------------------------------------------------------------

GameServer * GameServersList::get_server(ulong server_id)
{
    if (servers_list.find(server_id) == servers_list.end()) {
        if (update_list() == -1) {
            return nullptr;
        }

        if (servers_list.find(server_id) == servers_list.end()) {
            return nullptr;
        }
    }

    if (servers_list[server_id] == nullptr) {
        return nullptr;
    }
    
    return servers_list[server_id].get();
}
