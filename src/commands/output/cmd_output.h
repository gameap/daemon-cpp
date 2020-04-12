#ifndef GDAEMON_CMD_OUTPUT_H
#define GDAEMON_CMD_OUTPUT_H

#include "base_output.h"

namespace GameAP {
    class CmdOutput: public BaseOutput {
        public:
            CmdOutput() {
                this->init();
            };

            /**
             * Initial output
             */
            void init() {
                m_output = std::make_shared<std::string> ("");
            }

            /**
             * Append output
             * @param line
             */
            void append(const std::string &line);

            /**
             * Get output
             * @param str_out
             */
            void get(std::string *str_out);

            /**
             * Clear output buffer
             */
            void clear();
        private:
            std::mutex m_mutex;
            std::shared_ptr<std::string> m_output;
    };
}
#endif //GDAEMON_CMD_OUTPUT_H
