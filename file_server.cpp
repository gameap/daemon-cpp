#include "file_server.h"

#include "functions/gcrypt.h"
#include "config.h"

using boost::asio::ip::tcp;

// ---------------------------------------------------------------------

void FileServerSess::start ()
{
    write_binn = binn_list();
    mode = 1;
    aes_key = "12345678901234561234567890123456";
    do_read();
}

// ---------------------------------------------------------------------

void FileServerSess::do_read()
{
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(read_buf, max_length),
        [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {

                read_length += length;
                // std::cout << "MODE: " << mode << std::endl;

                if (mode == 3) {
                    // std::cout << "Write File" << std::endl;
                    write_file(length);
                } else {

                    if (strstr(read_buf, "\xFF\xFF\xFF\xFF") != nullptr) {
                        // std::string msg(read_buf, read_length-4);
                        // binn_list_add_str(write_binn, &msg[0]);
                        switch (mode) {
                            case 0: {
                                break;
                            };
                            case 1: {
                                cmd_process();
                                break;
                            };

                            case 2: {
                                break;
                            };
                        }

                    }
                }

                // read_length += length;

                // if (strstr(read_buf, "\xFF\xFF\xFF\xFF") != nullptr) {
                    // binn_free(write_binn);
                    // write_binn = binn_list();

                    // std::string msg(read_buf, read_length-4);
                    // binn_list_add_str(write_binn, &msg[0]);

                    // read_length = 0;
                    // memset(read_buf, 0, max_length-1);

                    // do_write();
                // }
            }
    });
}

void FileServerSess::do_write()
{
    read_length = 0;
    memset(read_buf, 0, max_length-1);

    auto self(shared_from_this());
    char send[max_length];

    size_t len = 0;

    // msg = GCrypt::aes_encrypt((char*)binn_ptr(write_binn), aes_key);
    // send = (char*)binn_ptr(write_binn);

    strcpy(send, (char*)binn_ptr(write_binn));
    strcat(send, "\xFF\xFF\xFF\xFF");
    std::cout << "SEND: " << send << std::endl;

    boost::asio::async_write(socket_, boost::asio::buffer(send, strlen(send)),
        [this, self](boost::system::error_code ec, std::size_t) {
            if (!ec) {
                do_read();
            }
    });
}

void FileServerSess::cmd_process()
{
    binn *read_binn;
    std::string msg(read_buf, read_length-4);
    read_binn = binn_open((void*)msg.c_str());

    int command;
    command = binn_list_int16(read_binn, 1);

    switch (command) {
        case 2: {
            // Shell command
        };

        case 3: {
            // File send
            filename = binn_list_str(read_binn, 2);
            size_t filesize = binn_list_int16(read_binn, 3);
            int chmod = binn_list_int16(read_binn, 4);
            bool make_dir = binn_list_bool(read_binn, 5);

            // Check
            // ...

            std::cout << "filename: " << filename << std::endl;
            std::cout << "filesize: " << filesize << std::endl;
            std::cout << "chmod: " << chmod << std::endl;
            std::cout << "make_dir: " << make_dir << std::endl;

            binn_release(write_binn);
            write_binn = binn_list();

            // ALL OK
            binn_list_add_int16(write_binn, 100);
            binn_list_add_str(write_binn, "OK");

            mode = 3;
            open_file();

            do_write();
        };
    }
}

void FileServerSess::open_file()
{
    output_file.open(filename, std::ios_base::binary | std::ios_base::ate);
    if (!output_file) {
        std::cout << "failed to open " << filename << std::endl;
        return;
    }

    std::cout << "File opened: " << filename << std::endl;
}

void FileServerSess::write_file(size_t length)
{
    if (output_file.eof() == false) {
        // std::cout << "writed bytes: " << length << std::endl;
        output_file.write(read_buf, length);
    } else {
        std::cout << "CLOSE FILE" << std::endl;
        close_file();
    }

    do_read();
}

void FileServerSess::close_file()
{
    output_file.close();
    mode = 1;
}

// ---------------------------------------------------------------------

// ---------------------------------------------------------------------

void FileServer::do_accept()
{
    acceptor_.async_accept(socket_, [this](boost::system::error_code ec)
    {
        if (!ec) {
            std::make_shared<FileServerSess>(std::move(socket_))->start();
        }

        do_accept();
    });
}

// ---------------------------------------------------------------------

int run_file_server(int port)
{
    try {
        boost::asio::io_service io_service;

        FileServer s(io_service, port);

        io_service.run();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
