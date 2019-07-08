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
    m_server_id = mserver_id;
    m_last_update_vars = 0;

    m_staft_crash_disabled = false;
    m_cmd_output = std::make_shared<std::string> ("");

    try {
        _update_vars();
    } catch (std::exception &e) {
        std::cerr << "Server update vars error: " << e.what() << std::endl;
    }
}

// ---------------------------------------------------------------------

void GameServer::_update_vars()
{
    if (time(nullptr) - m_last_update_vars < TIME_UPDDIFF) {
        return;
    }

    if (m_install_process && m_install_status_changed < time(nullptr) - TIME_INSTALL_BLOCK) {
        return;
    }

    DedicatedServer& dedicatedServer = DedicatedServer::getInstance();

    Json::Value jvalue;

    try {
        jvalue = Gameap::Rest::get("/gdaemon_api/servers/" + std::to_string(m_server_id));
    } catch (Gameap::Rest::RestapiException &exception) {
        // Try later
        std::cerr << exception.what() << std::endl;
        return;
    }

    if (jvalue.empty()) {
        throw("Empty API response");
    }

    m_work_path = dedicatedServer.get_work_path();

    m_work_path /= jvalue["dir"].isString()
                 ? jvalue["dir"].asString()
                 : throw("Empty game server directory");

    m_installed = getJsonUInt(jvalue["installed"]);

    m_uuid            = jvalue["uuid"].asString();
    m_uuid_short      = jvalue["uuid_short"].asString();

    m_user            = jvalue["su_user"].asString();

    m_ip              = jvalue["server_ip"].asString();

    m_server_port     = getJsonUInt(jvalue["server_port"]);
    m_query_port      = getJsonUInt(jvalue["query_port"]);
    m_rcon_port       = getJsonUInt(jvalue["rcon_port"]);

    if (m_query_port == 0) {
        m_query_port = m_server_port;
    }

    if (m_rcon_port == 0) {
        m_rcon_port = m_server_port;
    }

    m_start_command   = jvalue["start_command"].asString();
    m_game_scode      = jvalue["code_name"].asString();

    m_game_localrep  = jvalue["game"]["local_repository"].asString();
    m_game_remrep    = jvalue["game"]["remote_repository"].asString();

    m_steam_app_id            = jvalue["game"]["steam_app_id"].asString();
    m_steam_app_set_config    = jvalue["game"]["steam_app_set_config"].asString();

    m_gt_localrep    = jvalue["game_mod"]["local_repository"].asString();
    m_gt_remrep      = jvalue["game_mod"]["remote_repository"].asString();

    // Replace os shortcode
    m_game_localrep   = str_replace("{os}", OS, m_game_localrep);
    m_game_remrep     = str_replace("{os}", OS, m_game_remrep);
    m_gt_localrep     = str_replace("{os}", OS, m_gt_localrep);
    m_gt_remrep       = str_replace("{os}", OS, m_gt_remrep);

    m_staft_crash = jvalue["start_after_crash"].asBool();

    m_aliases.clear();

    try {
        Json::Value jaliases;

        Json::Reader jreader(Json::Features::strictMode());

        jaliases = jvalue["game_mod"]["vars"];

        for( Json::ValueIterator itr = jaliases.begin() ; itr != jaliases.end() ; itr++ ) {

            if ((*itr)["default"].isNull()) {
                m_aliases.insert(std::pair<std::string, std::string>((*itr)["var"].asString(), ""));
            }
            else if ((*itr)["default"].isString()) {
                m_aliases.insert(std::pair<std::string, std::string>((*itr)["var"].asString(), (*itr)["default"].asString()));
            }
            else if ((*itr)["default"].isInt()) {
                m_aliases.insert(std::pair<std::string, std::string>((*itr)["var"].asString(), std::to_string((*itr)["default"].asInt())));
            }
            else {
                std::cerr << "Unknown alias type: " << (*itr)["default"] << std::endl;
            }
        }

        Json::Value server_vars = jvalue["vars"];;

        for( Json::ValueIterator itr = server_vars.begin() ; itr != server_vars.end() ; itr++ ) {
            if ((*itr).isString()) {
                if (m_aliases.find(itr.key().asString()) == m_aliases.end()) {
                    m_aliases.insert(std::pair<std::string, std::string>(itr.key().asString(), (*itr).asString()));
                } else {
                    m_aliases[itr.key().asString()] = (*itr).asString();
                }
            }
            else if ((*itr).isInt()) {
                if (m_aliases.find(itr.key().asString()) == m_aliases.end()) {
                    m_aliases.insert(std::pair<std::string, std::string>(itr.key().asString(), std::to_string((*itr).asInt())));
                } else {
                    m_aliases[itr.key().asString()] = std::to_string((*itr).asInt());
                }
            } else {
                std::cerr << "Unknown alias type: " << (*itr) << std::endl;
            }
        }
    }
    catch (std::exception &e) {
        std::cerr << "Aliases error: " << e.what() << std::endl;
    }

    m_last_update_vars = time(nullptr);
}

