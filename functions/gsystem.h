#include <boost/process.hpp>
#include <boost/iostreams/stream.hpp>

namespace GameAP {
    int exec(const std::string &cmd, std::string &out);
    int exec(const std::string &cmd, boost::process::pipe &out);
}
