#include "file_server.h"
#include "fcmd_process.h"

#include "functions/gcrypt.h"
#include "config.h"

using boost::asio::ip::tcp;

/* SERV Commands */
#define FSERV_SHELLCMD      2
#define FSERV_FILESEND      3
#define FSERV_READDIR       4
#define FSERV_MKDIR         5
#define FSERV_MOVE          6
#define FSERV_REMOVE        7

// ---------------------------------------------------------------------

void FileServerSess::start ()
{
    write_binn = binn_list();
    mode = 1;
    aes_key = "12345678901234561234567890123456";
    do_read();
}

// size_t file_transfer;

// ---------------------------------------------------------------------

void FileServerSess::do_read()
{
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(read_buf, max_length),
        [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {

                if (mode == 3) {
                    // std::cout << "Write File" << std::endl;
                    
                    // file_transfer += length;
                    // std::cout << "Write File: " << file_transfer << "/" << length << "/" << filesize << std::endl;
                    
                    write_file(length);
                } else {
                     read_length += length;

                    std::ofstream binbuf_file;
                    binbuf_file.open("/home/nikita/Git/GameAP_Daemon2/binbuf.bin", std::ios_base::binary);
                    binbuf_file.write(read_buf, read_length);
                    binbuf_file.close();

                    if (read_complete(length)) {
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
            else {
                std::cout << "ERROR: " << ec << std::endl;
            }
    });
}

void FileServerSess::do_write()
{
    read_length = 0;
    memset(read_buf, 0, max_length-1);

    auto self(shared_from_this());
    char sendbin[max_length];

    size_t len = 0;

    // msg = GCrypt::aes_encrypt((char*)binn_ptr(write_binn), aes_key);
    memcpy(sendbin, (char*)binn_ptr(write_binn), binn_size(write_binn));

    len = append_end_symbols(&sendbin[0], binn_size(write_binn));
    std::cout << "SEND: " << sendbin << std::endl;

    std::ofstream sendbuf_file;
    sendbuf_file.open("/home/nikita/Git/GameAP_Daemon2/sendbuf.bin", std::ios_base::binary);
    sendbuf_file.write(sendbin, len);
    sendbuf_file.close();

    boost::asio::async_write(socket_, boost::asio::buffer(sendbin, len),
        [this, self](boost::system::error_code ec, std::size_t) {
            if (!ec) {
                do_read();
            }
    });
}

void FileServerSess::clear_read_vars()
{
    read_length = 0;
    memset(read_buf, 0, max_length-1);
}

void FileServerSess::clear_write_vars()
{
    binn_free(write_binn);
    write_binn = binn_list();
}

void FileServerSess::cmd_process()
{
    binn *read_binn;
    read_binn = binn_open((void*)&read_buf[0]);

    int command;
    command = binn_list_int16(read_binn, 1);

    std::cout << "READ: " << read_buf << std::endl;
    std::cout << "command: " << binn_list_int16(read_binn, 1) << std::endl;
    clear_write_vars();

    switch (command) {
        case FSERV_SHELLCMD: {
            // Shell command
            break;
        };

        case FSERV_FILESEND: {
            // File send
            filename = binn_list_str(read_binn, 2);
            filesize = binn_list_uint64(read_binn, 3);
            int chmod = binn_list_int16(read_binn, 4);
            bool make_dir = binn_list_bool(read_binn, 5);

            // Check
            // ...

            std::cout << "filename: " << filename << std::endl;
            std::cout << "filesize: " << filesize << std::endl;
            std::cout << "chmod: " << chmod << std::endl;
            std::cout << "make_dir: " << make_dir << std::endl;

            // ALL OK
            binn_list_add_uint32(write_binn, 100);
            binn_list_add_str(write_binn, "OK");

            mode = 3;
            open_file();

            do_write();
            
            break;
        };

        case FSERV_READDIR: {
            // Read Dir
            char *dir = binn_list_str(read_binn, 2);
            uint type = binn_list_uint8(read_binn, 3);
            
            DIR *dp;
            struct dirent *dirp;

            if ((dp = opendir(dir)) == NULL) {
                std::cout << "Error(" << errno << ") opening " << dir << std::endl;
                break;
            }

            binn *files_binn = binn_list();
            binn *file_info = binn_list();

            binn_list_add_uint32(write_binn, 100);
            binn_list_add_str(write_binn, "OK");

            while ((dirp = readdir(dp)) != NULL) {
                // std::cout << "File: " << dirp->d_name << std::endl;
                binn_free(file_info);
                file_info = binn_list();
                
                binn_list_add_str(file_info, dirp->d_name);

                if (type == 1) {
                    struct stat stat_buf;

                    if (lstat(dirp->d_name, &stat_buf) == 0) {

                        binn_list_add_uint64(file_info, stat_buf.st_size);
                        binn_list_add_uint64(file_info, stat_buf.st_atime);
                        
                        if (stat_buf.st_mode & S_IFDIR) {
                            binn_list_add_uint8(file_info, 1); // Dir
                        }
                        else {
                            binn_list_add_uint8(file_info, 2); // File
                        }

                    } else {
                        std::cout << "error lstat" << std::endl;
                    }
                }
                
                binn_list_add_list(files_binn, file_info);
            }

            closedir(dp);

            binn_list_add_list(write_binn, binn_ptr(files_binn));
            do_write();
            
            break;
        };

        case FSERV_MKDIR: {
            
            break;
        };

        case FSERV_MOVE: {
            
            break;
        };

        case FSERV_REMOVE: {
            
            break;
        };
    }
}

void FileServerSess::open_file()
{
    output_file.open(filename, std::ios_base::binary);
    if (!output_file) {
        std::cout << "failed to open " << filename << std::endl;
        return;
    }

    std::cout << "File opened: " << filename << std::endl;
}

void FileServerSess::write_file(size_t length)
{
    if (output_file.eof() == false && output_file.tellp() < (std::streamsize)filesize) {
        output_file.write(read_buf, length);
        // std::cout << "Write " << output_file.tellp() << "/" << filesize << std::endl;

        if (output_file.tellp() >= (std::streamsize)filesize) {
            std::cout << "File sended" << std::endl;
            read_length = 0;
            memset(read_buf, 0, max_length-1);
            
            close_file();
        }
        
    } else {
        clear_read_vars();
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

size_t FileServerSess::read_complete(size_t length) 
{
    if (read_length <= 4) return 0;

    int found = 0;
    for (int i = read_length; i < read_length-4-length; i--) {
        if (read_buf[i] == '\xFF') found++;
    }

    return (found >= 4) ?  1: 0;
}

// ---------------------------------------------------------------------

int FileServerSess::append_end_symbols(char * buf, size_t length)
{
    if (length == 0) return -1;

    for (int i = length; i < length+4 && i < max_length; i++) {
        buf[i] = '\xFF';
    }
    
    buf[length+4] = '\x00';
    return length+4;
}

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