// ---------------------------------------------------------------------

void GameServer::_append_cmd_output(std::string line)
{
    m_cmd_output_mutex.lock();

    line.append("\n");
    (*m_cmd_output).append(line);

    m_cmd_output_mutex.unlock();
}

// ---------------------------------------------------------------------

// size_t GameServer::get_cmd_output(std::string * output, size_t position)
int GameServer::get_cmd_output(std::string * str_out)
{
    *str_out = *m_cmd_output;
    return SUCCESS_STATUS_INT;
}

// ---------------------------------------------------------------------

std::string GameServer::get_cmd_output()
{
    return *m_cmd_output;
}

// ---------------------------------------------------------------------

void GameServer::clear_cmd_output()
{
    m_cmd_output_mutex.lock();
    (*m_cmd_output).clear();
    m_cmd_output_mutex.unlock();
}

// ---------------------------------------------------------------------

void GameServer::replace_shortcodes(std::string &cmd)
{
    cmd = str_replace("{dir}", m_work_path.string(), cmd);

    cmd = str_replace("{uuid}", m_uuid, cmd);
    cmd = str_replace("{uuid_short}", m_uuid_short, cmd);

    cmd = str_replace("{host}", m_ip, cmd);
    cmd = str_replace("{ip}", m_ip, cmd);
    cmd = str_replace("{game}", m_game_scode, cmd);

    cmd = str_replace("{id}", std::to_string(m_server_id), cmd);
    cmd = str_replace("{port}", std::to_string(m_server_port), cmd);
    cmd = str_replace("{query_port}", std::to_string(m_query_port), cmd);
    cmd = str_replace("{rcon_port}", std::to_string(m_rcon_port), cmd);
    
    cmd = str_replace("{user}", m_user, cmd);

    // Aliases
    for (std::map<std::string, std::string>::iterator itr = m_aliases.begin(); itr != m_aliases.end(); ++itr) {
        cmd = str_replace("{" + itr->first + "}", itr->second, cmd);
    }
}

// ---------------------------------------------------------------------

int GameServer::start_server()
{
    if (status_server()) {
        // Server online
        return SUCCESS_STATUS_INT;
    }

    DedicatedServer& deds = DedicatedServer::getInstance();
    std::string cmd  = str_replace("{command}", m_start_command, deds.get_script_cmd(DS_SCRIPT_START));
    replace_shortcodes(cmd);

    m_staft_crash_disabled = false;

    int result = _exec(cmd);

    return (result == EXIT_SUCCESS_CODE) ? SUCCESS_STATUS_INT : ERROR_STATUS_INT;
}

// ---------------------------------------------------------------------

void GameServer::start_if_need()
{
    if (m_installed != SERVER_INTALLED) {
        return;
    }

    bool cur_status = status_server();
    
    if (!m_staft_crash) {
        return;
    }

    if (m_staft_crash_disabled) {
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
        //return SUCCESS_STATUS_INT;
    }

    DedicatedServer& deds = DedicatedServer::getInstance();
    // std::string cmd  = str_replace("{command}", "", deds.get_script_cmd(DS_SCRIPT_STOP));
    std::string cmd  = deds.get_script_cmd(DS_SCRIPT_STOP);
    replace_shortcodes(cmd);

    m_staft_crash_disabled = true;
    
    int result = _exec(cmd);

    return (result == EXIT_SUCCESS_CODE) ? SUCCESS_STATUS_INT : ERROR_STATUS_INT;
}

