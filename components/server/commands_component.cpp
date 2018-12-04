#include <cstdlib>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/filesystem.hpp>

#include <binn.h>

#include "commands_component.h"
#include "functions/gsystem.h"

namespace fs = boost::filesystem;

/**
 * Start session
 */
void CommandsSession::start()
{
    do_read();
}

/**
 * Read
 */
void CommandsSession::do_read()
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
            }
        });
}

/**
 * Write message
 */
void CommandsSession::do_write()
{
    m_write_msg.append(END_SYMBOLS);

    auto self(shared_from_this());
    boost::asio::async_write(*m_connection->socket, boost::asio::buffer(m_write_msg.c_str(), m_write_msg.length()),
        [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                m_write_msg.erase();
                do_read();
            }
        });

    do_read();
}

/**
 * Main command operations
 */
void CommandsSession::cmd_process()
{
    binn *read_binn;
    read_binn = binn_open((void*)&m_read_msg[0]);

    unsigned char command;

    if (!binn_list_get_uint8(read_binn, 1, &command)) {
        response_msg(STATUS_ERROR, "Invalid Binn data: reading command failed", true);
        return;
    }

    switch (command) {
        case COMMAND_EXEC: {
            char * exec_command;
            char * work_dir;

            if (!binn_list_get_str(read_binn, 2, &exec_command)) {
                response_msg(STATUS_ERROR, "Invalid exec command", true);
                break;
            }

            if (!binn_list_get_str(read_binn, 3, &work_dir)) {
                response_msg(STATUS_ERROR, "Invalid work directory", true);
                break;
            }

            if (strlen(work_dir) == 0) {
                response_msg(STATUS_ERROR, "Empty work directory", true);
                break;
            }

            if (!fs::is_directory(work_dir)) {
                response_msg(STATUS_ERROR, "Invalid work directory", true);
                break;
            }

            fs::current_path(work_dir);

            std::cout << "Executing command: \"" << exec_command << "\"" << std::endl;
            std::string out;
            int exit_code = GameAP::exec(std::string(exec_command), out);

            binn *write_binn = binn_list();
            binn_list_add_uint32(write_binn, STATUS_OK);
            binn_list_add_int32(write_binn, exit_code);
            binn_list_add_str(write_binn, &out[0]);

            m_write_msg = std::string(static_cast<char*>(binn_ptr(write_binn)), static_cast<uint>(binn_size(write_binn)));

            break;
        };

        default: {
            std::cerr << "Unknown Command" << std::endl;
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
void CommandsSession::response_msg(unsigned int snum, const char * sdesc, bool write)
{
    binn *write_binn = binn_list();
    binn_list_add_uint32(write_binn, snum);
    binn_list_add_str(write_binn, (char *)sdesc);

    m_write_msg = std::string(static_cast<char*>(binn_ptr(write_binn)), static_cast<uint>(binn_size(write_binn)));
}