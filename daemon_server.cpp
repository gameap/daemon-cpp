#include "consts.h"
#include "config.h"

#include "file_server.h"
#include "daemon_server.h"
#include "classes/dedicated_server.h"

#include "functions/gcrypt.h"

// ---------------------------------------------------------------------

ssl_socket::lowest_layer_type& DaemonServerSess::socket()
{
    return socket_.lowest_layer();
}

void DaemonServerSess::handle_handshake(const boost::system::error_code& error) {
    if (!error) {
        do_read();
    } else {
        delete this;
    }
}

// ---------------------------------------------------------------------

void DaemonServerSess::start ()
{
    write_binn = binn_list();
    mode = DAEMON_SERVER_MODE_NOAUTH;

    Config& config = Config::getInstance();
    pub_keyfile = config.pub_key_file;

    read_length = 0;

    socket_.async_handshake(boost::asio::ssl::stream_base::server,
                            boost::bind(&DaemonServerSess::handle_handshake, this,
                                        boost::asio::placeholders::error));
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

                            if ((read_length-MSG_END_SYMBOLS_NUM) != 256) {
                                std::cerr << "Incorrect message (RSA Size error)" << std::endl;
                                return;
                            }

                            // Decrypt without end chars
                            char * decstring;
                            GCrypt::rsa_pub_decrypt(&decstring, &read_buf[0], read_length-4, &pub_keyfile[0]);

                            // Check auth
                            binn *read_binn;
                            read_binn = binn_open((void*)decstring);
                            
                            std::cout << "read_length: " << read_length << std::endl;

                            char *login;
                            char *password;

                            if (!binn_list_get_str(read_binn, 2, &login)
                                || !binn_list_get_str(read_binn, 3, &password)
                            ) {
                                std::cerr << "Incorrect binn data" << std::endl;
                                return; 
                            }

                            if (config.daemon_login == login
                                && config.daemon_password == password
                            ) {
                                binn_list_add_uint32(write_binn, 100);
                                binn_list_add_str(write_binn, (char *)"Auth success");

                                if (!binn_list_get_uint16(read_binn, 4, &mode)) {
                                    mode = DAEMON_SERVER_MODE_NOAUTH;
                                    std::cerr << "Incorrect binn data" << std::endl;
                                    return; 
                                }

                                std::cout << "Auth success" << std::endl;
                            }
                            else {
                                binn_list_add_uint32(write_binn, 2);
                                binn_list_add_str(write_binn, (char *)"Auth failed");
                            }

                            do_write(true);
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
            //std::make_shared<FileServerSess>(socket_.get_io_service(), context_)->start();
            break;

        // case DAEMON_SERVER_MODE_SHELL:
            // break;
    }

    delete this;
}

// ---------------------------------------------------------------------

size_t DaemonServerSess::read_complete(size_t length)
{
    if (read_length <= 4) return 0;

    int found = 0;
    for (int i = read_length; i > read_length-length && found < MSG_END_SYMBOLS_NUM; i--) {
        if (read_buf[i-1] == '\xFF') found++;
    }

    return (found >= 4) ?  1: 0;
}

// ---------------------------------------------------------------------

int DaemonServerSess::append_end_symbols(char * buf, size_t length)
{
    if (length == 0) return -1;

    for (int i = length; i < length + MSG_END_SYMBOLS_NUM; i++) {
        buf[i] = '\xFF';
    }

    buf[length + MSG_END_SYMBOLS_NUM] = '\x00';
    return length + MSG_END_SYMBOLS_NUM;
}

// ---------------------------------------------------------------------

void DaemonServerSess::do_write(bool rsa_crypt)
{
    read_length = 0;
    memset(read_buf, 0, max_length-1);

    auto self(shared_from_this());
	char sendbin[10240];

    size_t write_len = 0;

    if (rsa_crypt == true) {
        char buf[binn_size(write_binn)+1];
        char * encstring;
        size_t length = GCrypt::rsa_pub_encrypt(&encstring, (char*)binn_ptr(write_binn), binn_size(write_binn), &pub_keyfile[0]);

        if (length == -1) {
            // Crypt error
            return;
        }
        
        memcpy(sendbin, encstring, length);
        write_len = length;
    }
    else {
        memcpy(sendbin, (char*)binn_ptr(write_binn), binn_size(write_binn));
        write_len = binn_size(write_binn);
    }
    
    size_t len = 0;
    len = append_end_symbols(&sendbin[0], write_len);

    boost::asio::async_write(socket_, boost::asio::buffer(sendbin, len),
        [this, self](boost::system::error_code ec, std::size_t) {
            if (!ec) {
                do_read();
            }
    });
}

// ---------------------------------------------------------------------

void DaemonServer::start_accept()
{
    std::shared_ptr<DaemonServerSess> session = std::make_shared<DaemonServerSess>(io_service_, context_);

    acceptor_.async_accept(session->socket(),
                           boost::bind(&DaemonServer::handle_accept, this, session,
                                       boost::asio::placeholders::error)
    );
}

// ---------------------------------------------------------------------

void DaemonServer::handle_accept(std::shared_ptr<DaemonServerSess> session, const boost::system::error_code& error) {
    if (!error) {
        session->start();
        session = std::make_shared<DaemonServerSess>(io_service_, context_);

        acceptor_.async_accept(session->socket(),
                               boost::bind(&DaemonServer::handle_accept, this, session,
                                           boost::asio::placeholders::error)
        );
    }
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
