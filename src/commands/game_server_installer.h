#ifndef GDAEMON_GAME_SERVER_INSTALLER_H
#define GDAEMON_GAME_SERVER_INSTALLER_H

#include <unordered_set>
#include <mutex>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include "commands/output/base_output.h"

#define INST_NO_SOURCE      0
#define INST_FROM_LOCREP    1
#define INST_FROM_REMREP    2
#define INST_FROM_STEAM     3

#define INST_TYPE_INVALID   0
#define INST_TYPE_FILE      1
#define INST_TYPE_DIR       2

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

    class GameServerInstaller {
        public:
            GameServerInstaller(const std::shared_ptr<BaseOutput>& cmd_output)
                : m_cmd_output{cmd_output}
            {
                m_game_source_type = INST_NO_SOURCE;
                m_mod_source_type  = INST_NO_SOURCE;

                m_game_type = INST_TYPE_INVALID;
                m_mod_type  = INST_TYPE_INVALID;
            }

            std::string m_game_localrep;
            std::string m_game_remrep;
            std::string m_mod_localrep;
            std::string m_mod_remrep;

            unsigned int m_steam_app_id;
            std::string m_steam_app_set_config;

            /**
             * Absolute path to game server catalog
             */
            fs::path m_server_absolute_path;

            /**
             * Server path owner
             */
            std::string m_user;

            /**
             * Set user and down privileges
             * @param username
             */
            void set_user(const std::string & user);

            /**
             * Run installation of server
             * @return int
             */
            int install_server();

            /**
             * Return errors list
             * @return
             */
            std::string get_errors();
        private:
            int m_game_source_type;
            int m_mod_source_type;

            int m_game_type;
            int m_mod_type;

            std::vector<std::string> m_error;

            std::unordered_set<int> m_ignored_game_source;
            std::unordered_set<int> m_ignored_mod_source;

            std::shared_ptr<BaseOutput> m_cmd_output;

            int _detect_sources();
            void _detect_game_source();
            void _detect_mod_source();

            /**
             * Detect local repository source (File or directory)
             */
            int _detect_localrep_type(const fs::path &path);

            int _install_game();
            int _install_mod();

            int _install_locrep(const fs::path &path, int type);
            int _install_remrep(const fs::path &uri);
            int _install_game_steam();

            int _unpack_archive(const fs::path &archive);

            std::string _get_game_source();
            std::string _get_mod_source();

            int _exec(const std::string &command, bool enable_append);
            int _exec(const std::string &command);

            void _error(const std::string& msg);
    };
}
#endif //GDAEMON_GAME_SERVER_INSTALLER_H
