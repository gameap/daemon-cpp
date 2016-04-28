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
            std::cerr << "Usage: client_readdir <host> <port>\n";
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

        binn_list_add_int16(write_binn, 4);                                 // Read dir
        binn_list_add_str(write_binn, "/home/nikita/Git/GameAP_Daemon2/");   // Dir path
        binn_list_add_uint8(write_binn, 1);                                 // Mode

        // SH
        char sendbin[max_length];
        memcpy(sendbin, (char*)binn_ptr(write_binn), binn_size(write_binn));

        append_end_symbols(sendbin, binn_size(write_binn));

        boost::asio::write(s, boost::asio::buffer(sendbin, binn_size(write_binn)+4));
        int length = read(s, boost::asio::buffer(buf), boost::bind(read_not_complete, buf, _1,_2));

        std::ofstream readbuf_file;
        readbuf_file.open("/home/nikita/Git/GameAP_Daemon2/readbuf.bin", std::ios_base::binary);
        readbuf_file.write(buf, length);
        readbuf_file.close();

        // msg = std::string(buf);
        msg = msg.substr(0, length-4);
        std::cout << "READ: " << msg << std::endl;

        read_binn = binn_open((void*)&buf[0]);
        // \SH

        if (binn_list_uint32(read_binn, 1) == 100) {
            std::cout << "ALL OK " << std::endl;
            binn *files_list;
            binn *file_info;
            files_list = binn_open(binn_list_list(read_binn, 3));

            int count = binn_count(files_list);
            for (int i = 1; i <= count; i++) {
                file_info = binn_open(binn_list_list(files_list, i));

                std::cout << binn_list_str(file_info, 1)
                << " " << binn_list_uint64(file_info, 2)
                << " " << binn_list_uint64(file_info, 3)
                << " " << (uint)binn_list_uint8(file_info, 4)
                << std::endl;
            }
        }

        // int result_code = binn_list_int16(read_binn, 1);
        
        pause();
        
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
