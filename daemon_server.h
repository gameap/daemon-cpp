#ifndef DAEMON_SERVER_H
#define DAEMON_SERVER_H

#define DAEMON_SERVER_H

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>

#include <fstream>

#include <binn.h>

// ---------------------------------------------------------------------

class DaemonServerSess : public std::enable_shared_from_this<DaemonServerSess> {
public:
    DaemonServerSess(boost::asio::ip::tcp::socket socket) : socket_(std::move(socket)) {};

    void start();

private:
    int append_end_symbols(char * buf, size_t length);
    void do_write();
    void do_read();
    size_t read_complete(size_t length);
    
    enum { max_length = 1024 };
    size_t read_length;
    char read_buf[max_length];

    binn *write_binn;

    boost::asio::ip::tcp::socket socket_;

    ushort sessid = 0;
    ushort mode;

};

// ---------------------------------------------------------------------

class DaemonServer
{
public:
DaemonServer(boost::asio::io_service& io_service, short port)
        : acceptor_(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
        socket_(io_service)
{
        do_accept();
};

private:
    void do_accept();
    
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::ip::tcp::socket socket_;
};

// ---------------------------------------------------------------------

int run_server(int port);

#endif
