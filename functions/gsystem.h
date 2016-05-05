#include <boost/process.hpp>
#include <boost/iostreams/stream.hpp>

namespace GameAP {
    int exec(const std::string &cmd, std::string &out);
    boost::process::child exec(std::string cmd, boost::process::pipe &out);
}
