#define BOOST_NO_CXX11_SCOPED_ENUMS
#include "consts.h"

#include <boost/process.hpp>

#include <json/json.h>

#include "config.h"
#include "game_server.h"

#include <boost/filesystem.hpp>
#undef BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/format.hpp>

#include "log.h"

#include "game_server_installer.h"

#include "functions/restapi.h"
#include "functions/gsystem.h"
#include "functions/gstring.h"

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
    m_install_process = false;
    m_install_status_changed = 0;

    m_last_process_check = 0;

    m_staft_crash_disabled = false;
    m_cmd_output = std::make_shared<CmdOutput>();

    try {
        _update_vars(true);
    } catch (std::exception &e) {
        GAMEAP_LOG_ERROR << "Server update vars error: " << e.what();
    }
}

// ---------------------------------------------------------------------

void GameServer::_update_vars()
{
    _update_vars(false);
}

// ---------------------------------------------------------------------

void GameServer::_update_vars(bool force)
{
    if (!force && time(nullptr) - m_last_update_vars < TIME_UPDDIFF) {
        return;
    }

    if (!force && m_install_process) {
        return;
    }

    DedicatedServer& dedicatedServer = DedicatedServer::getInstance();

    Json::Value jvalue;

    try {
        jvalue = Gameap::Rest::get("/gdaemon_api/servers/" + std::to_string(m_server_id));
    } catch (Gameap::Rest::RestapiException &exception) {
        // Try later
        GAMEAP_LOG_ERROR << exception.what();
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

    m_mod_localrep    = jvalue["game_mod"]["local_repository"].asString();
    m_mod_remrep      = jvalue["game_mod"]["remote_repository"].asString();

    // Replace os shortcode
    m_game_localrep   = str_replace("{os}", OS, m_game_localrep);
    m_game_remrep     = str_replace("{os}", OS, m_game_remrep);
    m_mod_localrep     = str_replace("{os}", OS, m_mod_localrep);
    m_mod_remrep       = str_replace("{os}", OS, m_mod_remrep);

    m_staft_crash = jvalue["start_after_crash"].asBool();

    m_aliases.clear();

    try {
        Json::Value jaliases;

        Json::Reader jreader(Json::Features::strictMode());

        jaliases = jvalue["game_mod"]["vars"];

        for(auto itr = jaliases.begin() ; itr != jaliases.end() ; itr++ ) {

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
                GAMEAP_LOG_ERROR << "Unknown alias type: " << (*itr)["default"];
            }
        }

        Json::Value server_vars = jvalue["vars"];

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
                GAMEAP_LOG_ERROR << "Unknown alias type: " << (*itr);
            }
        }
    }
    catch (std::exception &e) {
        GAMEAP_LOG_ERROR << "Aliases error: " << e.what();
    }

    m_last_update_vars = time(nullptr);
}

// ---------------------------------------------------------------------

void GameServer::_append_cmd_output(const std::string &line)
{
    m_cmd_output->append(line);
}

// ---------------------------------------------------------------------

// size_t GameServer::get_cmd_output(std::string * output, size_t position)
int GameServer::get_cmd_output(std::string * str_out)
{
    m_cmd_output->get(str_out);
    return SUCCESS_STATUS_INT;
}

// ---------------------------------------------------------------------

std::string GameServer::get_cmd_output()
{
    std::string cmd_output;
    m_cmd_output->get(&cmd_output);
    return cmd_output;
}

// ---------------------------------------------------------------------

void GameServer::clear_cmd_output()
{
    m_cmd_output->clear();
}

// ---------------------------------------------------------------------

void GameServer::_replace_shortcodes(std::string &command)
{
    command = str_replace("{dir}", m_work_path.string(), command);

    command = str_replace("{uuid}", m_uuid, command);
    command = str_replace("{uuid_short}", m_uuid_short, command);

    command = str_replace("{host}", m_ip, command);
    command = str_replace("{ip}", m_ip, command);
    command = str_replace("{game}", m_game_scode, command);

    command = str_replace("{id}", std::to_string(m_server_id), command);
    command = str_replace("{port}", std::to_string(m_server_port), command);
    command = str_replace("{query_port}", std::to_string(m_query_port), command);
    command = str_replace("{rcon_port}", std::to_string(m_rcon_port), command);

    command = str_replace("{user}", m_user, command);

    // Aliases
    for (auto itr = m_aliases.begin(); itr != m_aliases.end(); ++itr) {
        command = str_replace("{" + itr->first + "}", itr->second, command);
    }
}

// ---------------------------------------------------------------------

int GameServer::start_server()
{
    DedicatedServer& deds = DedicatedServer::getInstance();
    std::string command  = str_replace("{command}", m_start_command, deds.get_script_cmd(DS_SCRIPT_START));
    _replace_shortcodes(command);

    m_staft_crash_disabled = false;

    change_euid_egid(m_user);
    int result = _exec(command);

    return (result == EXIT_SUCCESS_CODE) ? SUCCESS_STATUS_INT : ERROR_STATUS_INT;
}

// ---------------------------------------------------------------------

