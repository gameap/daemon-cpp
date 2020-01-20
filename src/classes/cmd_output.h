#ifndef GDAEMON_CMD_OUTPUT_H
#define GDAEMON_CMD_OUTPUT_H

#include <memory>
#include <string>
#include <mutex>

namespace GameAP {
    class CmdOutput {
        public:
            CmdOutput() {
                m_output = std::make_shared<std::string> ("");
            };

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
             *
             */
            void clear();
        private:
            std::mutex m_mutex;
            std::shared_ptr<std::string> m_output;
    };
}
#endif //GDAEMON_CMD_OUTPUT_H