// ---------------------------------------------------------------------

int GameServer::update_server()
{
    if (status_server()) {
        if (stop_server() == ERROR_STATUS_INT) {
            return ERROR_STATUS_INT;
        }
    }

    _set_installed(SERVER_INTALL_IN_PROCESS);

    DedicatedServer& deds = DedicatedServer::getInstance();
    std::string update_cmd = deds.get_script_cmd(DS_SCRIPT_UPDATE);
    std::string steamcmd_path = deds.get_steamcmd_path();

    std::cout << "Server update starting..." << std::endl;

    if (update_cmd.length() > 0) {
        replace_shortcodes(update_cmd);
        int result = _exec(update_cmd);

        if (result == EXIT_SUCCESS_CODE) {
            _set_installed(SERVER_INTALLED);
            return SUCCESS_STATUS_INT;
        } else {
            _set_installed(SERVER_NOT_INSTALLED);
            return ERROR_STATUS_INT;
        }
    }

    ushort game_install_from =  INST_NO_SOURCE;
    ushort game_mod_install_from =    INST_NO_SOURCE;

    // Install game
    if (!m_game_localrep.empty()) {
        game_install_from = INST_FROM_LOCREP;
    }
    else if (!m_game_remrep.empty()) {
        game_install_from = INST_FROM_REMREP;
    }
    else if (!m_steam_app_id.empty() && !steamcmd_path.empty()) {
        game_install_from = INST_FROM_STEAM;
    }
    else {
        // No Source to install =(
        _error("No source to install game");

        _set_installed(SERVER_NOT_INSTALLED);
        return ERROR_STATUS_INT;
    }

    if (!m_gt_localrep.empty()) {
        game_mod_install_from = INST_FROM_LOCREP;
    }
    else if (!m_gt_remrep.empty()) {
        game_mod_install_from = INST_FROM_REMREP;
    }
    // else if (false)                  game_mod_install_from = INST_FROM_STEAM;
    else {
        // No Source to install. No return ERROR_STATUS_INT
    }

    ushort game_source =    INST_NO_SOURCE;
    ushort gt_source =      INST_NO_SOURCE;

    fs::path source_path;

    // Checking ability to install from local repository
    // Change source to Remote repository if local repository not found

    if (game_install_from == INST_FROM_LOCREP) {
        if (!fs::exists(m_game_localrep)) {
            _error("Local repository not found: " + m_game_localrep);

            game_install_from = INST_FROM_REMREP;
        } else {
            if (fs::is_regular_file(m_game_localrep)) {
                game_source = INST_FILE;
                source_path = m_game_localrep;
            }
            else if (fs::is_directory(m_game_localrep)) {
                game_source = INST_DIR;
                source_path = m_game_localrep;
            } else {
                game_install_from = INST_FROM_REMREP;
            }
        }
    }

    // Checking ability to install from remote repository
    // Change source to Steamcmd if remote repository is invalid

    if (game_install_from == INST_FROM_REMREP) {
        if (!m_game_remrep.empty()) {
            // Check rep available
            // TODO ...

            game_source = INST_FILE;
            source_path = m_game_remrep;
        } else if (!m_steam_app_id.empty()) {
            game_install_from = INST_FROM_STEAM;
        } else {
            _error("No source to install game");
            _set_installed(0);
            return ERROR_STATUS_INT;
        }
    }

    // Installation game server

    // Mkdir
    if (!fs::exists(m_work_path)) {
        fs::create_directories(m_work_path);
    }

    if (game_install_from == INST_FROM_STEAM) {
        std::string steamcmd_fullpath = steamcmd_path + "/" + STEAMCMD;

        if (!fs::exists(steamcmd_fullpath)) {
            _error("SteamCMD not found: " + steamcmd_fullpath);

            _set_installed(SERVER_NOT_INSTALLED);
            return ERROR_STATUS_INT;
        }

        std::string additional_parameters = "";

        if (!m_steam_app_set_config.empty()) {
            additional_parameters += "+app_set_config \"" + m_steam_app_set_config + "\"";
        }

#ifdef _WIN32
        // Add runas or gameap-starter to steamcmd command
        // SteamCMD installation fail without this command

        Config& config = Config::getInstance();

        if (fs::exists(fs::current_path() / "daemon" / "runas.exe")) {
            steamcmd_fullpath = fs::current_path().string() 
                + "\\daemon\\runas.exe -w:"
                + m_work_path.string() 
                + " " 
                + steamcmd_fullpath;
        }
        else {
            steamcmd_fullpath = config.path_starter
                + " -t run -d "
                + m_work_path.string()
                + " -c "
                + steamcmd_fullpath;
        }
#endif 
        std::string steam_cmd_install = boost::str(boost::format("%1% +login anonymous +force_install_dir %2% +app_update \"%3%\" %4% validate +quit")
                                             % steamcmd_fullpath // 1
                                             % m_work_path // 2
                                             % m_steam_app_id // 3
                                             % additional_parameters // 4
        );

        bool steamcmd_install_success = false;
        uint tries = 0;
        while (tries <= 3) {
            _append_cmd_output("\nSteamCMD installation. Attempt #" + std::to_string(tries+1) + "\n");
            int result = _exec(steam_cmd_install);

            if (result == EXIT_SUCCESS_CODE) {
                steamcmd_install_success = true;
                break;
            }

            // Exit code 8: Error! App 'xx' state is 0xE after update job.
            if (result != EXIT_ERROR_CODE && result != 8) {
                steamcmd_install_success = false;
                break;
            }

            ++tries;
        }

        if (!steamcmd_install_success) {
            _error("Game installation via SteamCMD failed");

            _set_installed(SERVER_NOT_INSTALLED);
            return ERROR_STATUS_INT;
        }
    }

    // Wget/Copy and unpack
    if (game_install_from == INST_FROM_LOCREP && game_source == INST_FILE) {
        if (_unpack_archive(source_path) == ERROR_STATUS_INT) {
            _error("Unable to unpack: " + source_path.string());

            return ERROR_STATUS_INT;
        }
    }
    else if (game_install_from == INST_FROM_LOCREP && game_source == INST_DIR) {
        if (_copy_dir(source_path, m_work_path) == false) {
            _error("Unable to copy from " + source_path.string() + " to " + m_work_path.string());
            return ERROR_STATUS_INT;
        }
    }
    else if (game_install_from == INST_FROM_REMREP) {
        std::string cmd = boost::str(boost::format("wget -N -c %1% -P %2% ") % source_path.string() % m_work_path.string());

        if (_exec(cmd) != EXIT_SUCCESS_CODE) {
            _set_installed(SERVER_NOT_INSTALLED);
            return ERROR_STATUS_INT;
        }
        
        std::string archive = boost::str(boost::format("%1%/%2%") % m_work_path.string() % source_path.filename().string());
        
        if (_unpack_archive(archive) == ERROR_STATUS_INT) {
            _error("Unable to unpack: " + source_path.string());

            fs::remove(archive);
            return ERROR_STATUS_INT;
        }

        fs::remove(archive);
    }

    // -----------------------------
    // Game Type Install

    if (game_mod_install_from == INST_FROM_LOCREP) {

        if (fs::is_regular_file(m_gt_localrep)) {
            gt_source = INST_FILE;
            source_path = m_gt_localrep;
        }
        else if (fs::is_directory(m_gt_localrep)) {
            gt_source = INST_DIR;
            source_path = m_gt_localrep;
        } else {
            game_mod_install_from = INST_FROM_REMREP;
        }

        if (!fs::exists(source_path)) {
            _error("Local rep not found:  " + source_path.string());
            game_mod_install_from = INST_FROM_REMREP;
            source_path = m_gt_remrep;
        }
    }

    if (game_mod_install_from == INST_FROM_REMREP) {
        // Check rep available
        // TODO ...
        gt_source = INST_FILE;
        source_path = m_gt_remrep;
    }

    // Wget/Copy and unpack
    if (game_mod_install_from == INST_FROM_LOCREP && gt_source == INST_FILE) {
        if (_unpack_archive(source_path) == ERROR_STATUS_INT) {
            _error( "Unable to unpack: " + source_path.string());
            return ERROR_STATUS_INT;
        }
    }
    else if (game_mod_install_from == INST_FROM_LOCREP && gt_source == INST_DIR) {
        if (_copy_dir(source_path, m_work_path) == false) {
            _error("Unable to copy from " + source_path.string() + m_work_path.string());
            std::cerr << "Unable to copy from " << source_path << " to " << m_work_path << std::endl;
            return ERROR_STATUS_INT;
        }
    }
    else if (game_mod_install_from == INST_FROM_REMREP) {
        std::string cmd = boost::str(boost::format("wget -N -c %1% -P %2% ") % source_path.string() % m_work_path.string());

        if (_exec(cmd) != EXIT_SUCCESS_CODE) {
            _set_installed(SERVER_NOT_INSTALLED);
            return ERROR_STATUS_INT;
        }
        
        std::string archive = boost::str(boost::format("%1%/%2%") % m_work_path.string() % source_path.filename().string());

        if (_unpack_archive(archive) == ERROR_STATUS_INT) {
            _error("Unable to unpack: " + source_path.string());
            fs::remove(archive);
            return ERROR_STATUS_INT;
        }

        fs::remove(archive);
    }

    #ifdef __linux__
        if (m_user != "") {
            _exec(boost::str(boost::format("chown -R %1% %2%") % m_user % m_work_path.string()));
            fs::permissions(m_work_path, fs::owner_all);
        }
    #endif

    // Run after install script if exist
    // After execution, the script will be deleted
    fs::path after_install_script = m_work_path / AFTER_INSTALL_SCRIPT;
    if (fs::exists(after_install_script))
    {
        int result = _exec(after_install_script.string());

        if (result != EXIT_SUCCESS_CODE) {
            _set_installed(SERVER_NOT_INSTALLED);
            return ERROR_STATUS_INT;
        }

        fs::remove(after_install_script);
    }

    // Update installed = 1
    _set_installed(SERVER_INTALLED);

    return SUCCESS_STATUS_INT;
}

