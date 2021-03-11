#ifndef GDAEMON_DEDICATED_SERVER_CMD_H
#define GDAEMON_DEDICATED_SERVER_CMD_H

#include "commands/output/cmd_output.h"
#include "cmd.h"

namespace GameAP {
    class DedicatedServerCmd: public Cmd {
        public:
            DedicatedServerCmd(unsigned char command)
            {
                m_command = command;
                m_complete = false;

                this->create_output();
            };

            void destroy() { };

            // Commands
            static constexpr unsigned char CMD_EXECUTE = 1;

            // Command Options
            static constexpr unsigned char OPTION_SHELL_COMMAND = 1;

            void execute();

        protected:
            // Commands
            bool cmd_execute();
    };

}

#endif //GDAEMON_DEDICATED_SERVER_CMD_H
