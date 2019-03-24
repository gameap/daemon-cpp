#include <boost/process.hpp>
#include <boost/iostreams/stream.hpp>

#define EXIT_SUCCESS_CODE           0
#define EXIT_ERROR_CODE             1
#define EXIT_CRITICAL_ERROR_CODE    2

namespace GameAP {
    int exec(const std::string cmd, std::string &out);
    boost::process::child exec(const std::string cmd, boost::process::pipe &out);
    // void exec(const std::string &cmd, std::future<std::string> &out);
}
