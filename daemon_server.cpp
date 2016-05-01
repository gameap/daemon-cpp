#include "file_server.h"
#include "daemon_server.h"

// ---------------------------------------------------------------------

void DaemonServerSess::start ()
{
    do_read();
}

// ---------------------------------------------------------------------

void DaemonServerSess::do_read()
{
    std::make_shared<FileServerSess>(std::move(socket_))->start();
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
