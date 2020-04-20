#ifndef GDAEMON_STATUS_COMPONENT_H
#define GDAEMON_STATUS_COMPONENT_H

#include <cstring>
#include <cstdlib>

#include "daemon_server.h"

class StatusSession : public std::enable_shared_from_this<StatusSession> {
public:
    static constexpr auto END_SYMBOLS = "\xFF\xFF\xFF\xFF";

    static constexpr int STATUS_ERROR = 1;
    static constexpr int STATUS_CRITICAL_ERROR = 2;
    static constexpr int STATUS_UNKNOWN_COMMAND = 3;
    static constexpr int STATUS_OK = 100;

    static constexpr int COMMAND_VERSION            = 1;
    static constexpr int COMMAND_STATUS_BASE        = 2;
    static constexpr int COMMAND_STATUS_DETAILS     = 3;

    StatusSession(std::shared_ptr <Connection> connection) : m_connection(std::move(connection)) {};

    void start();
private:
    std::shared_ptr<Connection> m_connection;

    boost::asio::streambuf m_read_buffer;
    std::string m_read_msg;
    std::string m_write_msg;

    void do_read();
    void do_write();

    void cmd_process();
    void response_msg(unsigned int snum, const char * sdesc);
};


#endif //GDAEMON_STATUS_COMPONENT_H
