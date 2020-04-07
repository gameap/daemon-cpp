#include "game_server_cmd.h"

#include "functions/gstring.h"
#include "functions/gsystem.h"

#include "game_server_installer.h"
#include "classes/dedicated_server.h"

#include "log.h"

using namespace GameAP;

void GameServerCmd::execute()
{
    if (this->m_complete) {
        // Do nothing. Cmd is already complete
        return;
    }

    if (this->m_server == nullptr) {
        this->m_output->append("Invalid server");

        this->m_complete = true;
        this->m_result = false;
        return;
    }

    switch (this->m_command) {
        case START:
            this->m_result = this->start();
            break;

        case STOP:
        case KILL:
            this->m_result = this->stop();
            break;

        case RESTART:
            this->m_result = this->restart();
            break;
        case UPDATE:
        case INSTALL:
            this->m_result = this->update();
            break;

        case REINSTALL:
            this->m_result =
                    this->remove() && this->update();
            break;

        case DELETE:
            this->m_result = this->remove();
            break;

        case STATUS:
            this->m_result = this->status();
            break;

        default:
            this->m_output->append("Invalid game_server_cmd command");
            this->m_result = false;
            break;
    }

    GameServersList& gslist = GameServersList::getInstance();
    gslist.sync_all();

    this->m_complete = true;
}

bool GameServerCmd::start()
{
    DedicatedServer& ds = DedicatedServer::getInstance();

    std::string command = str_replace(
            "{command}",
            this->m_server->start_command,
            ds.get_script_cmd(DS_SCRIPT_START)
    );

    this->replace_shortcodes(command);

    // TODO: Replace value to std::variant type after C++17 changing
    if (this->m_server->get_setting("autostart") == "1" || this->m_server->get_setting("autostart") == "true") {
        this->m_server->set_setting("autostart_current", "1");
    }

    int result = this->unprivileged_exec(command);
    return (result == EXIT_SUCCESS_CODE);
}

bool GameServerCmd::status()
{
    DedicatedServer& ds = DedicatedServer::getInstance();
    std::string command  = ds.get_script_cmd(DS_SCRIPT_STATUS);
    this->replace_shortcodes(command);

    bool is_active = false;

    if (command.length() > 0) {
        int result = this->unprivileged_exec(command);
        is_active = (result == EXIT_SUCCESS_CODE);
    } else {
        fs::path work_path = ds.get_work_path();
        work_path /= this->m_server->dir;

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
                    is_active = (kill(pid, 0) == 0);
                #elif _WIN32
                    HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, pid);
                    DWORD ret = WaitForSingleObject(process, 0);
                    CloseHandle(process);
                    is_active = (ret == WAIT_TIMEOUT);
                #endif
            }
        } else {
            is_active = false;
        }
    }

    return is_active;
}

bool GameServerCmd::stop()
{
    DedicatedServer& ds = DedicatedServer::getInstance();
    std::string command  = ds.get_script_cmd(DS_SCRIPT_STOP);

    this->replace_shortcodes(command);

    if (this->m_server->autostart()) {
        // TODO: Replace value to std::variant type after C++17 changing
        this->m_server->set_setting("autostart_current", "0");
    }

    int result = this->unprivileged_exec(command);
    return result == EXIT_SUCCESS_CODE;
}

bool GameServerCmd::restart()
{
    bool stop_result = true;

    if (this->status()) {
        stop_result = this->stop();
    }

    return
        stop_result && this->start();
}

bool GameServerCmd::update()
{
    if (this->status() && ! this->stop()) {
        this->m_output->append("Unable to stop server");
        return false;
    }

    DedicatedServer& ds = DedicatedServer::getInstance();
    GameServerInstaller installer(this->m_output);

    installer.m_game_localrep        = this->m_server->game.local_repository;
    installer.m_game_remrep          = this->m_server->game.remote_repository;

    installer.m_mod_localrep         = this->m_server->game_mod.local_repository;
    installer.m_mod_remrep           = this->m_server->game_mod.remote_repository;

    installer.m_steam_app_id         = this->m_server->game.steam_app_id;
    installer.m_steam_app_set_config = this->m_server->game.steam_app_set_config;

    fs::path work_path = ds.get_work_path();
    work_path /= this->m_server->dir;

    installer.m_server_absolute_path = work_path;

    installer.set_user(this->m_server->user);

    this->m_server->installed = Server::SERVER_INSTALL_IN_PROCESS;

    int result = installer.install_server();

    if (result != EXIT_SUCCESS_CODE) {
        this->m_server->installed = Server::SERVER_NOT_INSTALLED;
        this->m_output->append(installer.get_errors());
        return false;
    }

    this->m_server->installed = Server::SERVER_INSTALLED;
    return true;
}

bool GameServerCmd::remove()
{
    if (!this->stop()) {
        return false;
    }

    DedicatedServer& ds = DedicatedServer::getInstance();
    std::string command  = ds.get_script_cmd(DS_SCRIPT_DELETE);

    if (command.length() > 0) {
        this->replace_shortcodes(command);
        int result = this->unprivileged_exec(command);
        return (result == EXIT_SUCCESS_CODE);
    } else {
        fs::path work_path = ds.get_work_path();
        work_path /= this->m_server->dir;

        try {
            GAMEAP_LOG_DEBUG << "Remove path: " << work_path;
            this->m_output->append("Removing " + work_path.string());
            fs::remove_all(work_path);
        }
        catch (fs::filesystem_error &e) {
            this->m_output->append("Unable to remove: " + std::string(e.what()));
            return false;
        }
    }

    this->m_server->installed = Server::SERVER_NOT_INSTALLED;

    return true;
}

void GameServerCmd::replace_shortcodes(std::string &command)
{
    DedicatedServer& dedicatedServer = DedicatedServer::getInstance();

    fs::path work_path = dedicatedServer.get_work_path();
    work_path /= this->m_server->dir;

    command = str_replace("{dir}", work_path.string(), command);

    command = str_replace("{uuid}", this->m_server->uuid, command);
    command = str_replace("{uuid_short}", this->m_server->uuid_short, command);

    command = str_replace("{host}", this->m_server->ip, command);
    command = str_replace("{ip}", this->m_server->ip, command);
    command = str_replace("{game}", this->m_server->game.start_code, command);

    command = str_replace("{id}", std::to_string(this->m_server_id), command);
    command = str_replace("{port}", std::to_string(this->m_server->connect_port), command);
    command = str_replace("{query_port}", std::to_string(this->m_server->query_port), command);
    command = str_replace("{rcon_port}", std::to_string(this->m_server->rcon_port), command);

    command = str_replace("{user}", this->m_server->user, command);

    for (const auto& var: this->m_server->vars) {
        command = str_replace("{" + var.first + "}", var.second, command);
    }
}

int GameServerCmd::unprivileged_exec(std::string &command)
{
    privileges_down(this->m_server->user);
    int result = this->cmd_exec(command);
    privileges_retrieve();

    return result;
}