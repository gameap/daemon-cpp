#ifndef GDAEMON_GSYSTEM_H
#define GDAEMON_GSYSTEM_H

#include <string>

#include <boost/process.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/filesystem.hpp>

#include "typedefs.h"

#define EXIT_SUCCESS_CODE           0
#define EXIT_ERROR_CODE             1
#define EXIT_CRITICAL_ERROR_CODE    2

#define PROCESS_FAIL       -1
#define PROCESS_SUCCESS     0
#define PROCESS_WORKING     1

namespace GameAP {
    namespace fs = boost::filesystem;
    namespace bp = boost::process;

    int exec(const std::string cmd, std::function<void (std::string)> callback);
    int exec(const std::string cmd, std::string &out);
    boost::process::child exec(const std::string cmd, boost::process::pipe &out);

    void privileges_down(const std::string & username);
    void privileges_retrieve();

    bp::environment load_env();

    bool copy_dir(
        const fs::path &source,
        const fs::path &destination
    );

#ifdef _WIN32
    extern std::unordered_map<unsigned int, std::thread> functions_gsystem_threads;
#endif

    pid_t run_process(std::function<void (void)> callback);
    int child_process_status(pid_t pid);

    // Shared memory
    void * shared_map_memory(size_t size);
    void destroy_shared_map_memory(void * ptr, size_t size);
}
#endif //GDAEMON_GSYSTEM_H