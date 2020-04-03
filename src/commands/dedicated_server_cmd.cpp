#include "dedicated_server_cmd.h"

using namespace GameAP;

void DedicatedServerCmd::execute()
{
    switch (this->m_command) {
        case CMD_EXECUTE:
            this->m_result = this->cmd_execute();
            break;

        default:
            this->m_output->append("Invalid dedicated_server_cmd command");
            this->m_result = false;
            break;
    }

    this->m_complete = true;
}

bool DedicatedServerCmd::cmd_execute()
{
    if (this->shell_command.empty()) {
        this->m_output->append("Empty shell command");
        return false;
    }

    int result = this->cmd_exec(this->shell_command);
    return result == EXIT_SUCCESS_CODE;
}