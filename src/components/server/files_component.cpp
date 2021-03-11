#include "files_component.h"

#include "log.h"
#include "config.h"

#include <boost/format.hpp>

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
#define FSERV_CHMOD         9

#define FSERV_FILE_DOWNLOAD 1
#define FSERV_FILE_UPLOAD   2

#define FSERV_END_SYMBOL '\xFF'

#define FSERV_DETAILS               1
#define FSERV_FULL_DETAILS          2

#define FSERV_TYPE_UNKNOWN          0
#define FSERV_TYPE_DIR              1
#define FSERV_TYPE_FILE             2
#define FSERV_TYPE_CHR_DEVICE       3
#define FSERV_TYPE_BLOCK_DEVICE     4
#define FSERV_TYPE_FIFO             5
#define FSERV_TYPE_SYMLINK          6
#define FSERV_TYPE_SOCKET           7

#define FSERV_STATUS_ERROR                  1
#define FSERV_STATUS_CRITICAL_ERROR         2
#define FSERV_STATUS_UNKNOWN_COMMAND        3
#define FSERV_STATUS_OK                     100
#define FSERV_STATUS_FILE_TRANSFER_READY    101

// ---------------------------------------------------------------------

void FileServerSess::start ()
{
    m_write_binn = binn_list();

    m_read_length = 0;
    m_mode = FSERV_AUTH;

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
    (*m_connection->socket).async_read_some(boost::asio::buffer(m_read_buf, max_length),
        [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {

                if (m_mode == FSERV_FILESEND) {

                    if (m_sendfile_mode == FSERV_FILE_DOWNLOAD) {
                        write_file(length);
                    } else if (m_sendfile_mode == FSERV_FILE_UPLOAD) {
                        send_file();
                    } else {
                        // Unknown mode
                        m_mode = FSERV_AUTH;
                    }


                } else {
                    m_read_length += length;

                    if (read_complete(length)) {
                        switch (m_mode) {
                            case FSERV_AUTH: {
                                m_read_binn = binn_open((void*)&m_read_buf[0]);

                                try {
                                    cmd_process();
                                    binn_free(m_read_binn);
                                } catch (boost::filesystem::filesystem_error &e) {
                                    binn_free(m_read_binn);
                                    response_msg(FSERV_STATUS_CRITICAL_ERROR, e.what(), true);
                                }
                                break;
                            }

                            case FSERV_NOAUTH:
                            default: {
                                break;
                            }
                        }
                    } else {
                        GAMEAP_LOG_VERBOSE << "length: " << length;
                        GAMEAP_LOG_VERBOSE << "Read incomplete";

                    }
                }
            } else if (ec != boost::asio::error::operation_aborted) {
                (*m_connection->socket).shutdown();
            }
        });
}

/**
 *  Response okay message
 */
void FileServerSess::write_ok()
{
    write_ok("OK");
}

void FileServerSess::write_ok(std::string message)
{
    response_msg(FSERV_STATUS_OK, message.c_str(), true);
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
    binn_list_add_uint32(m_write_binn, snum);
    binn_list_add_str(m_write_binn, (char *)sdesc);

    if (write) {
        do_write();
    }
}

/**
 * Write
 */
void FileServerSess::do_write()
{
    m_write_msg = std::string(static_cast<char*>(binn_ptr(m_write_binn)), static_cast<uint>(binn_size(m_write_binn)));
    m_write_msg.append(END_SYMBOLS);

    clear_write_vars();
    clear_read_vars();

    auto self(shared_from_this());
    boost::asio::async_write(*m_connection->socket, boost::asio::buffer(m_write_msg.c_str(), m_write_msg.length()),
        [this, self](boost::system::error_code ec, std::size_t length) {
        if (!ec) {
            m_write_msg.erase();
            do_read();
        } else if (ec != boost::asio::error::operation_aborted) {
            (*m_connection->socket).shutdown();
        }
    });
}

/**
 * Clear read variables
 */
void FileServerSess::clear_read_vars()
{
    m_read_length = 0;
    memset(m_read_buf, 0, max_length-1);
}

/**
 * Clear write variables
 */
void FileServerSess::clear_write_vars()
{
    binn_free(m_write_binn);
    m_write_binn = binn_list();
}

/**
 * Main command operations
 */