// ---------------------------------------------------------------------

void GameServer::_error(std::string msg)
{
    _append_cmd_output(msg);
    std::cerr << msg << std::endl;
}

// ---------------------------------------------------------------------

void GameServer::_set_installed(unsigned int status)
{
    m_install_process = (status == SERVER_INTALL_IN_PROCESS);
    m_install_status_changed = std::time(nullptr);
    m_installed = status;
}

// ---------------------------------------------------------------------

/**
 * Timeout. Unblock some operations (eg installation)
 */
void GameServer::_try_unblock()
{
    if (m_install_process && m_install_status_changed < time(nullptr) - TIME_INSTALL_BLOCK) {
        _set_installed(SERVER_NOT_INSTALLED);
    }
}

// ---------------------------------------------------------------------

int GameServer::_unpack_archive(fs::path const & archive)
{
    std::string cmd;

    if (archive.extension().string() == ".rar") {
        std::string errorMsg = "RAR archive not supported. Use 7z, zip, tar, xz, gz or bz2 archives";
        _append_cmd_output(errorMsg);
        std::cerr << errorMsg << std::endl;
        return ERROR_STATUS_INT;
    }

    #ifdef __linux__
        if (archive.extension().string() == ".xz")           cmd = boost::str(boost::format("tar -xpvJf %1% -C %2%") % archive.string() % m_work_path.string());
        else if (archive.extension().string() == ".gz")      cmd = boost::str(boost::format("tar -xvf %1% -C %2%") % archive.string() % m_work_path.string());
        else if (archive.extension().string() == ".bz2")     cmd = boost::str(boost::format("tar -xvf %1% -C %2%") % archive.string() % m_work_path.string());
        else if (archive.extension().string() == ".tar")     cmd = boost::str(boost::format("tar -xvf %1% -C %2%") % archive.string() % m_work_path.string());
        else if (archive.extension().string() == ".zip")     cmd = boost::str(boost::format("unzip -o %1% -d %2%") % archive.string() % m_work_path.string());
        else cmd = boost::str(boost::format("7z x %1% -aoa -o%2%") % archive.string() % m_work_path.string());
    #elif _WIN32
        Config& config = Config::getInstance();

        cmd = boost::str(boost::format("%1% x %2% -aoa -o%3%") % config.path_7zip % archive.string() % m_work_path.string());
    #endif

    if (_exec(cmd) != EXIT_SUCCESS_CODE) {
        return ERROR_STATUS_INT;
    }
}

