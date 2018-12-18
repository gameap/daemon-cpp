#include <iostream>
#include "gsystem.h"

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

    // ---------------------------------------------------------------------

    int exec(const std::string cmd, std::string &out)
    {
        int exit_code;

        try {
            bp::ipstream out_stream;
            bp::ipstream err_stream;
            bp::child child_proccess(bp::search_path(PROC_SHELL), args={SHELL_PREF, cmd}, bp::std_out > out_stream, bp::std_err > err_stream);

            std::string s;

            while (child_proccess.running() && (std::getline(out_stream, s) || std::getline(err_stream, s))) {
                out.append(s);
                out.append("\n");
            }

            child_proccess.wait();
            exit_code = child_proccess.exit_code();
        } catch (boost::process::process_error &e) {
            std::cerr << "Execute error: " << e.what() << std::endl;
            return -1;
        }

        return exit_code;
    }

    // ---------------------------------------------------------------------

    boost::process::child exec(const std::string cmd, boost::process::pipe &out)
    {
        try {
            bp::child child_proccess(bp::search_path(PROC_SHELL), args={SHELL_PREF, cmd}, bp::std_out > out);
            return child_proccess;
        } catch (boost::process::process_error &e) {
            std::cerr << "Execute error: " << e.what() << std::endl;
        }
    }

    /*
    void exec(const std::string &cmd, std::future<std::string> &out)
    {
        boost::asio::io_service ios;

        std::future<std::string> data;

        child c(cmd,
                bp::std_in.close(),
                bp::std_out > out,
                bp::std_err > out,
                ios
        );

        ios.run();
    }
     */

    // End GameAP namespace
}
