#ifndef FILE_SERVER_H
#define FILE_SERVER_H

#define FILE_SERVER_H

#ifdef _WIN32
#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#endif

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

#include "typedefs.h"

// ---------------------------------------------------------------------

class FileServerSess : public std::enable_shared_from_this<FileServerSess> {
public:
    FileServerSess(boost::asio::ip::tcp::socket socket) : socket_(std::move(socket)) {};
    void start();

    uint publ;


private:
    void do_read();
    void do_write();
    
    void write_ok();
    
    void response_msg(int snum, const char * sdesc) {
        response_msg(snum, sdesc, false);
    }
    
    void response_msg(int snum, const char * sdesc, bool write);

    void clear_read_vars();
    void clear_write_vars();

    void cmd_process();

    void open_input_file();
    void send_file();
    void close_input_file();
    
    void open_output_file();
    void write_file(size_t length);
    void close_output_file();

    size_t read_complete(size_t length);
    int append_end_symbols(char * buf, size_t length);

    boost::asio::ip::tcp::socket socket_;
    enum { max_length = 1024 };
    
    size_t read_length;
    char read_buf[max_length];
    char write_buf[max_length];
    
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
    std::string filename;
    size_t filesize;

    unsigned char sendfile_mode;
    
    std::ifstream input_file;
    std::ofstream output_file;
};

#endif
