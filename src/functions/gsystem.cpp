#include <string>
#include <functional>
#include <boost/asio.hpp>
#include <boost/process/extend.hpp>

#include "log.h"
#include "gsystem.h"
#include "consts.h"

#ifdef __linux__
#include <sys/mman.h>
#include <pwd.h>
#endif

#if defined(BOOST_POSIX_API)
    #define PROC_SHELL "sh"
    #define SHELL_PREF "-c"
#elif defined(BOOST_WINDOWS_API)
    #define PROC_SHELL "cmd"
    #define SHELL_PREF "/c"
#endif

namespace GameAP {
    using namespace boost::iostreams;

    namespace bp = boost::process;

    int exec(const std::string cmd, std::function<void (std::string)> callback)
    {
        int exit_code = -1;

        boost::asio::io_service ios;
        boost::asio::streambuf buf;
        bp::async_pipe ap(ios);

        try {
            bp::ipstream out_stream;
            bp::ipstream err_stream;

            bp::child child_proccess(
                    bp::search_path(PROC_SHELL),
                    bp::args={SHELL_PREF, cmd},
                    (bp::std_out & bp::std_err) > out_stream,
                    load_env(),
                    bp::extend::on_setup=[](auto & exec) {
                        #ifdef __linux__
                            setuid(geteuid());
                            setgid(getegid());
                        #endif
                    }
            );

            std::string s;

            while (child_proccess.running() && (std::getline(out_stream, s))) {
                 callback(s);
            }

            child_proccess.wait();
            exit_code = child_proccess.exit_code();
        } catch (boost::process::process_error &e) {
            GAMEAP_LOG_ERROR << "Execute error: " << e.what();
            return -1;
        }

        return exit_code;
    }

    int exec(const std::string cmd, std::string &out)
    {
        int exit_code;

        try {
            bp::ipstream out_stream;
            bp::ipstream err_stream;

            bp::child child_proccess(
                    bp::search_path(PROC_SHELL),
                    bp::args={SHELL_PREF, cmd},
                    bp::std_out > out_stream,
                    bp::std_err > err_stream,
                    load_env(),
                    bp::extend::on_setup=[](auto & exec) {
                        #ifdef __linux__
                            setuid(geteuid());
                            setgid(getegid());
                        #endif
                    }
            );

            std::string s;

            while (child_proccess.running() && (std::getline(out_stream, s) || std::getline(err_stream, s))) {
                out.append(s);
                out.append("\n");
            }

            child_proccess.wait();
            exit_code = child_proccess.exit_code();
        } catch (boost::process::process_error &e) {
            GAMEAP_LOG_ERROR << "Execute error: " << e.what();
            return -1;
        }

        return exit_code;
    }

    boost::process::child exec(const std::string cmd, boost::process::pipe &out)
    {
        try {
            bp::child child_proccess(
                    bp::search_path(PROC_SHELL),
                    bp::args={SHELL_PREF, cmd},
                    (bp::std_out & bp::std_err) > out,
                    load_env(),
                    bp::extend::on_setup=[](auto & exec) {
                        #ifdef __linux__
                            setuid(geteuid());
                            setgid(getegid());
                        #endif
                    }
            );

            return child_proccess;
        } catch (boost::process::process_error &e) {
            GAMEAP_LOG_ERROR << "Execute error: " << e.what();
        }
    }

    bp::environment load_env()
    {
        bp::environment env = static_cast<bp::environment>(boost::this_process::environment());

        #ifdef __linux__
            struct passwd *pw = getpwuid(geteuid());

            env["HOME"]                     = pw->pw_dir;
            env["USER"]                     = pw->pw_name;
        #endif

        env["GAMEAP_DAEMON_VERSION"]    = GAMEAP_DAEMON_VERSION;

        return env;
    }

