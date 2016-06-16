#include <boost/filesystem.hpp>

void run_daemon();

int main(int argc, char** argv)
{
    boost::filesystem::path exe_path( boost::filesystem::initial_path<boost::filesystem::path>() );
    exe_path = boost::filesystem::system_complete( boost::filesystem::path( argv[0] ) );
    boost::filesystem::current_path(exe_path.parent_path());

    run_daemon();
}
