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

    int exec(const std::string &cmd, std::string &out)
    {
        try {
            bp::ipstream is;
            bp::child child_proccess(cmd, bp::std_out > is, bp::std_err > is);

            std::string s;

            while (child_proccess.running() && std::getline(is, s)) {
                out.append(s);
                out.append("\n");
            }
        } catch (boost::system::system_error &e) {
            return -1;
        }

        return 0;
    }

    // ---------------------------------------------------------------------

    boost::process::child exec(std::string cmd, boost::process::pipe &out)
    {
        bp::child child_proccess(cmd, bp::std_out > out, bp::std_err > out);
        return child_proccess;
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
