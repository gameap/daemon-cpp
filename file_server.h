#ifndef FILE_SERVER_H
#define FILE_SERVER_H

#define FILE_SERVER_H

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>

#include <binn.h>

class session : public std::enable_shared_from_this<session> {
public:
  session(boost::asio::ip::tcp::socket socket) : socket_(std::move(socket)) {};
  void start();

private:
  void do_read();
  void do_write();

  boost::asio::ip::tcp::socket socket_;
  enum { max_length = 1024 };
  
  std::size_t read_length;
  char read_buf[max_length];
  
  binn *write_binn;
  char *aes_key;
};

class server
{
public:
server(boost::asio::io_service& io_service, short port)
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

int run_file_server(int port);

#endif