void FileServerSess::cmd_process()
{
    unsigned char command;

    if (!binn_list_get_uint8(m_read_binn, 1, &command)) {
        binn_free(m_read_binn);
        response_msg(FSERV_STATUS_ERROR, "Binn parse error: reading command failed", true);
        return;
    }

    clear_write_vars();

    switch (command) {
        case FSERV_SHELLCMD: {
            // Shell command
            break;
        };

        case FSERV_FILESEND: {
            // File send
            if (!binn_list_get_uint8(m_read_binn, 2, &m_sendfile_mode)) {
                response_msg(FSERV_STATUS_ERROR, "Binn parse error: invalid sendfile mode", true);
                break;
            }

            char * fname;
            if (!binn_list_get_str(m_read_binn, 3, &fname)) {
                response_msg(FSERV_STATUS_ERROR, "Binn parse error: invalid filename", true);
                break;
            }

            m_filename = std::string(fname);

            // Check
            // ...

            m_mode = FSERV_FILESEND;

            if (m_sendfile_mode == FSERV_FILE_DOWNLOAD) {
                BOOL make_dir;
                unsigned short chmod;

                uint64 fsize;

                if (!binn_list_get_uint64(m_read_binn, 4, &fsize)) {
                    response_msg(FSERV_STATUS_ERROR, "Binn parse error: invalid filesize", true);
                    break;
                }

                if (!binn_list_get_bool(m_read_binn, 5, &make_dir)) {
                    response_msg(FSERV_STATUS_ERROR, "Binn parse error: invalid makedir value", true);
                    break;
                }
                if (!binn_list_get_uint16(m_read_binn, 6, &chmod)) {
                    response_msg(FSERV_STATUS_ERROR, "Binn parse error: invalid chmod", true);
                    break;
                }

                m_filesize = fsize;

                open_output_file();
                response_msg(FSERV_STATUS_FILE_TRANSFER_READY, "File receive started", true);
            } else if (m_sendfile_mode == FSERV_FILE_UPLOAD) {

                fs::path p = m_filename;

                if (!fs::exists(m_filename)) {
                    response_msg(FSERV_STATUS_ERROR, "File not found", true);
                    return;
                }

                GAMEAP_LOG_VERBOSE << "Filename: " << p;
                GAMEAP_LOG_VERBOSE << "Filesize: " << fs::file_size(p);

                binn_list_add_uint32(m_write_binn, FSERV_STATUS_FILE_TRANSFER_READY);
                binn_list_add_str(m_write_binn, (char *)"File sending ready");
                binn_list_add_uint64(m_write_binn, fs::file_size(p));

                open_input_file();
                do_write();
            } else {
                response_msg(FSERV_STATUS_ERROR, "Unknown sendfile mode", true);
            }

            break;
        };

        case FSERV_READDIR: {
            // Read Dir
            char *dir;
            unsigned char type;

            if (!binn_list_get_str(m_read_binn, 2, &dir)) {
                response_msg(FSERV_STATUS_ERROR, "Binn parse error: invalid dir value", true);
                break;
            }

            if (!binn_list_get_uint8(m_read_binn, 3, &type)) {
                response_msg(FSERV_STATUS_ERROR, "Binn parse error: invalid type value", true);
                break;
            }

            binn *files_binn = binn_list();
            binn *file_info = binn_list();

            fs::path dirp(dir);

            if (!fs::exists(dirp)) {
                response_msg(FSERV_STATUS_ERROR, "Directory open error", true);
                GAMEAP_LOG_ERROR << "Directory open error: " << dir;
                break;
            }

            for (auto &file : fs::directory_iterator{ dirp }) {
                try {
                    fs::path file_path = file.path();
                    std::string file_name = file.path().filename().string();

                    binn_free(file_info);
                    file_info = binn_list();

                    binn_list_add_str(file_info, &file_name[0]);

                    if (type == FSERV_DETAILS) {
                        if (fs::is_symlink(file_path)) {
                            file_path = fs::read_symlink(file_path);
                        }

                        if (!fs::exists(file_path)) {
                            continue;
                        }

                        fs::file_status file_status = fs::is_symlink(file_path)
                            ? fs::symlink_status(file_path)
                            : fs::status(file_path);

                        if (file_status.type() == fs::file_type::type_unknown) {
                            continue;
                        }

                        binn_list_add_uint64(file_info, (file_status.type() == fs::file_type::regular_file)
                            ? fs::file_size(file_path)
                            : 0
                        );


                        binn_list_add_uint64(file_info, (uint64)(fs::last_write_time(file_path)));

                        if (file_status.type() == fs::file_type::directory_file) {
                            binn_list_add_uint8(file_info, FSERV_TYPE_DIR);
                        }
                        else {
                            binn_list_add_uint8(file_info, FSERV_TYPE_FILE);
                        }

                        binn_list_add_uint16(file_info, (unsigned short)file_status.permissions());
                    }

                    binn_list_add_list(files_binn, file_info);
                }
                catch (const std::exception & ex) {
                    GAMEAP_LOG_ERROR << file.path().filename() << ": " << ex.what();
                }
            }

            response_msg(FSERV_STATUS_OK, "OK", false);
            binn_list_add_list(m_write_binn, files_binn);

            binn_free(files_binn);
            binn_free(file_info);

            do_write();

            break;
        };

        case FSERV_MKDIR: {

            char *path;
            if (!binn_list_get_str(m_read_binn, 2, &path)) {
                response_msg(FSERV_STATUS_ERROR, "Binn parse error: invalid path", true);
                break;
            }

            fs::path p = path;
            if (fs::exists(p)) {
                std::string message = boost::str(boost::format("File `%s` exist") % path);
                response_msg(FSERV_STATUS_ERROR, message.c_str(), true);
                break;
            }

            try {
                fs::create_directories(p);
            }
            catch (fs::filesystem_error &e) {
                GAMEAP_LOG_ERROR << "Error mkdir: " << e.what();
                response_msg(FSERV_STATUS_ERROR, e.what(), true);
                return;
            }

            write_ok();

            break;
        };

        case FSERV_MOVE: {
            char *oldfile;
            char *newfile;
            BOOL copy;

            if (!binn_list_get_str(m_read_binn, 2, &oldfile)) {
                response_msg(FSERV_STATUS_ERROR, "Binn parse error: invalid oldfile value", true);
                break;
            }

            if (!binn_list_get_str(m_read_binn, 3, &newfile)) {
                response_msg(FSERV_STATUS_ERROR, "Binn parse error: invalid newfile value", true);
                break;
            }

            if (!binn_list_get_bool(m_read_binn, 4, &copy)) {
                response_msg(FSERV_STATUS_ERROR, "Binn parse error: invalid copy value", true);
                break;
            }

            try {
                if (copy) {
                    fs::copy(oldfile, newfile);
                } else {
                    fs::rename(oldfile, newfile);
                }
            }
            catch (fs::filesystem_error &e) {
                GAMEAP_LOG_ERROR << "Error move: " << e.what();
                response_msg(FSERV_STATUS_ERROR, e.what(), true);
                return;
            }

            write_ok();

            break;
        };

        case FSERV_REMOVE: {
            char *file;
            BOOL recursive ;

            if (!binn_list_get_str(m_read_binn, 2, &file)) {
                response_msg(FSERV_STATUS_ERROR, "Binn parse error: invalid file value", true);
                break;
            }

            if (!binn_list_get_bool(m_read_binn, 3, &recursive)) {
                response_msg(FSERV_STATUS_ERROR, "Binn parse error: invalid recursive value", true);
                break;
            }

            fs::path p = file;
            if ( ! fs::exists(p)) {
                response_msg(FSERV_STATUS_ERROR, "File not found", true);
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
                GAMEAP_LOG_ERROR << "Error remove: " << e.what();
                response_msg(FSERV_STATUS_ERROR, e.what(), true);
                return;
            }

            write_ok();

            break;
        };

        case FSERV_FILEINFO: {
            // Get detail file information:
            // Mime type

            char *file;

            if (!binn_list_get_str(m_read_binn, 2, &file)) {
                response_msg(FSERV_STATUS_ERROR, "Binn parse error: invalid file value", true);
                break;
            }

            if (!fs::exists(file)) {
                response_msg(FSERV_STATUS_ERROR, "File not found", true);
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
#ifdef __linux__
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
#endif
                else {
                    binn_list_add_uint8(file_info, FSERV_TYPE_UNKNOWN); // Unknown
                }

                binn_list_add_uint64(file_info, (uint64_t)stat_buf.st_mtime);
                binn_list_add_uint64(file_info, (uint64_t)stat_buf.st_atime);
                binn_list_add_uint64(file_info, (uint64_t)stat_buf.st_ctime);

                binn_list_add_uint16(file_info, (unsigned short)fs::status(file).permissions());
            } else {
                GAMEAP_LOG_ERROR << "error stat (" << errno << "): " << strerror(errno);

                response_msg(FSERV_STATUS_ERROR, strerror(errno), true);
            }

            // TODO: Implement this
            binn_list_add_str(file_info, nullptr); // Mime. Not implemented

            response_msg(FSERV_STATUS_OK, "File info", false);
            binn_list_add_list(m_write_binn, file_info);
            binn_free(file_info);
            do_write();

            break;
        };

        case FSERV_CHMOD: {
            char *file;
            ushort permissions;

            if (!binn_list_get_str(m_read_binn, 2, &file)) {
                response_msg(FSERV_STATUS_ERROR, "Binn parse error: invalid file value", true);
                break;
            }

            if (!binn_list_get_uint16(m_read_binn, 3, &permissions)) {
                response_msg(FSERV_STATUS_ERROR, "Binn parse error: invalid permissions value", true);
                break;
            }

            if (!fs::exists(file)) {
                response_msg(FSERV_STATUS_ERROR, "File not found", true);
                break;
            }

            fs::perms fs_permissions = fs::no_perms;

            if (permissions & fs::owner_read) fs_permissions |= fs::owner_read;
            if (permissions & fs::owner_write) fs_permissions |= fs::owner_write;
            if (permissions & fs::owner_exe) fs_permissions |= fs::owner_exe;

            if (permissions & fs::group_read) fs_permissions |= fs::group_read;
            if (permissions & fs::group_write) fs_permissions |= fs::group_write;
            if (permissions & fs::group_exe) fs_permissions |= fs::group_exe;

            if (permissions & fs::others_read) fs_permissions |= fs::others_read;
            if (permissions & fs::others_write) fs_permissions |= fs::others_write;
            if (permissions & fs::others_exe) fs_permissions |= fs::others_exe;

            if (permissions & fs::set_uid_on_exe) fs_permissions |= fs::set_uid_on_exe;
            if (permissions & fs::set_gid_on_exe) fs_permissions |= fs::set_gid_on_exe;
            if (permissions & fs::sticky_bit) fs_permissions |= fs::sticky_bit;

            fs::permissions(file, fs_permissions);
            write_ok();

            break;
        };

        default : {
            GAMEAP_LOG_WARNING << "Unknown Command";
            response_msg(FSERV_STATUS_UNKNOWN_COMMAND, "Unknown command", true);
            return;
        };
    }
}

