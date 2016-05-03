#ifndef FILE_SERVER_H
#define FILE_SERVER_H

#define FILE_SERVER_H

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>

#include <boost/filesystem.hpp>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include <fstream>

#include <binn.h>

// ---------------------------------------------------------------------

class FileServerSess : public std::enable_shared_from_this<FileServerSess> {
public:
    FileServerSess(boost::asio::ip::tcp::socket socket) : socket_(std::move(socket)) {};
    void start();

    uint publ = 334;

private:
    void do_read();
    void do_write();
    
    void write_ok();
    
    void response_msg(int snum, const char * sdesc) {
        response_msg(snum, sdesc);
    }
    
    void response_msg(int snum, const char * sdesc, bool write);

    void clear_read_vars();
    void clear_write_vars();

    void cmd_process();

    void open_file();
    void write_file(size_t length);
    void close_file();

    size_t read_complete(size_t length);
    int append_end_symbols(char * buf, size_t length);

    boost::asio::ip::tcp::socket socket_;
    enum { max_length = 1024 };
    
    size_t read_length;
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
    size_t filesize;
    std::ofstream output_file;
};

#endif