#include <cstdlib>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/filesystem.hpp>

#include <binn.h>

#include "status_component.h"
#include "functions/gsystem.h"

#include "log.h"
#include "consts.h"

namespace fs = boost::filesystem;

/**
 * Start session
 */
void StatusSession::start()
{
    do_read();
}

/**
 * Read
 */
void StatusSession::do_read()
{
    auto self(shared_from_this());
    boost::asio::async_read_until(*m_connection->socket, m_read_buffer, END_SYMBOLS,
                                  [this, self](boost::system::error_code ec, std::size_t length) {
                                      if (!ec) {
                                          m_read_msg = std::string(
                                                  boost::asio::buffers_begin(m_read_buffer.data()),
                                                  boost::asio::buffers_end(m_read_buffer.data()) - strlen(END_SYMBOLS));

                                          m_read_buffer.consume(length);
                                          cmd_process();
                                      } else if (ec != boost::asio::error::operation_aborted) {
                                          (*m_connection->socket).shutdown();
                                      }
                                  });
}

/**
 * Write message
 */
void StatusSession::do_write()
{
    m_write_msg.append(END_SYMBOLS);

    auto self(shared_from_this());
    boost::asio::async_write(*m_connection->socket, boost::asio::buffer(m_write_msg.c_str(), m_write_msg.length()),
                             [this, self](boost::system::error_code ec, std::size_t length) {
                                 if (!ec) {
                                     m_write_msg.erase();
                                     do_read();
                                 } else if (ec != boost::asio::error::operation_aborted) {
                                     (*m_connection->socket).shutdown();
                                 }
                             });
}

/**
 * Main command operations
 */
void StatusSession::cmd_process()
{
    binn *read_binn;
    read_binn = binn_open((void*)&m_read_msg[0]);

    unsigned char command;

    if (!binn_list_get_uint8(read_binn, 1, &command)) {
        response_msg(STATUS_ERROR, "Invalid Binn data: reading command failed", true);
        return;
    }

    switch (command) {

        /*
         * Write
         *  GameAP Daemon version
         *  GameAP Daemon Compilation date
         */
        case COMMAND_VERSION: {
            binn *write_binn = binn_list();
            binn_list_add_uint32(write_binn, STATUS_OK);
            binn_list_add_str(write_binn, GAMEAP_DAEMON_VERSION); // Version
            binn_list_add_str(write_binn, __DATE__ " " __TIME__); // Compilation date
            break;
        };

        /*
         * Write
         *  GameAP Daemon uptime
         *  The number of working tasks
         *  The number of waiting tasks
         *  The number of working servers
         */
        case COMMAND_STATUS_BASE: {
            // TODO: Implement
            break;
        };

        /*
         * Write
         *  GameAP Daemon uptime
         *  The id list of working tasks
         *  The id list of waiting tasks
         *  The id list of working servers
         */
        case COMMAND_STATUS_DETAILS: {
            // TODO: Implement
            break;
        };

        default: {
            GAMEAP_LOG_WARNING << "Unknown Command";
            response_msg(STATUS_UNKNOWN_COMMAND, "Unknown command", true);
            return;
        };
    }

    do_write();
}

/**
 * Response message
 *
 * @param snum message code (1 - error, 2 - critical error, 3 - unknown command, 100 - okay)
 * @param sdesc text message
 * @param write
 */
void StatusSession::response_msg(unsigned int snum, const char * sdesc, bool write)
{
    binn *write_binn = binn_list();
    binn_list_add_uint32(write_binn, snum);
    binn_list_add_str(write_binn, (char *)sdesc);

    m_write_msg = std::string(static_cast<char*>(binn_ptr(write_binn)), static_cast<uint>(binn_size(write_binn)));
}