#include "file_server.h"

#include "functions/gcrypt.h"
#include "config.h"

using boost::asio::ip::tcp;
namespace fs = boost::filesystem;

/* SERV Commands */
#define FSERV_NOAUTH        0
#define FSERV_AUTH          1

#define FSERV_SHELLCMD      2
#define FSERV_FILESEND      3
#define FSERV_READDIR       4
#define FSERV_MKDIR         5
#define FSERV_MOVE          6
#define FSERV_REMOVE        7
#define FSERV_FILEINFO      8

#define FSERV_FILE_DOWNLOAD 1
#define FSERV_FILE_UPLOAD   2

#define FSERV_END_SYMBOL '\xFF'

#define FSERV_TYPE_UNKNOWN          0
#define FSERV_TYPE_DIR              1
#define FSERV_TYPE_FILE             2
#define FSERV_TYPE_CHR_DEVICE       3
#define FSERV_TYPE_BLOCK_DEVICE     4
#define FSERV_TYPE_FIFO             5
#define FSERV_TYPE_SYMLINK          6
#define FSERV_TYPE_SOCKET           7

// ---------------------------------------------------------------------

void FileServerSess::start ()
{
    write_binn = binn_list();

    read_length = 0;
    mode = FSERV_AUTH;

    // setgid(1000);
    // setuid(1000);
    do_read();
}

/**
 * Read
 */
void FileServerSess::do_read()
{
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(read_buf, max_length),
        [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {

                if (mode == FSERV_FILESEND) {

                    if (sendfile_mode == FSERV_FILE_DOWNLOAD) {
                        write_file(length);
                    } else {
                        // Unknown mode
                        mode = FSERV_AUTH;
                    }


                } else {
                    read_length += length;

                    if (read_complete(length)) {
                        switch (mode) {
                            case FSERV_AUTH: {
                                try {
                                    cmd_process();
                                } catch (boost::filesystem::filesystem_error &e) {
                                    response_msg(2, e.what(), true);
                                }
                                break;
                            }

                            case FSERV_NOAUTH:
                            default: {
                                break;
                            }
                        }
                    } else {
                        std::cout << "length: " << length << std::endl;
                        std::cout << "Read incomplete" << std::endl;

                    }
                }
            }
            else {
                std::cerr << "ERROR! " << ec.category().name() << ": " << ec.message() << std::endl;
            }
        });
}

/**
 *  Response okay message
 */
void FileServerSess::write_ok()
{
    response_msg(100, "OK", true);
}

/**
 * Response message
 *
 * @param snum message code (1 - error, 2 - critical error, 3 - unknown command, 100 - okay)
 * @param sdesc text message
 * @param write
 */
void FileServerSess::response_msg(unsigned int snum, const char * sdesc, bool write)
{
    binn_list_add_uint32(write_binn, snum);
    binn_list_add_str(write_binn, (char *)sdesc);

    if (write) {
        do_write();
    }
}

/**
 * Write
 */
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
    clear_read_vars();

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

/**
 * Clear read variables
 */
void FileServerSess::clear_read_vars()
{
    read_length = 0;
    memset(read_buf, 0, max_length-1);
}

/**
 * Clear write variables
 */
void FileServerSess::clear_write_vars()
{
    binn_free(write_binn);
    write_binn = binn_list();
}

/**
 * Main command operations
 */
