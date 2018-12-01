#ifndef DAEMON_SERVER_H
#define DAEMON_SERVER_H

#define DAEMON_SERVER_H

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <fstream>

#include <binn.h>

#define MSG_END_SYMBOLS_NUM 4

typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;

// ---------------------------------------------------------------------

class DaemonServerSess : public std::enable_shared_from_this<DaemonServerSess> {
public:
    DaemonServerSess(boost::asio::io_service& io_service, boost::asio::ssl::context& context)
            : socket_(io_service, context)
    {};

    void start();
    ssl_socket::lowest_layer_type& socket();
    void handle_handshake(const boost::system::error_code& error);

private:
    int append_end_symbols(char * buf, size_t length);
    void do_write(bool rsa_crypt);
    void do_read();
    size_t read_complete(size_t length);

    enum { max_length = 1024 };
    size_t read_length;
    char read_buf[max_length];
    std::string pub_keyfile;

    binn *write_binn;
    ssl_socket socket_;

    ushort sessid;
    ushort mode;

};

// ---------------------------------------------------------------------

class DaemonServer
{
public:
DaemonServer(boost::asio::io_service& io_service, short port)
        : acceptor_(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v6(), port)),
          //socket_(io_service, context_)
          io_service_(io_service),
        context_(boost::asio::ssl::context::sslv23)
{
        context_.set_options(
                boost::asio::ssl::context::default_workarounds
                | boost::asio::ssl::context::no_sslv2
                | boost::asio::ssl::context::single_dh_use);

        //context_.set_password_callback(boost::bind(&DaemonServer::get_password, this));

        context_.use_certificate_chain_file("/home/nikita/Git/GDaemon2/keys/ssl/user.crt");
        context_.use_private_key_file("/home/nikita/Git/GDaemon2/keys/ssl/user.key", boost::asio::ssl::context::pem);
        context_.use_tmp_dh_file("/home/nikita/Git/GDaemon2/keys/ssl/dh2048.pem");

        start_accept();
};

private:
    void start_accept();
    void handle_accept(std::shared_ptr<DaemonServerSess> session, const boost::system::error_code& error);

    boost::asio::io_service& io_service_;
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::ssl::context context_;
};

// ---------------------------------------------------------------------

int run_server(int port);

#endif
