#include "consts.h"
#include "config.h"

#include "file_server.h"
#include "daemon_server.h"

// ---------------------------------------------------------------------

void DaemonServerSess::start ()
{
    write_binn = binn_list();
    mode = DAEMON_SERVER_MODE_NOAUTH;

    read_length = 0;
    do_read();
}

// ---------------------------------------------------------------------

void DaemonServerSess::do_read()
{
    switch (mode) {
        case DAEMON_SERVER_MODE_NOAUTH: {

            auto self(shared_from_this());
            socket_.async_read_some(boost::asio::buffer(read_buf, max_length),
                [this, self](boost::system::error_code ec, std::size_t length) {
                    if (!ec) {
                        read_length += length;

                        if (read_complete(length)) {
                            Config& config = Config::getInstance();

                            // Check auth
                            binn *read_binn;
                            read_binn = binn_open((void*)&read_buf[0]);

                            if (config.daemon_login == binn_list_str(read_binn, 2)
                                && config.daemon_password == binn_list_str(read_binn, 3)
                            ) {
                                binn_list_add_uint32(write_binn, 100);
                                binn_list_add_str(write_binn, (char *)"Auth success");

                                mode = binn_list_uint16(read_binn, 4);

                                std::cout << "Auth success" << std::endl;
                            }
                            else {
                                binn_list_add_uint32(write_binn, 2);
                                binn_list_add_str(write_binn, (char *)"Auth failed");
                            }

                            do_write();
                        }
                    }
                    else {
                        std::cout << "ERROR: " << ec << std::endl;
                    }
            });

            std::cout << "NOAUTH" << std::endl;
            break;
        }

        case DAEMON_SERVER_MODE_AUTH:
            std::cout << "AUTH" << std::endl;
            break;

        case DAEMON_SERVER_MODE_CMD:
            std::cout << "CMD" << std::endl;
            break;

        case DAEMON_SERVER_MODE_FILES:
            std::cout << "DAEMON_SERVER_MODE_FILES" << std::endl;
            std::make_shared<FileServerSess>(std::move(socket_))->start();
            break;

        // case DAEMON_SERVER_MODE_SHELL:
            // break;
    }

    // do_read();
}

// ---------------------------------------------------------------------

size_t DaemonServerSess::read_complete(size_t length)
{
    if (read_length <= 4) return 0;

    int found = 0;
    for (int i = read_length; i > read_length-length && found < 4; i--) {
        if (read_buf[i-1] == '\xFF') found++;
    }

    return (found >= 4) ?  1: 0;
}

// ---------------------------------------------------------------------

int DaemonServerSess::append_end_symbols(char * buf, size_t length)
{
    if (length == 0) return -1;

    for (int i = length; i < length+4; i++) {
        buf[i] = '\xFF';
    }

    buf[length+4] = '\x00';
    return length+4;
}

// ---------------------------------------------------------------------

void DaemonServerSess::do_write()
{
    read_length = 0;
    memset(read_buf, 0, max_length-1);

    auto self(shared_from_this());
    char sendbin[binn_size(write_binn)+5];
	// char sendbin[10240];

    // msg = GCrypt::aes_encrypt((char*)binn_ptr(write_binn), aes_key);
    memcpy(sendbin, (char*)binn_ptr(write_binn), binn_size(write_binn));

    size_t len = 0;
    len = append_end_symbols(&sendbin[0], binn_size(write_binn));

    boost::asio::async_write(socket_, boost::asio::buffer(sendbin, len),
        [this, self](boost::system::error_code ec, std::size_t) {
            if (!ec) {
                do_read();
            }
    });
}

// ---------------------------------------------------------------------

void DaemonServer::do_accept()
{
    acceptor_.async_accept(socket_, [this](boost::system::error_code ec)
    {
        if (!ec) {
            std::make_shared<DaemonServerSess>(std::move(socket_))->start();
        }

        do_accept();
    });
}

// ---------------------------------------------------------------------

int run_server(int port)
{
    try {
        boost::asio::io_service io_service;

        DaemonServer s(io_service, port);

        io_service.run();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
