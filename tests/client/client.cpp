//
// blocking_tcp_echo_client.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "functions/gcrypt.h"

#include <binn.h>
#include <fstream>

using boost::asio::ip::tcp;

enum { max_length = 1024 };

// ---------------------------------------------------------------------

size_t read_not_complete(char * buf, boost::system::error_code err, size_t length) 
{
	if ( err) return 0;
    std::ofstream binbuf_file;

    if (length <= 4) return 1;

    int found = 0;
    for (int i = length; i > 0; i--) {
        if (buf[i] == '\xFF') found++;
    }

    return (found >= 4) ?  0: 1;
}

// ---------------------------------------------------------------------

int append_end_symbols(char * buf, size_t length)
{
    if (length == 0) return -1;

    for (int i = length; i < length+4 && i < max_length; i++) {
        buf[i] = '\xFF';
    }
    
    buf[length+4] = '\x00';
    return length+4;
}

// ---------------------------------------------------------------------

int main(int argc, char* argv[])
{
    char *aes_key;
    aes_key = "12345678901234561234567890123456";
    std::string msg;

    try
    {
        if (argc != 3) {
            std::cerr << "Usage: blocking_tcp_echo_client <host> <port>\n";
            return 1;
        }

        boost::asio::io_service io_service;

        tcp::socket s(io_service);
        tcp::resolver resolver(io_service);
        boost::asio::connect(s, resolver.resolve({argv[1], argv[2]}));

        char buf[max_length];

        char write_buf[max_length];

        // crypto.setAESKey((unsigned char*)&aes_key[0], 16);

        // File send
        binn *write_binn;
        binn *read_binn;
        write_binn = binn_list();

        // std::string path = "/home/nikita/Git/GameAP_Daemon2/tests/ubuntu-16.04-desktop-amd64.iso";
        std::string path = "/home/nikita/Git/GameAP_Daemon2/tests/client/Makefile";
        std::ifstream source_file;
        source_file.open(path.c_str(), std::ios_base::binary | std::ios_base::ate);

        if (!source_file) {
            std::cout << "failed to open " << path << std::endl;
            return 1;
        }

        binn_list_add_int16(write_binn, 3);                             // File send
        binn_list_add_str(write_binn, "/home/nikita/Downloads/test_daemon/Makefile");   // File name
        binn_list_add_uint32(write_binn, source_file.tellg());          // File size
        binn_list_add_int16(write_binn, 777);                           // Chmod
        binn_list_add_bool(write_binn, false);                              // No mkdir
        
        // char send[max_length];
        // strncpy(send, (char*)binn_ptr(write_binn), binn_size(write_binn));

        char sendbin[max_length];
        // strncpy(sendbin, (char*)binn_ptr(write_binn), binn_size(write_binn));
        memcpy(sendbin, (char*)binn_ptr(write_binn), binn_size(write_binn));
        
        // send
        read_binn = binn_open((void*)sendbin);

        int senbinsize;
        if (senbinsize = append_end_symbols(&sendbin[0], binn_size(write_binn)) == -1) {
            std::cout << "Error append" << std::endl;
            return 1;
        }
        
        boost::asio::write(s, boost::asio::buffer(sendbin, binn_size(write_binn)+4));

        int length = read(s, boost::asio::buffer(buf), boost::bind(read_not_complete, buf, _1,_2));

        msg = std::string(buf);
        msg = msg.substr(0, length-4);
        std::cout << "READ: " << msg << std::endl;

        read_binn = binn_open((void*)&msg[0]);

        int result_code = binn_list_int16(read_binn, 1);

        std::cout << "Result Code: " << binn_list_int16(read_binn, 1) << std::endl;
        std::cout << "Result Text: " << binn_list_str(read_binn, 2) << std::endl;

        // Clear buf
        memset(write_buf, 0, max_length-1);

        if (result_code == 100) {
            // Send File
            source_file.seekg(0);
            while (!source_file.eof()) {
                source_file.read(write_buf, (std::streamsize)768);
                boost::asio::write(s, boost::asio::buffer(write_buf, source_file.gcount()));
                // std::cout << "Send bytes: " << source_file.gcount() << std::endl;
            }

            std::cout << "SENDED!" << std::endl;
        }

        source_file.close();

        binn_free(write_binn);
        write_binn = binn_list();

        /* SECOND FILE */

        path = "/home/nikita/Git/GameAP_Daemon2/tests/client/wallpaper.jpg";
        source_file.open(path.c_str(), std::ios_base::binary | std::ios_base::ate);

        if (!source_file) {
            std::cout << "failed to open " << path << std::endl;
            return 1;
        }

        binn_list_add_int16(write_binn, 3);                             // File send
        binn_list_add_str(write_binn, "/home/nikita/Downloads/test_daemon/wallpaper.jpg");   // File name
        binn_list_add_uint32(write_binn, source_file.tellg());          // File size
        binn_list_add_int16(write_binn, 777);                           // Chmod
        binn_list_add_bool(write_binn, false);                              // No mkdir
        
        // char send[max_length];
        // strncpy(send, (char*)binn_ptr(write_binn), binn_size(write_binn));

        // char sendbin[max_length];
        // strncpy(sendbin, (char*)binn_ptr(write_binn), binn_size(write_binn));
        memcpy(sendbin, (char*)binn_ptr(write_binn), binn_size(write_binn));
        
        // send
        read_binn = binn_open((void*)sendbin);

        // int senbinsize;
        if (senbinsize = append_end_symbols(&sendbin[0], binn_size(write_binn)) == -1) {
            std::cout << "Error append" << std::endl;
            return 1;
        }
        
        boost::asio::write(s, boost::asio::buffer(sendbin, binn_size(write_binn)+4));

        length = read(s, boost::asio::buffer(buf), boost::bind(read_not_complete, buf, _1,_2));

        msg = std::string(buf);
        msg = msg.substr(0, length-4);
        std::cout << "READ: " << msg << std::endl;

        read_binn = binn_open((void*)&msg[0]);

        result_code = binn_list_int16(read_binn, 1);

        std::cout << "Result Code: " << binn_list_int16(read_binn, 1) << std::endl;
        std::cout << "Result Text: " << binn_list_str(read_binn, 2) << std::endl;

        // Clear buf
        memset(write_buf, 0, max_length-1);

        if (result_code == 100) {

            int fsize = source_file.tellg();
            
            // Send File
            source_file.seekg(0);

            size_t sended = 0;

            while (!source_file.eof()) {
                source_file.read(write_buf, (std::streamsize)512);
                
                if (sended = boost::asio::write(s, boost::asio::buffer(write_buf, source_file.gcount()))) {
                    // std::cout << "Send " << source_file.tellg() << "/" << sended << "/" << fsize << std::endl;
                } else {
                    // std::cout << "Send Error!" << std::endl;
                }
            }

            std::cout << "SENDED!" << std::endl;
        }

        pause();
        

        /*
        while (true) {
            std::cout << "Enter message: ";
            
            char request[max_length];

            std::cin.getline(request, max_length);
            strcat(request, "\xFF\xFF\xFF\xFF");
            size_t request_length = std::strlen(request);

            boost::asio::write(s, boost::asio::buffer(request, request_length));

            std::string reply;

            boost::asio::streambuf response;
            
            memset(buf, 0, sizeof(buf));
            int bytes = read(s, boost::asio::buffer(buf), boost::bind(read_complete, buf,_1,_2));

            reply = std::string(buf, bytes);
            reply = reply.substr(0, reply.size()-4);
            char *repl;

            repl = &reply[0];
            // repl = GCrypt::aes_decrypt(&reply[0], aes_key);
            
            binn *read_binn;
            read_binn = binn_open((void*)repl);

            char *message;
            message = binn_list_str(read_binn, 1);

            std::cout << "MSG: " << message << std::endl;
            
            // std::cout << "Reply is: ";
            // std::cout.write(repl, strlen(repl));
            // std::cout << "\n";
        }
        */
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
