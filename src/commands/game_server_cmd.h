#ifndef GDAEMON_GAME_SERVER_CMD_H
#define GDAEMON_GAME_SERVER_CMD_H

#include <boost/filesystem.hpp>

#include "classes/cmd_output.h"
#include "models/server.h"
#include "classes/game_servers_list.h"
#include "cmd.h"

namespace GameAP {
    class GameServerCmd: public Cmd {
        public:
            GameServerCmd(unsigned char command, unsigned int server_id)
            {
                m_server_id = server_id;
                m_command = command;

                GameServersList& gslist = GameServersList::getInstance();
                m_server = *gslist.get_server(server_id);

                m_output = std::make_shared<CmdOutput>();

                m_complete = false;
            };

            static constexpr unsigned char START     = 1;
            static constexpr unsigned char PAUSE     = 2;
            static constexpr unsigned char UNPAUSE   = 3;
            static constexpr unsigned char STATUS    = 4;
            static constexpr unsigned char STOP      = 5;
            static constexpr unsigned char KILL      = 6;
            static constexpr unsigned char RESTART   = 7;
            static constexpr unsigned char UPDATE    = 8;
            static constexpr unsigned char INSTALL   = 9;
            static constexpr unsigned char REINSTALL = 10;
            static constexpr unsigned char DELETE    = 11;

            void execute();

        protected:
            unsigned int m_server_id;
            Server m_server;

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
            void replace_shortcodes(std::string &command);
    };
}
#endif //GDAEMON_GAME_SERVER_CMD_H
