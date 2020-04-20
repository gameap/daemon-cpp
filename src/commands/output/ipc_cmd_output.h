#ifndef GDAEMON_IPC_CMD_OUTPUT_H
#define GDAEMON_IPC_CMD_OUTPUT_H

#include "base_output.h"

#ifndef IPC_OUTPUT_BUFFER_SIZE
#define IPC_OUTPUT_BUFFER_SIZE 2000000
#endif

namespace GameAP {

    /**
     * Inter Process Communications (IPC) Output
     */
    class IpcCmdOutput : public BaseOutput {
        public:
            IpcCmdOutput() {
                this->init();
            }

            void init() {
                this->m_read_pos  = 0;
                this->m_write_pos = 0;
                this->m_readable  = 0;
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
             * Clear output buffer
             */
            void clear();

        private:
            std::mutex m_mutex{};
            char m_buf[IPC_OUTPUT_BUFFER_SIZE + 1]{};
            unsigned int m_read_pos{};
            unsigned int m_write_pos{};

            unsigned int m_readable;
    };
}

#endif