    bool copy_dir(
            fs::path const & source,
            fs::path const & destination
    ) {
        try {
            // Check whether the function call is valid
            if(!fs::exists(source) || !fs::is_directory(source)) {
                // TODO: Add throw here
                GAMEAP_LOG_ERROR << "Source dir not found:  "
                                 << source.string()
                                 << " does not exist or is not a directory.";
                return false;
            }

            if (!fs::exists(destination)) {
                // Create the destination directory

                if (!fs::create_directory(destination)) {
                    // TODO: Add throw here
                    GAMEAP_LOG_ERROR << "Unable to create destination directory"
                                     << destination.string();
                    return false;
                }
            }

        } catch(fs::filesystem_error const & e) {
            // TODO: Add throw here
            GAMEAP_LOG_ERROR << e.what();
            return false;
        }

        // Iterate through the source directory
        for(fs::directory_iterator file(source);
            file != fs::directory_iterator(); ++file
                ) {
            try {
                fs::path current(file->path());
                if(fs::is_directory(current)) {
                    // Found directory: Recursion
                    if(!copy_dir(current, destination / current.filename())) {
                        return false;
                    }
                } else {
                    if (fs::is_regular_file(current)) {
                        fs::copy_file(current, destination / current.filename(), fs::copy_option::overwrite_if_exists);
                    }
                    else {
                        fs::copy(current, destination / current.filename());
                    }
                }
            } catch(fs::filesystem_error const & e) {
                // TODO: Add throw here
                GAMEAP_LOG_ERROR << e.what();
                return false;
            }
        }

        return true;
    }

    void privileges_down(const std::string & username)
    {
        #ifdef __linux__
                if (!username.empty()) {
                    passwd * pwd;
                    pwd = getpwnam(&username[0]);

                    if (pwd == nullptr) {
                        GAMEAP_LOG_ERROR << "Invalid user: " << username;
                        return;
                    }

                    if (pwd->pw_uid != getuid() && seteuid(pwd->pw_uid) == -1) {
                        GAMEAP_LOG_ERROR << "Failed to set Effective uid (" << pwd->pw_uid
                                         << "). Username: " << username
                                         << ". " << strerror(errno);
                    }

                    if (pwd->pw_gid != getgid() && setegid(pwd->pw_gid) == -1) {
                        GAMEAP_LOG_ERROR << "Failed to set Effective gid (" << pwd->pw_gid
                                         << "). Username: " << username
                                         << ". " << strerror(errno);
                    }
                }
        #endif
    }

    void privileges_retrieve()
    {
        #ifdef __linux__
            if (geteuid() != getuid() && seteuid(getuid()) == -1) {
                GAMEAP_LOG_ERROR << "Failed to set Effective uid (" << getuid() << ").";
            }

            if (getegid() != getgid() && setegid(getgid()) == -1) {
                GAMEAP_LOG_ERROR << "Failed to set Effective gid (" << getgid() << ").";
            }
        #endif
    }

    pid_t run_process(std::function<void (void)> callback)
    {
        #ifdef __linux__
            pid_t pid = fork();

            if (pid == -1) {
                GAMEAP_LOG_ERROR << "Fork failed\n";
            }

            if (pid == 0) {
                callback();
                exit(0);
            }

            return pid;
        #endif

        #ifdef _WIN32
            // TODO: Not implemented
        #endif
    }

    int child_process_status(pid_t pid)
    {
        #ifdef __linux__
            int status;

            if (waitpid(pid, &status, WNOHANG) > 0) {
                // Process complete

                return WIFEXITED(status) != 0 ? PROCESS_SUCCESS : PROCESS_FAIL;
            } else {
                return PROCESS_WORKING;
            }
        #endif

        #ifdef _WIN32
            // TODO: Not implemented
        #endif
    }

    void * shared_map_memory(size_t size)
    {
        #ifdef __linux__
            void * addr = mmap(nullptr, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

            if (addr == MAP_FAILED) {
                GAMEAP_LOG_ERROR << "Unable to create shared map memory\n";
                return nullptr;
            }

            return addr;
        #endif

        #ifdef _WIN32
                // TODO: Not implemented
        #endif
    }

    void destroy_shared_map_memory(void * ptr, size_t size)
    {
        #ifdef __linux__
            if (munmap(ptr, size) == -1) {
                GAMEAP_LOG_ERROR << "Unable to destroy shared map memory\n";
            }
        #endif

        #ifdef _WIN32
                // TODO: Not implemented
        #endif
    }

    // End GameAP namespace
}
