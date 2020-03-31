#ifndef GDAEMON_GAME_SERVER_CMD_H
#define GDAEMON_GAME_SERVER_CMD_H

#include <boost/filesystem.hpp>

#include "cmd_output.h"
#include "models/server.h"
#include "classes/game_servers_list.h"

namespace GameAP {
    class GameServerCmd {
        public:
            GameServerCmd(unsigned short command, unsigned int server_id) :
                m_command(command),
                m_server_id(server_id) {

                GameServersList& gslist = GameServersList::getInstance();
                // m_gserver = gslist.get_server(server_id);

                m_complete = false;
            };

            static constexpr auto START     = 1;
            static constexpr auto PAUSE     = 2;
            static constexpr auto UNPAUSE   = 3;
            static constexpr auto STATUS    = 4;
            static constexpr auto STOP      = 5;
            static constexpr auto KILL      = 6;
            static constexpr auto RESTART   = 7;
            static constexpr auto UPDATE    = 8;
            static constexpr auto INSTALL   = 9;
            static constexpr auto REINSTALL = 10;
            static constexpr auto DELETE    = 11;

            void execute();

            bool is_complete() const;
            bool result() const;
            void output(std::string *str_out);


        protected:
            unsigned short m_command;
            unsigned int m_server_id;
            Server m_server;
            bool m_complete;
            bool m_result;
            std::shared_ptr<CmdOutput> m_output;

            // Commands
            bool start();
            // bool pause();
            // bool unpause();
            bool status();
            bool stop();
            // void kill();
            bool restart();
            bool update();
            // bool install();
            // bool reinstall();
            bool remove();

        private:
            int cmd_exec(const std::string &command);
            void replace_shortcodes(std::string &command);
    };
}
#endif //GDAEMON_GAME_SERVER_CMD_H
