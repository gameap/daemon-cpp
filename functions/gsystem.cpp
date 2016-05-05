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

// ---------------------------------------------------------------------

int exec(const std::string &cmd, std::string &out)
{
    boost::process::pipe pout = boost::process::create_pipe();
    
    try {
        file_descriptor_sink sinkout(pout.sink, close_handle);
        
        child c = execute(
            run_exe(boost::process::shell_path()),
            set_args(std::vector<std::string>{PROC_SHELL, SHELL_PREF, cmd}),
            bind_stderr(sinkout),
            bind_stdout(sinkout),
            throw_on_error()
        );
    } catch (boost::system::system_error &e) {
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


// ---------------------------------------------------------------------

boost::process::child exec(std::string cmd, boost::process::pipe &out)
{
    boost::iostreams::file_descriptor_sink sinkout(out.sink, boost::iostreams::close_handle);

    return execute(
        run_exe(boost::process::shell_path()),
        set_args(std::vector<std::string>{PROC_SHELL, SHELL_PREF, cmd}),
        bind_stderr(sinkout),
        bind_stdout(sinkout),
        throw_on_error()
    );
}

// End GameAP namespace
}