/**
 * Open file to upload/sending
 */
void FileServerSess::open_input_file()
{
    m_input_file.open(m_filename, std::ios_base::binary);
    if (!m_input_file) {
        GAMEAP_LOG_ERROR << "failed to open " << m_filename;
        return;
    }

    GAMEAP_LOG_DEBUG << "File opened: " << m_filename;
}

/**
 * Send file to client
 */
void FileServerSess::send_file()
{
    auto self(shared_from_this());

    GAMEAP_LOG_DEBUG << "Sending file";
    while (!m_input_file.eof()) {
        m_input_file.read(m_write_buf, (std::streamsize)max_length);

        boost::system::error_code ec;
        (*m_connection->socket).write_some(boost::asio::buffer(m_write_buf, m_input_file.gcount()), ec);

        if (ec) {
            GAMEAP_LOG_ERROR << "Sending file error (" << ec << "): " << ec.message();
            break;
        }
    }

    GAMEAP_LOG_INFO << "File send success";
    close_input_file();
    clear_read_vars();

    do_read();
}

/**
 * Close upload file
 */
void FileServerSess::close_input_file()
{
    m_input_file.close();
    m_mode = FSERV_AUTH;
}

/**
 * Open file to download
 */
void FileServerSess::open_output_file()
{
    m_output_file.open(m_filename, std::ios_base::binary);

    if (!m_output_file) {
        GAMEAP_LOG_ERROR << "failed to open " << m_filename;
        return;
    }

    GAMEAP_LOG_DEBUG << "File opened: " << m_filename;
}

