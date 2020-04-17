#include "game_server_installer.h"

#include "functions/gsystem.h"
#include "consts.h"
#include "config.h"

#include "log.h"
#include "classes/dedicated_server.h"

#ifdef __linux__
#define STEAMCMD "steamcmd.sh"
#elif _WIN32
#define STEAMCMD "steamcmd.exe"
#endif

using namespace GameAP;

namespace fs = boost::filesystem;
namespace bp = ::boost::process;

/**
 * Run game server installation
 * @return int
 */
int GameServerInstaller::install_server()
{
    GAMEAP_LOG_VERBOSE << "Installation information. "
        << " m_server_absolute_path: "   << m_server_absolute_path
        << " m_steam_app_id: "           << m_steam_app_id
        << " m_steam_app_set_config: "   << m_steam_app_set_config
        << " m_game_localrep: "          << m_game_localrep
        << " m_game_remrep: "            << m_game_remrep
        << " m_mod_localrep: "           << m_mod_localrep
        << " m_mod_remrep: "             << m_mod_remrep;

    if (_detect_sources() == ERROR_STATUS_INT) {
        return ERROR_STATUS_INT;
    }

    GAMEAP_LOG_VERBOSE << "Detect information. "
        << " m_game_source_type: "   << m_game_source_type
        << " m_mod_source_type: "    << m_mod_source_type
        << " m_game_type: "          << m_game_type
        << " m_mod_type: "           << m_mod_type;

    if (!fs::exists(m_server_absolute_path)) {
        fs::create_directories(m_server_absolute_path);
    }

    fs::current_path(m_server_absolute_path);

    int install_result;

    install_result = _install_game();

    if (install_result != SUCCESS_STATUS_INT) {
        return ERROR_STATUS_INT;
    }

    install_result = _install_mod();

    if (install_result != SUCCESS_STATUS_INT) {
        return ERROR_STATUS_INT;
    }

    fs::path after_install_script = m_server_absolute_path / AFTER_INSTALL_SCRIPT;

    if (fs::exists(after_install_script)) {

        // Need a retrieve privileges
        privileges_retrieve();
        int result = _exec(after_install_script.string());

#ifdef __linux__
        if (m_user != "" && getuid() == 0) {
            _exec(boost::str(
                    boost::format("chown -R %1% %2%")
                    % m_user
                    % m_server_absolute_path.string()
            ));
            fs::permissions(m_server_absolute_path, fs::owner_all);
        }
#endif

        privileges_down(this->m_user);

        if (result != EXIT_SUCCESS_CODE) {
            return ERROR_STATUS_INT;
        }

        fs::remove(after_install_script);
    }

    return SUCCESS_STATUS_INT;
}

/**
 * Detect game and mod sources
 *
 * Available game sources:
 *      SteamCMD
 *      Local repository
 *      Remote repository
 *
 * Available mod sources:
 *      Local repository
 *      Remote repository
 *
 * @return
 */
int GameServerInstaller::_detect_sources()
{
    bool game_source_detected = false;
    while (! game_source_detected) {
        _detect_game_source();

        if (m_game_source_type == INST_NO_SOURCE) {
            _error("No source to install game");
            return ERROR_STATUS_INT;
        }

        if (m_game_source_type == INST_FROM_LOCREP
            && _detect_localrep_type(_get_game_source()) == INST_TYPE_INVALID
        ) {
            m_ignored_game_source.insert(m_game_source_type);
            m_game_source_type = INST_NO_SOURCE;
        } else {
            game_source_detected = true;
        }
    }

    bool mod_source_detected = false;
    while (!mod_source_detected) {
        _detect_mod_source();

        if (m_mod_source_type == INST_NO_SOURCE) {
            break;
        }

        if (m_mod_source_type == INST_FROM_LOCREP
            && _detect_localrep_type(_get_mod_source()) == INST_TYPE_INVALID
        ) {
            m_ignored_mod_source.insert(m_mod_source_type);
            m_mod_source_type = INST_NO_SOURCE;
        } else {
            mod_source_detected = true;
        }
    }
    
    return SUCCESS_STATUS_INT;
}

void GameServerInstaller::_detect_game_source()
{
    DedicatedServer& deds = DedicatedServer::getInstance();
    std::string steamcmd_path = deds.get_steamcmd_path();

    std::string steamcmd_fullpath = steamcmd_path + "/" + STEAMCMD;

    if (! m_game_localrep.empty()
        && m_ignored_game_source.find(INST_FROM_LOCREP) == m_ignored_game_source.end()
    ) {
        m_game_source_type = INST_FROM_LOCREP;
    } else if (! m_game_remrep.empty()
               && m_ignored_game_source.find(INST_FROM_REMREP) == m_ignored_game_source.end()
    ) {
        m_game_source_type = INST_FROM_REMREP;
    } else if (m_steam_app_id > 0
               && ! steamcmd_path.empty()
               && m_ignored_game_source.find(INST_FROM_STEAM) == m_ignored_game_source.end()
    ) {
        m_game_source_type = INST_FROM_STEAM;
    }
}

