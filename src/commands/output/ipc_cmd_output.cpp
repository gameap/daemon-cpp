#include "ipc_cmd_output.h"

#include <vector>

using namespace GameAP;

void IpcCmdOutput::append(const std::string &line)
{
    std::vector<char> chars(line.c_str(), line.c_str() + line.size());
    chars.push_back('\n');

    for (const auto &c: chars) {
        this->m_buf[this->m_write_pos++] = c;

        if (this->m_readable < IPC_OUTPUT_BUFFER_SIZE) {
            this->m_readable++;
        }

        if (this->m_write_pos >= IPC_OUTPUT_BUFFER_SIZE) {
            this->m_write_pos = 0;
        }
    }
}

void IpcCmdOutput::get(std::string *str_out)
{
    auto readable = this->m_readable;
    auto read_pos = this->m_read_pos;

    str_out->clear();

    while (readable > 0) {

        if (read_pos >= IPC_OUTPUT_BUFFER_SIZE) {
            read_pos = 0;
        }

        str_out->append(1, this->m_buf[read_pos]);

        read_pos++;
        readable--;
    }
}

void IpcCmdOutput::clear()
{
    this->m_readable = 0;
    this->m_read_pos = this->m_write_pos;
}