/**
 * Write file
 * @param length
 */
void FileServerSess::write_file(size_t length)
{
    auto self(shared_from_this());

    if (m_filesize == 0) {
        close_output_file();
        clear_read_vars();

        write_ok("Empty file");
        return;
    }

    if ( ! m_output_file.eof() && m_output_file.tellp() < (std::streamsize)m_filesize) {
        m_output_file.write((char*) &m_read_buf, length);
        GAMEAP_LOG_DEBUG << "Write file";

        if (m_output_file.bad()) {
            GAMEAP_LOG_ERROR << "File write error";

            close_output_file();
            clear_read_vars();

            response_msg(1, "File receiving error", true);
            return;
        }

        if (m_output_file.tellp() >= (std::streamsize)m_filesize) {
            GAMEAP_LOG_INFO << "File received: " << m_filename;

            close_output_file();
            clear_read_vars();

            write_ok();
        } else {
            do_read();
        }

    } else {
        close_output_file();
        clear_read_vars();

        response_msg(1, "File receiving error", true);
    }
}

/**
 * Close downloaded file
 */
void FileServerSess::close_output_file()
{
    m_output_file.close();
    m_mode = FSERV_AUTH;

    m_filename = "";
    m_filesize = 0;

    GAMEAP_LOG_INFO << "File closed";
}

/**
 * Check if read completed
 *
 * @param length
 * @return
 */
size_t FileServerSess::read_complete(size_t length)
{
    if (m_read_length <= 4) return 0;

    int found = 0;
    for (size_t i = m_read_length; i > m_read_length-length && found < 4; i--) {
        if (m_read_buf[i-1] == FSERV_END_SYMBOL) found++;
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
