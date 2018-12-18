#include <boost/process.hpp>
#include <boost/iostreams/stream.hpp>

namespace GameAP {
    int exec(const std::string cmd, std::string &out);
    boost::process::child exec(const std::string cmd, boost::process::pipe &out);
    // void exec(const std::string &cmd, std::future<std::string> &out);
}
