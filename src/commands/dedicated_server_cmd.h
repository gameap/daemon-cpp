#ifndef GDAEMON_DEDICATED_SERVER_CMD_H
#define GDAEMON_DEDICATED_SERVER_CMD_H

#include "classes/cmd_output.h"
#include "cmd.h"

namespace GameAP {
    class DedicatedServerCmd: public Cmd {
        public:
            DedicatedServerCmd(unsigned char command)
            {
                m_command = command;
                m_output = std::make_shared<CmdOutput>();
                m_complete = false;
            };

            static constexpr unsigned char CMD_EXECUTE = 1;

            void execute();

            // CMD_EXECUTE option
            void set_shell_command(const std::string &cmd) {
                this->shell_command = cmd;
            }

        protected:
            // Commands
            bool cmd_execute();

        private:
            std::string shell_command;
    };

}

#endif //GDAEMON_DEDICATED_SERVER_CMD_H
