#ifndef FILE_SERVER_H
#define FILE_SERVER_H

#define FILE_SERVER_H

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>

#include <fstream>

#include <binn.h>

// ---------------------------------------------------------------------

class FileServerSess : public std::enable_shared_from_this<FileServerSess> {
public:
    FileServerSess(boost::asio::ip::tcp::socket socket) : socket_(std::move(socket)) {};
    void start();

private:
    void do_read();
    void do_write();

    void cmd_process();

    void open_file();
    void write_file(size_t length);
    void close_file();

    boost::asio::ip::tcp::socket socket_;
    enum { max_length = 1024 };
    
    std::size_t read_length;
    char read_buf[max_length];
    
    binn *write_binn;
    char *aes_key;

    /*
     * 0 - NoAuth
     * 1 - Auth
     * 2 - Cmd
     * 3 - File send
     * */
    int mode;

    boost::asio::streambuf request_buf;
    char *filename;
    std::ofstream output_file;
};

// ---------------------------------------------------------------------

class FileServer
{
public:
FileServer(boost::asio::io_service& io_service, short port)
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

int run_file_server(int port);

#endif
