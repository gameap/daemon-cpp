#include "file_server.h"
#include "fcmd_process.h"

#include "functions/gcrypt.h"
#include "config.h"

using boost::asio::ip::tcp;

/* SERV Commands */
#define FSERV_AUTH          1

#define FSERV_SHELLCMD      2
#define FSERV_FILESEND      3
#define FSERV_READDIR       4
#define FSERV_MKDIR         5
#define FSERV_MOVE          6
#define FSERV_REMOVE        7

#define FSERV_FILE_DOWNLOAD 1
#define FSERV_FILE_UPLOAD   2

// ---------------------------------------------------------------------

void FileServerSess::start ()
{
    write_binn = binn_list();

    read_length = 0;
    mode = FSERV_AUTH;
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
            // char temp_buf[max_length];
            // memset(temp_buf, 0, max_length-1);
            // memcpy(temp_buf, read_buf, length);
            // std::cout << "|" << temp_buf << std::endl;

            // std::cout << "length: " << length << std::endl;

            if (!ec) {

                if (mode == FSERV_FILESEND) {

                    if (sendfile_mode == FSERV_FILE_DOWNLOAD) {
                        write_file(length);
                    }
                    else if (sendfile_mode == FSERV_FILE_UPLOAD) {
                        send_file();
                    } else {
                        // Unknown mode
                        mode = FSERV_AUTH;
                    }


                } else {
                    read_length += length;

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
            }
            else {
                // std::cout << "ERROR: " << ec << std::endl;
                std::cout << "ERROR! " << ec.category().name() << ": " << ec.message() << std::endl;
            }
    });
}

void FileServerSess::write_ok()
{
    response_msg(100, "OK", true);
}

void FileServerSess::response_msg(int snum, const char * sdesc, bool write)
{
    binn_list_add_uint32(write_binn, snum);
    binn_list_add_str(write_binn, (char *)sdesc);

    if (write) {
        do_write();
    }
}

void FileServerSess::do_write()
{
    auto self(shared_from_this());
    char sendbin[binn_size(write_binn)+5];
	// char sendbin[10240];

    size_t len = 0;

    // msg = GCrypt::aes_encrypt((char*)binn_ptr(write_binn), aes_key);
    memcpy(sendbin, (char*)binn_ptr(write_binn), binn_size(write_binn));

    len = append_end_symbols(&sendbin[0], binn_size(write_binn));

    // LOG SENDBIN
    // sleep(1);
    // time_t curtime = time(0);
    // std::string binlog = std::string("sendbinn_") + std::to_string(curtime) + std::string(".bin");
    // std::ofstream binlog_file;
    // binlog_file.open(binlog, std::ios_base::binary | std::ios_base::trunc);
    // binlog_file.write(sendbin, len);
    // \LOG SENDBIN

    clear_write_vars();
    // std::cout << "length:" << length << std::endl;

    boost::asio::async_write(socket_, boost::asio::buffer(sendbin, len),
        [this, self](boost::system::error_code ec, std::size_t) {
            if (!ec) {
                do_read();
            }
            else {
                std::cout << "SRAN ERROR!" << std::endl;
            }
    });
}