// ---------------------------------------------------------------------

int GameServer::_exec(std::string cmd, bool not_append)
{
    if (!not_append) {
        _append_cmd_output(fs::current_path().string() + "# " + cmd);
    }

    int exit_code = exec(cmd, [this, not_append](std::string line) {
        if (!not_append) {
            _append_cmd_output(line);
        }
    });

    if (!not_append) {
        _append_cmd_output(boost::str(boost::format("\nExited with %1%\n") % exit_code));
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
            return false;
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
        if (stop_server() == ERROR_STATUS_INT) {
            return ERROR_STATUS_INT;
        }
    }

    // installed = 0.
    {
        Json::Value jdata;
        jdata["installed"] = 0;
        Gameap::Rest::put("/gdaemon_api/servers/" + std::to_string(m_server_id), jdata);
    }

    DedicatedServer& deds = DedicatedServer::getInstance();
    std::string delete_cmd  = deds.get_script_cmd(DS_SCRIPT_DELETE);

    if (delete_cmd.length() > 0) {
        replace_shortcodes(delete_cmd);
        int result = _exec(delete_cmd);

        if (result != EXIT_SUCCESS_CODE) {
            return ERROR_STATUS_INT;
        }
    } else {
        try {
            std::cout << "Remove path: " << m_work_path << std::endl;
            fs::remove_all(m_work_path);
        }
        catch (fs::filesystem_error &e) {
            std::cerr << "Error remove: " << e.what() << std::endl;
            return ERROR_STATUS_INT;
        }
    }

    return SUCCESS_STATUS_INT;
}