void GameServer::start_if_need()
{
    if (m_installed != SERVER_INSTALLED) {
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
    DedicatedServer& deds = DedicatedServer::getInstance();
    std::string command  = deds.get_script_cmd(DS_SCRIPT_STOP);
    _replace_shortcodes(command);

    m_staft_crash_disabled = true;
    
    int result = _exec(command);

    return (result == EXIT_SUCCESS_CODE) ? SUCCESS_STATUS_INT : ERROR_STATUS_INT;
}

// ---------------------------------------------------------------------

int GameServer::_update_server()
{
    if (status_server()) {
        if (stop_server() == ERROR_STATUS_INT) {
            return ERROR_STATUS_INT;
        }
    }

    _update_vars(true);
    _set_installed(SERVER_INSTALL_IN_PROCESS);

    GameServerInstaller installer(m_cmd_output);

    installer.m_game_localrep = m_game_localrep;
    installer.m_game_remrep = m_game_remrep;

    installer.m_mod_localrep = m_mod_localrep;
    installer.m_mod_remrep = m_mod_remrep;

    installer.m_steam_app_id = m_steam_app_id;
    installer.m_steam_app_set_config = m_steam_app_set_config;

    installer.m_server_absolute_path = m_work_path;
    installer.m_user = m_user;

    int install_result = installer.install_server();

    if (install_result != EXIT_SUCCESS_CODE) {
        _set_installed(SERVER_NOT_INSTALLED);
        return ERROR_STATUS_INT;
    } else {
        _set_installed(SERVER_INSTALLED);
        return SUCCESS_STATUS_INT;
    }
}

// ---------------------------------------------------------------------

int GameServer::update_server()
{
    int result;

    try {
        result = _update_server();
    } catch (std::exception &e) {
        _error(e.what());
        result = ERROR_STATUS_INT;
        _set_installed(SERVER_NOT_INSTALLED);
    }

    return result;
}

// ---------------------------------------------------------------------

void GameServer::_error(const std::string &msg)
{
    _append_cmd_output(msg);
    GAMEAP_LOG_ERROR << msg;
}

// ---------------------------------------------------------------------

void GameServer::_set_installed(unsigned int status)
{
    m_install_process = (status == SERVER_INSTALL_IN_PROCESS);
    m_install_status_changed = std::time(nullptr);
    m_installed = status;

    try {
        Json::Value jdata;
        jdata["installed"] = m_installed;
        Gameap::Rest::put("/gdaemon_api/servers/" + std::to_string(m_server_id), jdata);
    } catch (Gameap::Rest::RestapiException &exception) {
        GAMEAP_LOG_ERROR << exception.what();
    }
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

int GameServer::_exec(const std::string &command, bool not_append)
{
    if (!not_append) {
        _append_cmd_output(fs::current_path().string() + "# " + command);
    }

    int exit_code = exec(command, [this, not_append](std::string line) {
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

int GameServer::_exec(const std::string &command)
{
    return _exec(command, false);
}

// ---------------------------------------------------------------------

/**
 * Remove game server files
 *
 * @return 0 success, -1 error
 */
int GameServer::delete_files()
{
    if (status_server()) {
        if (stop_server() == ERROR_STATUS_INT) {
            return ERROR_STATUS_INT;
        }
    }

    _set_installed(SERVER_NOT_INSTALLED);

    DedicatedServer& deds = DedicatedServer::getInstance();
    std::string delete_cmd  = deds.get_script_cmd(DS_SCRIPT_DELETE);

    if (delete_cmd.length() > 0) {
        _replace_shortcodes(delete_cmd);
        int result = _exec(delete_cmd);

        if (result != EXIT_SUCCESS_CODE) {
            return ERROR_STATUS_INT;
        }
    } else {
        try {
            GAMEAP_LOG_DEBUG << "Remove path: " << m_work_path;
            fs::remove_all(m_work_path);
        }
        catch (fs::filesystem_error &e) {
            _error("Unable to remove: " + std::string(e.what()));
            return ERROR_STATUS_INT;
        }
    }

    return SUCCESS_STATUS_INT;
}

// ---------------------------------------------------------------------

int GameServer::cmd_exec(std::string command)
{
    std::vector<std::string> split_lines;
    boost::split(split_lines, command, boost::is_any_of("\n\r"));

    for (auto itl = split_lines.begin(); itl != split_lines.end(); ++itl) {
        if (*itl == "") {
            continue;
        }

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
    _replace_shortcodes(status_cmd);

    change_euid_egid(m_user);
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
    if (m_installed != SERVER_INSTALLED) {
        return false;
    }

    if (m_last_process_check > time(nullptr) - TIME_CACHE_STATUS) {
        return m_active;
    }

    try {
        _update_vars();
    } catch (std::exception &e) {
        GAMEAP_LOG_ERROR << "Server update vars error: " << e.what();
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

    return (bool)m_active;
}

// ---------------------------------------------------------------------

void GameServer::loop()
{
    _try_unblock();
    _update_vars();

    start_if_need();
}

// ---------------------------------------------------------------------

void GameServer::update()
{
    _update_vars(false);
}

// ---------------------------------------------------------------------

void GameServer::update(bool force)
{
    _update_vars(force);
}