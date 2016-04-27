#include "file_server.h"

#include "functions/gcrypt.h"

using boost::asio::ip::tcp;

void session::start ()
{
    write_binn = binn_list();
    aes_key = "12345678901234561234567890123456";
    do_read();
}

void session::do_read()
{
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(read_buf, max_length),
        [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {

                read_length += length;

                if (strstr(read_buf, "\xFF\xFF\xFF\xFF") != nullptr) {
                    binn_free(write_binn);
                    write_binn = binn_list();
                    
                    std::string msg(read_buf, read_length-4);
                    binn_list_add_str(write_binn, &msg[0]);
                    
                    read_length = 0;
                    memset(read_buf, 0, max_length-1);
                    
                    do_write();
                }
            }
    });
}

void session::do_write()
{
    auto self(shared_from_this());
    char *msg;

    size_t len = 0;
    
    std::cout << "BIN: " << (char *)binn_ptr(write_binn) << std::endl;
    std::cout << "BLAT 1: " << msg << std::endl;
    msg = GCrypt::aes_encrypt((char*)binn_ptr(write_binn), aes_key);
    std::cout << "MSG 2: " << msg << std::endl;
    
    // sprintf(msg, "%s%s", msg, "\xFF\xFF\xFF\xFF");
    
    strcat(msg, "\xFF\xFF\xFF\xFF");
    std::cout << "SEND: " << msg << std::endl;

    // std::cout << "DECRYPT: " << msg << std::endl;

    boost::asio::async_write(socket_, boost::asio::buffer(msg, strlen(msg)),
        [this, self](boost::system::error_code ec, std::size_t) {
            if (!ec) {
                do_read();
            }
    });
}

void server::do_accept()
{
    acceptor_.async_accept(socket_, [this](boost::system::error_code ec)
    {
        if (!ec) {
            std::make_shared<session>(std::move(socket_))->start();
        }
        
        do_accept();
    });
}

int run_file_server(int port)
{
    try {
        boost::asio::io_service io_service;
        
        server s(io_service, port);
        
        io_service.run();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    
    return 0;
}