void GameServerInstaller::_detect_mod_source()
{
    if (! m_mod_localrep.empty()) {
        m_mod_source_type = INST_FROM_LOCREP;
    }
    else if (! m_mod_remrep.empty()) {
        m_mod_source_type = INST_FROM_REMREP;
    }
    else {
        // No Source to install. No return ERROR_STATUS_INT
    }
}

int GameServerInstaller::_detect_localrep_type(const fs::path& path)
{
    if (path.empty()) {
        return INST_TYPE_INVALID;
    }

    if (!fs::exists(path)) {
        GAMEAP_LOG_ERROR << "Local repository not found: "
                         << m_game_localrep;

        return INST_TYPE_INVALID;
    }

    if (fs::is_regular_file(path)) {
        return INST_TYPE_FILE;
    }
    else if (fs::is_directory(m_game_localrep)) {
        return INST_TYPE_DIR;
    } else {
        return INST_TYPE_INVALID;
    }
}

void GameServerInstaller::_error(const std::string& msg)
{
    m_error.push_back(msg);
    GAMEAP_LOG_ERROR << msg;
}

std::string GameServerInstaller::get_errors()
{
    std::string errors;

    for (auto line: m_error) {
        errors.append(line + "\n");
    }

    return errors;
}

std::string GameServerInstaller::_get_game_source()
{
    switch (m_game_source_type) {
        case(INST_FROM_LOCREP):
            return m_game_localrep;
        case(INST_FROM_REMREP):
            return m_game_remrep;
        default:
            return "";
    }
}

std::string GameServerInstaller::_get_mod_source()
{
    switch (m_mod_source_type) {
        case(INST_FROM_LOCREP):
            return m_mod_localrep;
        case(INST_FROM_REMREP):
            return m_mod_remrep;
        default:
            return "";
    }
}

int GameServerInstaller::_install_game()
{
    int install_result;

    if (m_game_source_type == INST_FROM_STEAM) {
        install_result = _install_game_steam();
    }
    else if (m_game_source_type == INST_FROM_LOCREP) {
        install_result = _install_locrep(_get_game_source(), m_game_type);
    }
    else if (m_game_source_type == INST_FROM_REMREP) {
        install_result = _install_remrep(_get_game_source());
    } else {
        _error("Invalid game installation source type");
        install_result = ERROR_STATUS_INT;
    }

    if (install_result != SUCCESS_STATUS_INT) {
        return ERROR_STATUS_INT;
    } else {
		return SUCCESS_STATUS_INT;
	}
}

int GameServerInstaller::_install_mod()
{
    int install_result;

    if (m_mod_source_type == INST_FROM_LOCREP) {
        install_result = _install_locrep(_get_mod_source(), m_mod_type);
    }
    else if (m_mod_source_type == INST_FROM_REMREP) {
        install_result = _install_remrep(_get_mod_source());
    }
    else {
        install_result = SUCCESS_STATUS_INT;
    }

    return install_result;
}

int GameServerInstaller::_install_game_steam()
{
    DedicatedServer& deds = DedicatedServer::getInstance();
    std::string steamcmd_path = deds.get_steamcmd_path();

    std::string steamcmd_fullpath = steamcmd_path + "/" + STEAMCMD;

    if (!fs::exists(steamcmd_fullpath)) {
        _error("SteamCMD not found: " + steamcmd_fullpath);
        return ERROR_STATUS_INT;
    }

    std::string additional_parameters;

    if (!m_steam_app_set_config.empty()) {
        additional_parameters += "+app_set_config \"" + m_steam_app_set_config + "\"";
    }

    #ifdef _WIN32
        // Add runas or gameap-starter to steamcmd command
        // SteamCMD installation fail without this command

        Config& config = Config::getInstance();
        fs::path ds_work_path = deds.get_work_path();

        if (fs::exists(ds_work_path / "daemon" / "runas.exe")) {
            steamcmd_fullpath = ds_work_path.string()
                + "\\daemon\\runas.exe -w:"
                + m_server_absolute_path.string()
                + " "
                + steamcmd_fullpath;
        }
        else {
            steamcmd_fullpath = config.path_starter
                + " -t run -d "
                + m_server_absolute_path.string()
                + " -c "
                + steamcmd_fullpath;
        }
    #endif

    std::string steam_cmd_install = boost::str(
        boost::format("%1% +login anonymous +force_install_dir %2% +app_update \"%3%\" %4% validate +quit")
        % steamcmd_fullpath // 1
        % m_server_absolute_path // 2
        % m_steam_app_id // 3
        % additional_parameters // 4
    );

    bool steamcmd_install_success = false;
    uint tries = 0;
    while (tries <= 3) {
        // TODO: Implement appending
        // _append_cmd_output("\nSteamCMD installation. Attempt #" + std::to_string(tries+1) + "\n");
        int result = _exec(steam_cmd_install);

        if (result == EXIT_SUCCESS_CODE) {
            steamcmd_install_success = true;
            break;
        }

        // Exit code 8: Error! App 'xx' state is 0xE after update job.
        if (result != EXIT_ERROR_CODE && result != 7 && result != 8) {
            steamcmd_install_success = false;
            break;
        }

        ++tries;
    }

    if (!steamcmd_install_success) {
        _error("Game installation via SteamCMD failed");
        return ERROR_STATUS_INT;
    } else {
        return SUCCESS_STATUS_INT;
    }
}

