#include <string>

#include "functions/gsystem.h"
#include "functions/gstring.h"
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

    using namespace boost::process;
    using namespace boost::iostreams;

    namespace bp = boost::process;

    int exec(const std::string cmd, std::function<void (std::string)> callback)
    {
        callback(cmd);
        return 0;
    }

    int exec(const std::string cmd, std::string &out)
    {
        int exit_code;

        out.append(cmd);
        out.append("\n");

        return exit_code;
    }

    boost::process::child exec(const std::string cmd, boost::process::pipe& out)
    {
        bp::child child_proccess(
            bp::search_path(PROC_SHELL),
            args = { SHELL_PREF, cmd },
            (bp::std_out & bp::std_err) > out,
            load_env()
        );

        return child_proccess;
    }

    void privileges_down(const std::string& username)
    {

    }

    void privileges_retrieve()
    {

    }

    bp::environment load_env()
    {
        bp::environment env = static_cast<bp::environment>(boost::this_process::environment());

        env["HOME"]                     = TESTS_ROOT_DIR;
        env["USER"]                     = "test";
        env["GAMEAP_DAEMON_VERSION"]    = GAMEAP_DAEMON_VERSION;

        return env;
    }

    bool copy_dir(
            fs::path const & source,
            fs::path const & destination
    ) {
        // Not copy
        return true;
    }

#ifdef _WIN32
    std::unordered_map<unsigned int, gsystem_thread*> functions_gsystem_threads;
#endif

    pid_t run_process(std::function<void(void)> callback)
    {
#ifdef __linux__
        pid_t pid = fork();

        if (pid == 0) {
            callback();
            exit(0);
        }

        return pid;
#endif

#ifdef _WIN32
        gsystem_thread * gsthread = new gsystem_thread;
        gsthread->result = -1;

        gsthread->thread = std::thread([=]() {
            callback();
            gsthread->result = 0;
        });

        pid_t thread_hash = std::hash<std::thread::id>{}(gsthread->thread.get_id());

        functions_gsystem_threads.insert(
            std::pair<unsigned int, gsystem_thread*>(thread_hash, gsthread)
        );

        return thread_hash;
#endif
    }

    int child_process_status(pid_t pid)
    {
#ifdef __linux__
        int status;

        if (waitpid(pid, &status, WNOHANG) > 0) {
            // Process complete

            return WIFEXITED(status) != 0 ? PROCESS_SUCCESS : PROCESS_FAIL;
        }
        else {
            return PROCESS_WORKING;
        }
#endif

#ifdef _WIN32
        auto itr = functions_gsystem_threads.find(pid);

        if (itr == functions_gsystem_threads.end()) {
            return PROCESS_SUCCESS;
        }

        if (itr->second->result == 0) {
            delete itr->second;
            functions_gsystem_threads.erase(itr);
            return PROCESS_SUCCESS;
        }

        return itr->second->thread.joinable() ? PROCESS_WORKING : PROCESS_FAIL;
#endif
    }

    void* shared_map_memory(size_t size)
    {
#ifdef __linux__
        void* addr = mmap(nullptr, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

        if (addr == MAP_FAILED) {
            return nullptr;
        }

        return addr;
#endif

#ifdef _WIN32
        return malloc(size);
#endif
    }

    void destroy_shared_map_memory(void* ptr, size_t size)
    {
#ifdef __linux__
        munmap(ptr, size);
#endif

#ifdef _WIN32
        free(ptr);
#endif
    }

    void fix_path_slashes(std::string &path)
    {
#ifdef _WIN32
        std::replace(path.begin(), path.end(), '/', '\\');
            path = str_replace("\\\\", "\\", path);
#else
        std::replace(path.begin(), path.end(), '\\', '/');
        path = str_replace("//", "/", path);
#endif
    }

    // End GameAP namespace
}
