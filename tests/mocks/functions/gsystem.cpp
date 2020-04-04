#include <string>

#include "functions/gsystem.h"
#include "consts.h"

#ifdef __linux__
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

    boost::process::child exec(const std::string cmd, boost::process::pipe &out)
    {
        // TODO: Implement
    }

    void privileges_down(const std::string username)
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

    // End GameAP namespace
}
