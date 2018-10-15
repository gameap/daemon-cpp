#include <iostream>

#include <cassert>
#include <functions/gsystem.h>

#include <boost/process.hpp>

void gsystem_test()
{
    std::string cmd;
    std::string out;

    // exec
    cmd = "echo TEST";

    GameAP::exec(cmd, out);
    assert(out == "TEST\n\n");

    // pipes
    cmd = "echo 1;sleep 5;echo 2;echo 3;sleep 5;echo 4;";
    out = "";
    boost::process::pipe out_pipe = boost::process::create_pipe();

    boost::iostreams::file_descriptor_source source(out_pipe.source, boost::iostreams::close_handle);
    boost::iostreams::stream<boost::iostreams::file_descriptor_source> is(source);

    boost::process::child c = GameAP::exec(cmd, out_pipe);
    std::string s;
    time_t start_time;
    time(&start_time);
    int counter = 0;
    while (!is.eof()) {
        std::getline(is, s);

        switch (counter) {
            case 0:
                assert(s == "1");
                assert((time(nullptr)-start_time) == 0);
                break;

            case 1:
                assert(s == "2");
                assert((time(nullptr)-start_time) == 5);
                break;

            case 2:
                assert(s == "3");
                assert((time(nullptr)-start_time) == 5);
                break;

            case 3:
                assert(s == "4");
                assert((time(nullptr)-start_time) == 10);
                break;

            case 4:
                assert(s.empty());
                time_t tim = time(nullptr);
                assert((time(nullptr)-start_time) == 10);
                break;
        }

        counter++;
    }

    auto exit_code = wait_for_exit(c);
}


int main()
{
    std::cout << "Tests Started" << std::endl;

    gsystem_test();
}
