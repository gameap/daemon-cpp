#ifndef GDAEMON_CMD_H
#define GDAEMON_CMD_H

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include "functions/gsystem.h"

namespace fs = boost::filesystem;

namespace GameAP {

    class Cmd {
        public:

            /**
             * Execute command
             */
            virtual void execute() = 0;

            /**
             * Return true if command is completed. Otherwise false
             * @return bool
             */
            bool is_complete() const
            {
                return this->m_complete;
            }

            /**
             * Return true if command successfully completed. Otherwise false
             * @return bool
             */
            bool result() const
            {
                return this->m_result;
            }

            /**
             * Write an clear command output buffer
             * @param str_out  string to write
             */
            void output(std::string *str_out)
            {
                this->m_output->get(str_out);
                this->m_output->clear();
            }

        protected:
            /**
             * Current command
             */
            unsigned char m_command;

            bool m_complete;
            bool m_result;
            std::shared_ptr<CmdOutput> m_output;

            /**
             * Execute shell command
             * @param command
             * @return  command exit code (0 success, 1 fail, 2 critical error, ...)
             */
            int cmd_exec(const std::string &command)
            {
                this->m_output->append(fs::current_path().string() + "# " + command);

                // TODO: Coroutines here
                int exit_code = exec(command, [this](std::string line) {
                    this->m_output->append(line);
                });

                this->m_output->append(boost::str(boost::format("\nExited with %1%\n") % exit_code));

                return exit_code;
            }
    };
}

#endif //GDAEMON_CMD_H