// ---------------------------------------------------------------------

int GameServer::cmd_exec(std::string cmd)
{
    std::vector<std::string> split_lines;
    boost::split(split_lines, cmd, boost::is_any_of("\n\r"));

    for (std::vector<std::string>::iterator itl = split_lines.begin(); itl != split_lines.end(); ++itl) {
        if (*itl == "") continue;

        if (_exec(*itl) != EXIT_SUCCESS_CODE) {
            return ERROR_STATUS_INT;
        }
    }

    return SUCCESS_STATUS_INT;
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

    return (result == EXIT_SUCCESS_CODE);
}

// ---------------------------------------------------------------------

/**
 * Check server status.
 *
 * @return  True if process active, False if process inactive
 */
bool GameServer::status_server()
{
    if (m_installed != SERVER_INTALLED) {
        return false;
    }

    try {
        _update_vars();
    } catch (std::exception &e) {
        std::cerr << "Server update vars error: " << e.what() << std::endl;
    }

    DedicatedServer& deds = DedicatedServer::getInstance();
    std::string status_cmd  = deds.get_script_cmd(DS_SCRIPT_STATUS);

    if (status_cmd.length() > 0) {
        m_active = _server_status_cmd();
    } else {
        fs::path p(m_work_path);
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
                m_active = (kill(pid, 0) == 0);
#elif _WIN32
                HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, pid);
            DWORD ret = WaitForSingleObject(process, 0);
            CloseHandle(process);
            m_active = (ret == WAIT_TIMEOUT);
#endif
            }
        } else {
            m_active = false;
        }
    }

    m_last_process_check = std::time(nullptr);

    Json::Value jdata;
    jdata["process_active"] = (int)m_active;

    std::time_t now = std::time(nullptr);
    std::tm * ptm = std::localtime(&now);
    char buffer[32];
    std::strftime(buffer, 32, "%F %T", ptm);

    jdata["last_process_check"] = buffer;

    Gameap::Rest::put("/gdaemon_api/servers/" + std::to_string(m_server_id), jdata);

    return (bool)m_active;
}

// ---------------------------------------------------------------------

void GameServer::loop()
{
    _try_unblock();
    _update_vars();

    start_if_need();
}