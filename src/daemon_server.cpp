#include <chrono>

#include "log.h"
#include "consts.h"

#include "components/server/files_component.h"
#include "components/server/commands_component.h"
#include "daemon_server.h"
#include "classes/dedicated_server.h"
#include "functions/auth.h"

#include "status.h"

using namespace GameAP;

// ---------------------------------------------------------------------

ssl_socket::lowest_layer_type& DaemonServerSess::socket()
{
    return connection_->socket->lowest_layer();
}

// ---------------------------------------------------------------------

void DaemonServerSess::handle_handshake(const boost::system::error_code& error) {
    if (!error) {
        do_read();
    } else {
        GAMEAP_LOG_ERROR << "Error: (" << error << ") " << error.message();
    }
}

// ---------------------------------------------------------------------

void DaemonServerSess::start ()
{
    mode = DAEMON_SERVER_MODE_NOAUTH;

    read_length = 0;

    (*connection_->socket).async_handshake(boost::asio::ssl::stream_base::server,
                            boost::bind(&DaemonServerSess::handle_handshake, shared_from_this(),
                                        boost::asio::placeholders::error));
}

// ---------------------------------------------------------------------

void DaemonServerSess::do_read()
{
    switch (mode) {
        case DAEMON_SERVER_MODE_NOAUTH: {

            auto self(shared_from_this());
            (*connection_->socket).async_read_some(boost::asio::buffer(read_buf, max_length),
                [this, self](boost::system::error_code ec, std::size_t length) {
                    if (!ec) {
                        read_length += length;

                        if (read_complete(length)) {
                            // Check auth
                            binn *read_binn;
                            read_binn = binn_open(&read_buf[0]);

                            char *login;
                            char *password;

                            if (!binn_list_get_str(read_binn, 2, &login)
                                || !binn_list_get_str(read_binn, 3, &password)
                            ) {
                                GAMEAP_LOG_ERROR << "Incorrect binn data";
                                binn_free(read_binn);
                                return; 
                            }

                            if (gameap::auth::check(login, password)) {
                                binn_list_add_uint32(m_write_binn, 100);
                                binn_list_add_str(m_write_binn, (char *)"Auth success");

                                if (!binn_list_get_uint16(read_binn, 4, &mode)) {
                                    mode = DAEMON_SERVER_MODE_NOAUTH;
                                    GAMEAP_LOG_ERROR << "Incorrect binn data";
                                    binn_free(read_binn);
                                    return; 
                                }

                                GAMEAP_LOG_INFO << "Auth success";
                            }
                            else {
                                binn_list_add_uint32(m_write_binn, 2);
                                binn_list_add_str(m_write_binn, (char *)"Auth failed");
                            }

                            binn_free(read_binn);
                            do_write();
                        }
                    } else if (ec != boost::asio::error::operation_aborted) {
                        (*connection_->socket).shutdown();
                    }
                    else {
                        GAMEAP_LOG_ERROR << "ERROR: " << ec;
                    }
            });
            break;
        }

        case DAEMON_SERVER_MODE_AUTH:
            GAMEAP_LOG_DEBUG << "AUTH";
            break;

        case DAEMON_SERVER_MODE_CMD:
            GAMEAP_LOG_DEBUG << "DAEMON_SERVER_MODE_CMD";
            std::make_shared<CommandsSession>(std::move(connection_))->start();
            break;

        case DAEMON_SERVER_MODE_FILES:
            GAMEAP_LOG_DEBUG << "DAEMON_SERVER_MODE_FILES";
            std::make_shared<FileServerSess>(std::move(connection_))->start();
            break;
    }
}

// ---------------------------------------------------------------------

size_t DaemonServerSess::read_complete(size_t length)
{
    if (read_length <= 4) return 0;

    int found = 0;
    for (auto i = read_length; i > read_length-length && found < MSG_END_SYMBOLS_NUM; i--) {
        if (read_buf[i-1] == '\xFF') found++;
    }

    return (found >= 4) ?  1: 0;
}

// ---------------------------------------------------------------------

int DaemonServerSess::append_end_symbols(char * buf, size_t length)
{
    if (length == 0) return -1;

    for (auto i = length; i < length + MSG_END_SYMBOLS_NUM; i++) {
        buf[i] = '\xFF';
    }

    buf[length + MSG_END_SYMBOLS_NUM] = '\x00';
    return length + MSG_END_SYMBOLS_NUM;
}

// ---------------------------------------------------------------------

void DaemonServerSess::do_write()
{
    read_length = 0;
    memset(read_buf, 0, max_length-1);

    auto self(shared_from_this());
	char sendbin[10240];

    size_t write_len = 0;

    memcpy(sendbin, (char*)binn_ptr(m_write_binn), binn_size(m_write_binn));
    write_len = binn_size(m_write_binn);

    size_t len = 0;
    len = append_end_symbols(&sendbin[0], write_len);

    boost::asio::async_write(*connection_->socket, boost::asio::buffer(sendbin, len),
        [this, self](boost::system::error_code ec, std::size_t) {
            if (!ec) {
                do_read();
            } else if (ec != boost::asio::error::operation_aborted) {
                (*connection_->socket).shutdown();
            }
    });
}

// ---------------------------------------------------------------------

void DaemonServer::start_accept()
{
    auto connection = std::make_shared<Connection>(io_service_, context_);
    std::shared_ptr<DaemonServerSess> session = std::make_shared<DaemonServerSess>(std::move(connection));

    acceptor_.async_accept(session->socket(),
                           boost::bind(&DaemonServer::handle_accept, this, session,
                                       boost::asio::placeholders::error)
    );
}

// ---------------------------------------------------------------------

void DaemonServer::handle_accept(std::shared_ptr<DaemonServerSess> session, const boost::system::error_code& error) {
    if (!error) {
        session->start();

        GAMEAP_LOG_INFO << "Client connected: "
                  << session->socket().remote_endpoint().address().to_string()
                  << ":"
                  << session->socket().remote_endpoint().port();

        auto connection = std::make_shared<Connection>(io_service_, context_);
        session = std::make_shared<DaemonServerSess>(std::move(connection));

        acceptor_.async_accept(session->socket(),
                               boost::bind(&DaemonServer::handle_accept, this, session,
                                           boost::asio::placeholders::error)
        );
    } else {
        GAMEAP_LOG_ERROR << "Handle Accept error: (" << error << ") " << error.message();
    }
}
// ---------------------------------------------------------------------

int run_server(const std::string& ip, const ushort port)
{
    try {
        boost::asio::io_service io_service{DaemonServer::THREADS_NUM};

        boost::asio::ip::tcp::endpoint endpoint = ip.empty()
                                              ? boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v6(), port)
                                              : boost::asio::ip::tcp::endpoint(
                                                      boost::asio::ip::address::from_string(ip),
                                                      port);

        DaemonServer s(io_service, endpoint);

        std::vector<std::thread> v_services;
        v_services.reserve(DaemonServer::THREADS_NUM);

        auto run_closure = [&io_service] {
            while(status_active) {
                io_service.run_for(std::chrono::seconds(5));
            }
        };

        for (auto i = DaemonServer::THREADS_NUM - 1; i > 0; --i) {
            v_services.emplace_back(run_closure);
        }

        for (std::thread & v_service: v_services) {
            v_service.detach();
        }

        run_closure();
    }
    catch (std::exception& e) {
        GAMEAP_LOG_ERROR << "Daemon Server. Exception: " << e.what();
        return 1;
    }


    return 0;
}