void FileServerSess::cmd_process()
{
    binn *read_binn;
    read_binn = binn_open((void*)&read_buf[0]);

    unsigned char command;

    if (!binn_list_get_uint8(read_binn, 1, &command)) {
        response_msg(1, "Binn data get error", true);
        return;
    }

    std::cout << "command: " << binn_list_int16(read_binn, 1) << std::endl;
    clear_write_vars();

    switch (command) {
        case FSERV_SHELLCMD: {
            // Shell command
            break;
        };

        case FSERV_FILESEND: {
            // File send
            char * fname;
            if (!binn_list_get_uint8(read_binn, 2, &sendfile_mode)
                || !binn_list_get_str(read_binn, 3, &fname)
                    ) {
                response_msg(1, "Binn data get error", true);
                break;
            }

            filename = std::string(fname);

            // Check
            // ...

            mode = FSERV_FILESEND;

            if (sendfile_mode == FSERV_FILE_DOWNLOAD) {
                BOOL make_dir;
                uint chmod;

                uint64 fsize;
                if (!binn_list_get_uint64(read_binn, 4, &fsize)
                    || !binn_list_get_bool(read_binn, 5, &make_dir)
                    || !binn_list_get_uint8(read_binn, 6, (unsigned char *)&chmod)
                 ) {
                    response_msg(1, "Binn data get error", true);
                    break;
                }
                filesize = fsize;

                open_output_file();
                write_ok();
            } else if (sendfile_mode == FSERV_FILE_UPLOAD) {

                fs::path p = filename;

                if (!fs::exists(filename)) {
                    response_msg(1, "File not found", true);
                    return;
                }

                std::cout << "Filename: " << p << std::endl;
                std::cout << "Filesize: " << fs::file_size(p) << std::endl;

                binn_list_add_uint32(write_binn, 100);
                binn_list_add_str(write_binn, (char *)"Send start");
                binn_list_add_uint64(write_binn, fs::file_size(p));
                do_write();

                open_input_file();
                send_file();
            } else {
                response_msg(1, "Unknown sendfile mode", true);
            }

            break;
        };

        case FSERV_READDIR: {
            // Read Dir
            char *dir;
            unsigned char type;

            if (!binn_list_get_str(read_binn, 2, &dir)
                || !binn_list_get_uint8(read_binn, 3, &type)
             ) {
                response_msg(1, "Binn data get error", true);
                break;
            }

            DIR *dp;
            struct dirent *dirp;
            if ((dp = opendir(dir)) == nullptr) {
                response_msg(1, "Directory open error", true);
                std::cerr << "Error(" << errno << ") opening " << dir << std::endl;
                break;
            }

            binn *files_binn = binn_list();
            binn *file_info = binn_list();

            while ((dirp = readdir(dp)) != nullptr) {
                binn_free(file_info);
                file_info = binn_list();

                binn_list_add_str(file_info, dirp->d_name);

                if (type == 1) {
                    fs::path file_path(std::string(std::string(dir) + fs::path::preferred_separator + std::string(dirp->d_name)));
                    fs::file_status file_status = fs::status(file_path);

                    binn_list_add_uint64(file_info, (file_status.type() == fs::file_type::regular_file)
                                                    ? fs::file_size(file_path)
                                                    : 0
                    );

                    binn_list_add_uint64(file_info, (uint64)fs::last_write_time(file_path));

                    if (file_status.type() == fs::file_type::directory_file) {
                        binn_list_add_uint8(file_info, FSERV_TYPE_DIR);
                    } else {
                        binn_list_add_uint8(file_info, FSERV_TYPE_FILE);
                    }

                    std::stringstream file_permission;
                    file_permission << std::oct << file_status.permissions();

                    binn_list_add_uint16(file_info, (unsigned short)std::stoi(file_permission.str()));
                }

                binn_list_add_list(files_binn, file_info);
            }

            closedir(dp);

            response_msg(100, "OK", false);
            binn_list_add_list(write_binn, files_binn);
            do_write();

            break;
        };

        case FSERV_MKDIR: {

            char *path;
            if (!binn_list_get_str(read_binn, 2, &path)) {
                response_msg(1, "Binn data get error", true);
                break;
            }

            try {
                fs::path p = path;
                fs::create_directories(p);
            }
            catch (fs::filesystem_error &e) {
                std::cout << "Error mkdir: " << e.what() << std::endl;
                response_msg(1, e.what(), true);
                return;
            }

            write_ok();

            break;
        };

        case FSERV_MOVE: {
            char *oldfile;
            char *newfile;
            BOOL copy;

            if (!binn_list_get_str(read_binn, 2, &oldfile)
                || !binn_list_get_str(read_binn, 3, &newfile)
                || !binn_list_get_bool(read_binn, 4, &copy)
                    ) {
                response_msg(1, "Binn data get error", true);
                break;
            }


            try {
                if (copy) {
                    // Copy
                    fs::copy(oldfile, newfile);
                } else {
                    // Move
                    fs::rename(oldfile, newfile);
                }
            }
            catch (fs::filesystem_error &e) {
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
            char *file;
            BOOL recursive ;

            if (!binn_list_get_str(read_binn, 2, &file)
                || !binn_list_get_bool(read_binn, 3, &recursive)
                    ) {
                response_msg(1, "Binn data get error", true);
                break;
            }

            try {
                if (recursive) {
                    fs::remove_all(file);
                }
                else {
                    fs::remove(file);
                }
            }
            catch (fs::filesystem_error &e) {
                std::cout << "Error remove: " << e.what() << std::endl;
                response_msg(1, e.what(), true);
                return;
            }

            write_ok();

            break;
        };

        case FSERV_FILEINFO: {
            // Get detail file information:
            // Mime type

            char *file;

            if (!binn_list_get_str(read_binn, 2, &file)) {
                response_msg(1, "Binn data get error", true);
                break;
            }

            if (!fs::exists(file)) {
                response_msg(1, "File not found", true);
                break;
            }

            binn *file_info = binn_list();

            binn_list_add_str(file_info, file);

            struct stat stat_buf;

            if (stat(std::string(file).c_str(), &stat_buf) == 0) {
                binn_list_add_uint64(file_info, (uint64_t)stat_buf.st_size); // File size

                if (stat_buf.st_mode & S_IFDIR) {
                    binn_list_add_uint8(file_info, FSERV_TYPE_DIR); // Dir
                }
                else if (stat_buf.st_mode & S_IFREG) {
                    binn_list_add_uint8(file_info, FSERV_TYPE_FILE); // File
                }
                else if (stat_buf.st_mode & S_IFCHR) {
                    binn_list_add_uint8(file_info, FSERV_TYPE_CHR_DEVICE); // Character device
                }
                else if (stat_buf.st_mode & S_IFBLK) {
                    binn_list_add_uint8(file_info, FSERV_TYPE_BLOCK_DEVICE); // Block device
                }
                else if (stat_buf.st_mode & S_IFIFO) {
                    binn_list_add_uint8(file_info, FSERV_TYPE_FIFO); // Named pipe
                }
                else if (stat_buf.st_mode & S_IFLNK) {
                    binn_list_add_uint8(file_info, FSERV_TYPE_SYMLINK); // Symlink
                }
                else if (stat_buf.st_mode & S_IFSOCK) {
                    binn_list_add_uint8(file_info, FSERV_TYPE_SOCKET); // Socket
                }
                else {
                    binn_list_add_uint8(file_info, FSERV_TYPE_UNKNOWN); // Unknown
                }

                binn_list_add_uint64(file_info, (uint64_t)stat_buf.st_mtime);
                binn_list_add_uint64(file_info, (uint64_t)stat_buf.st_atime);
                binn_list_add_uint64(file_info, (uint64_t)stat_buf.st_ctime);

                std::stringstream file_permission;
                file_permission << std::oct << fs::status(file).permissions();

                binn_list_add_uint16(file_info, (unsigned short)std::stoi(file_permission.str()));
            } else {
                std::cerr << "error stat (" << errno << "): " << strerror(errno) << std::endl;

                response_msg(1, strerror(errno), true);
            }

            // TODO: Implement this
            binn_list_add_str(file_info, nullptr); // Mime. Not implemented

            response_msg(100, "OK", false);
            binn_list_add_list(write_binn, file_info);
            do_write();

            break;
        };

        default : {
            std::cout << "Unknown Command" << std::endl;
            response_msg(3, "Unknown command", true);
            return;
        }
    }
}

/**
 * Open file to upload/sending
 */
void FileServerSess::open_input_file()
{
    input_file.open(filename, std::ios_base::binary);
    if (!input_file) {
        std::cout << "failed to open " << filename << std::endl;
        return;
    }

    std::cout << "File opened: " << filename << std::endl;
}

/**
 * Send file to client
 */
void FileServerSess::send_file()
{
    auto self(shared_from_this());

    std::cout << "Sending file" << std::endl;
    while (!input_file.eof()) {
        input_file.read(write_buf, (std::streamsize)max_length);

        boost::asio::async_write(socket_, boost::asio::buffer(write_buf, input_file.gcount()),
             [this, self](boost::system::error_code ec, std::size_t) {
                 if (ec) {
                     std::cout << "Write socket error: " << ec << std::endl;
                 }
             });
    }

    std::cout << "File send success" << std::endl;
    close_input_file();
    clear_read_vars();
}

/**
 * Close upload file
 */
void FileServerSess::close_input_file()
{
    input_file.close();
    mode = FSERV_AUTH;
}

/**
 * Open file to download
 */
void FileServerSess::open_output_file()
{
    output_file.open(filename, std::ios_base::binary);

    if (!output_file) {
        std::cerr << "failed to open " << filename << std::endl;
        return;
    }

    std::cout << "File opened: " << filename << std::endl;
}

/**
 * Write file
 * @param length
 */
void FileServerSess::write_file(size_t length)
{
    auto self(shared_from_this());

    std::cout << "Filesize: " << (std::streamsize)filesize << std::endl;

    if ( ! output_file.eof() && output_file.tellp() < (std::streamsize)filesize) {
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
        std::cout << "Error" << std::endl;
        close_output_file();
        clear_read_vars();

        write_ok(); // Ready
    }
}

/**
 * Close downloaded file
 */
void FileServerSess::close_output_file()
{
    output_file.close();
    mode = FSERV_AUTH;

    filename = "";
    filesize = 0;

    std::cout << "File closed" << std::endl;
}

/**
 * Check if read completed
 *
 * @param length
 * @return
 */
size_t FileServerSess::read_complete(size_t length)
{
    if (read_length <= 4) return 0;

    int found = 0;
    for (size_t i = read_length; i > read_length-length && found < 4; i--) {
        // std::cout << i << ": " << read_buf[i-1] << std::endl;
        if (read_buf[i-1] == FSERV_END_SYMBOL) found++;
    }

    return (found >= 4) ?  1: 0;
}

/**
 * Add end symbols
 *
 * @param buf
 * @param length
 * @return
 */
int FileServerSess::append_end_symbols(char * buf, size_t length)
{
    if (length == 0) return -1;

    for (size_t i = length; i < length + 4; i++) {
        buf[i] = FSERV_END_SYMBOL;
    }

    buf[length+4] = '\x00';
    return length + 4;
}