int GameServerInstaller::_install_locrep(const fs::path &path, int type)
{
    if (type == INST_TYPE_FILE) {
        return _unpack_archive(path);
    }
    else if (type == INST_TYPE_DIR) {
        return !copy_dir(path, m_server_absolute_path);
    } else {
        _error("Invalid local repository installation type");
        return ERROR_STATUS_INT;
    }
}

int GameServerInstaller::_install_remrep(const fs::path &uri)
{
    std::string cmd = boost::str(
            boost::format("wget -N -c %1% -P %2% ")
                % uri.string()
                % m_server_absolute_path.string()
    );

    if (_exec(cmd) != EXIT_SUCCESS_CODE) {
        return ERROR_STATUS_INT;
    }

    std::string archive = boost::str(
            boost::format("%1%/%2%")
                % m_server_absolute_path.string()
                % uri.filename().string()
    );

    int unpack_result = _unpack_archive(archive);
    fs::remove(archive);

    return (unpack_result == EXIT_SUCCESS_CODE)
        ? EXIT_SUCCESS_CODE
        : ERROR_STATUS_INT;
}

int GameServerInstaller::_unpack_archive(const fs::path &archive) {
    std::string unpack_cmd;

    if (archive.extension().string() == ".rar") {
        std::string errorMsg = "RAR archive not supported. Use 7z, zip, tar, xz, gz or bz2 archives";
        _error(errorMsg);
        return ERROR_STATUS_INT;
    }

#ifdef __linux__
    if (archive.extension().string() == ".xz") {
        unpack_cmd = boost::str(
                boost::format("tar -xpvJf %1% -C %2%")
                    % archive.string()
                    % m_server_absolute_path.string()
        );
    }
    else if (archive.extension().string() == ".gz") {
        unpack_cmd = boost::str(
                boost::format("tar -xvf %1% -C %2%")
                    % archive.string()
                    % m_server_absolute_path.string()
        );
    }
    else if (archive.extension().string() == ".bz2") {
        unpack_cmd = boost::str(
                boost::format("tar -xvf %1% -C %2%")
                    % archive.string()
                    % m_server_absolute_path.string()
        );
    }
    else if (archive.extension().string() == ".tar") {
        unpack_cmd = boost::str(
                boost::format("tar -xvf %1% -C %2%")
                    % archive.string()
                    % m_server_absolute_path.string()
        );
    }
    else if (archive.extension().string() == ".zip") {
        unpack_cmd = boost::str(
                boost::format("unzip -o %1% -d %2%")
                    % archive.string()
                    % m_server_absolute_path.string()
        );
    }
    else {
        unpack_cmd = boost::str(
                boost::format("7z x %1% -aoa -o%2%")
                    % archive.string()
                    % m_server_absolute_path.string()
        );
    }
#elif _WIN32
    Config& config = Config::getInstance();
    unpack_cmd = boost::str(boost::format("%1% x %2% -aoa -o%3%") % config.path_7zip % archive.string() % m_server_absolute_path.string());
#endif

    if (_exec(unpack_cmd) != EXIT_SUCCESS_CODE) {
        return ERROR_STATUS_INT;
    } else {
		return SUCCESS_STATUS_INT;
	}
}

int GameServerInstaller::_exec(const std::string &command, bool enable_append)
{
    if (enable_append) {
        m_cmd_output->append(fs::current_path().string() + "# " + command);
    }

    int exit_code = exec(command, [this, enable_append](std::string line) {
        if (enable_append) {
            m_cmd_output->append(line);
        }
    });

    if (enable_append) {
        m_cmd_output->append(boost::str(boost::format("\nExited with %1%\n") % exit_code));
    }

    return exit_code;
}

void GameServerInstaller::set_user(const std::string &user)
{
    this->m_user = user;
    privileges_down(user);
}

int GameServerInstaller::_exec(const std::string &command)
{
    return _exec(command, true);
}
