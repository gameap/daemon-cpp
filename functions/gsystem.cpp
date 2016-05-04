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
    using namespace boost::process::initializers;
    using namespace boost::iostreams;

    int exec(const std::string &cmd, std::string &out)
    {
        boost::process::pipe pout = boost::process::create_pipe();
        if (exec(cmd, pout) != 0) {
            return -1;
        }

        boost::iostreams::file_descriptor_source source(pout.source, boost::iostreams::close_handle);
        boost::iostreams::stream<boost::iostreams::file_descriptor_source> is(source);
        std::string s;

        while (!is.eof()) {
            std::getline(is, s);
            out = out + s + '\n';
        }
        
        return 0;
    }

    int exec(const std::string &cmd, boost::process::pipe &out)
    {
        try {
            // boost::process::pipe out = create_pipe();
            file_descriptor_sink sinkout(out.sink, close_handle);
            
            child c = execute(
                run_exe(boost::process::shell_path()),
                set_args(std::vector<std::string>{PROC_SHELL, SHELL_PREF, cmd}),
                bind_stderr(sinkout),
                bind_stdout(sinkout),
                throw_on_error()
            );

            auto exit_code = wait_for_exit(c);
            
        } catch (boost::system::system_error &e) {
            // std::cerr << "Error: " << e.what() << std::endl;
            return -1;
        }

        return 0;
    }
}
