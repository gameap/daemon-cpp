#ifndef FILES_COMPONENT_H
#define FILES_COMPONENT_H

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
#include <boost/asio/ssl.hpp>

#include <boost/filesystem.hpp>
#include <sys/stat.h>
#include <sys/types.h>

#include <dirent.h>

#include <fstream>

#include <binn.h>

#include "typedefs.h"
#include "daemon_server.h"

// ---------------------------------------------------------------------

class FileServerSess : public std::enable_shared_from_this<FileServerSess> {
public:
    FileServerSess(std::shared_ptr<Connection> connection) : connection_(std::move(connection)) {};
    void start();

private:
    void do_read();
    void do_write();

    /**
     *  Response okay message
    */
    void write_ok();
    void write_ok(std::string message);

    /**
    * Response message
    *
    * @param snum message code (1 - error, 2 - critical error, 3 - unknown command, 100 - okay)
    * @param sdesc text message
    * @param write
    */
    void response_msg(unsigned int snum, const char * sdesc, bool write);

    void response_msg(unsigned int snum, const char * sdesc) {
        response_msg(snum, sdesc, false);
    }

    /**
     * Clear read variables
     */
    void clear_read_vars();

    /**
     * Clear write variables
     */
    void clear_write_vars();

    /**
     * Main command operations. Read dir, files, stat
     */
    void cmd_process();

    void open_input_file();
    void send_file();
    void close_input_file();
    
    void open_output_file();
    void write_file(size_t length);
    void close_output_file();

    /**
     * Check if read completed
     *
     * @param length
     * @return
     */
    size_t read_complete(size_t length);

    /**
     * Add end symbols
     *
     * @param buf
     * @param length
     * @return
     */
    int append_end_symbols(char * buf, size_t length);

    std::shared_ptr<Connection> connection_;
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
