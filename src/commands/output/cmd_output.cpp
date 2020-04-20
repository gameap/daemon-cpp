#include "cmd_output.h"

using namespace GameAP;

void CmdOutput::append(const std::string &line)
{
    m_mutex.lock();

    m_output->append(line);
    m_output->append("\n");

    m_mutex.unlock();
}

void CmdOutput::get(std::string *str_out)
{
    *str_out = *m_output;
}

void CmdOutput::clear()
{
    m_mutex.lock();
    m_output->clear();
    m_mutex.unlock();
}