void FileServerSess::clear_read_vars()
{
    read_length = 0;
    // memset(read_buf, 0, max_length-1);
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

    std::cout << "command: " << binn_list_int16(read_binn, 1) << std::endl;
    clear_write_vars();

    switch (command) {
        case FSERV_SHELLCMD: {
            // Shell command
            break;
        };

        case FSERV_FILESEND: {
            // File send
            sendfile_mode = binn_list_uint8(read_binn, 2);
            filename = binn_list_str(read_binn, 3);
            filesize = binn_list_uint64(read_binn, 4);
            bool make_dir = binn_list_bool(read_binn, 5);
            int chmod = binn_list_int16(read_binn, 6);

            // Check
            // ...

            // ALL OK
            // binn_list_add_uint32(write_binn, 100);
            // binn_list_add_str(write_binn, "OK");

            std::cout << "sendfile_mode: " << sendfile_mode << std::endl;
            std::cout << "filename: " << filename << std::endl;
            std::cout << "filesize: " << filesize << std::endl;
            std::cout << "make_dir: " << make_dir << std::endl;
            std::cout << "chmod: " << chmod << std::endl;

            mode = FSERV_FILESEND;

            if (sendfile_mode == FSERV_FILE_DOWNLOAD) {
                open_output_file();
                write_ok();
            } else if (sendfile_mode == FSERV_FILE_UPLOAD) {
                open_input_file();
                write_ok();
            } else {
                response_msg(1, "Unknown sendfile mode", true);
            }

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

			#ifdef __GNUC__
				chdir(dir);
			#elif _WIN32
				_chdir(dir);
			#endif

            binn *files_binn = binn_list();
            binn *file_info = binn_list();

            response_msg(100, "OK", false);

            while ((dirp = readdir(dp)) != NULL) {
                // std::cout << "File: " << dirp->d_name << std::endl;
                binn_free(file_info);
                file_info = binn_list();

                binn_list_add_str(file_info, dirp->d_name);

                if (type == 1) {
                    struct stat stat_buf;

					#ifdef _WIN32
                        if (stat(dirp->d_name, &stat_buf) == 0) {
                            binn_list_add_uint64(file_info, stat_buf.st_size);
							binn_list_add_uint64(file_info, stat_buf.st_atime);

							if (stat_buf.st_mode & S_IFDIR) {
								binn_list_add_uint8(file_info, 1); // Dir
							}
							else {
								binn_list_add_uint8(file_info, 2); // File
							}
                        } else {
							std::cout << "error stat (" << errno << "): " << strerror(errno) << std::endl;
						}
					#else
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
							std::cout << "error lstat (" << errno << "): " << strerror(errno) << std::endl;
						}
                    #endif
                }

                binn_list_add_list(files_binn, file_info);
            }

            closedir(dp);

            binn_list_add_list(write_binn, files_binn);

            do_write();

            break;
        };

        case FSERV_MKDIR: {

            try {
                boost::filesystem::path p = binn_list_str(read_binn, 2);
                boost::filesystem::create_directories(p);
            }
            catch (boost::filesystem::filesystem_error &e) {
                std::cout << "Error mkdir: " << e.what() << std::endl;
                response_msg(1, e.what(), true);
                return;
            }

            write_ok();

            break;
        };

        case FSERV_MOVE: {
            char *oldfile = binn_list_str(read_binn, 2);
            char *newfile = binn_list_str(read_binn, 3);
            bool copy = binn_list_bool(read_binn, 4);


            try {
                if (copy) {
                    // Copy
                    boost::filesystem::copy(oldfile, newfile);
                } else {
                    // Move
                    boost::filesystem::rename(oldfile, newfile);
                }
            }
            catch (boost::filesystem::filesystem_error &e) {
                std::cout << "Error move: " << e.what() << std::endl;
                response_msg(1, e.what(), true);
                return;
            }

            write_ok();

            // delete oldfile;
            // delete newfile;

            break;
        };

        case FSERV_REMOVE: {
            char *file      = binn_list_str(read_binn, 2);
            bool recursive  = binn_list_bool(read_binn, 3);

            try {
                if (recursive) {
                    boost::filesystem::remove_all(file);
                }
                else {
                    boost::filesystem::remove(file);
                }
            }
            catch (boost::filesystem::filesystem_error &e) {
                std::cout << "Error remove: " << e.what() << std::endl;
                response_msg(1, e.what(), true);
                return;
            }

            write_ok();

            break;
        };

        default : {
            std::cout << "Unknown Command" << std::endl;
            response_msg(3, "Unknown command", true);
            return;
        }
    }
}

void FileServerSess::open_input_file()
{
    input_file.open(filename, std::ios_base::binary);
    if (!input_file) {
        std::cout << "failed to open " << filename << std::endl;
        return;
    }

    std::cout << "File opened: " << filename << std::endl;
}

void FileServerSess::send_file()
{
    char write_buf[max_length];
    memset(write_buf, 0, max_length-1);

    while (!input_file.eof()) {
        input_file.read(write_buf, (std::streamsize)max_length);

        if (boost::asio::write(socket_, boost::asio::buffer(write_buf, input_file.gcount()))) {

        } else {

        }
    }

    close_input_file();
}

void FileServerSess::close_input_file()
{
    input_file.close();
    mode = FSERV_AUTH;
}

void FileServerSess::open_output_file()
{
    output_file.open(filename, std::ios_base::binary);

    if (!output_file) {
        std::cerr << "failed to open " << filename << std::endl;
        return;
    }

    std::cout << "File opened: " << filename << std::endl;
}

void FileServerSess::write_file(size_t length)
{
    auto self(shared_from_this());

    if (output_file.eof() == false && output_file.tellp() < (std::streamsize)filesize) {
        // std::cout << filename << " writed: " << output_file.tellp() << "/" << length << "/" << filesize << std::endl;
        output_file.write(read_buf, length);
        
        if (output_file.tellp() >= (std::streamsize)filesize) {
            std::cout << "File sended: " << filename << std::endl;

            close_output_file();
            clear_read_vars();

            write_ok(); // Ready
        } else {
            do_read();
        }

    } else {
        close_output_file();
        clear_read_vars();

        write_ok(); // Ready
    }
}

void FileServerSess::close_output_file()
{
    output_file.close();
    mode = FSERV_AUTH;

    filename = "";
    filesize = 0;

    std::cout << "File closed" << std::endl;
}

// ---------------------------------------------------------------------

size_t FileServerSess::read_complete(size_t length)
{
    if (read_length <= 4) return 0;

    int found = 0;
    for (int i = read_length; i > read_length-length && found < 4; i--) {
        if (read_buf[i-1] == '\xFF') found++;
    }

    return (found >= 4) ?  1: 0;
}

// ---------------------------------------------------------------------

int FileServerSess::append_end_symbols(char * buf, size_t length)
{
    if (length == 0) return -1;

    for (int i = length; i < length+4; i++) {
        buf[i] = '\xFF';
    }

    buf[length+4] = '\x00';
    return length+4;
}
