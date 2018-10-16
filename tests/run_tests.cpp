#include <iostream>

#include <cassert>
#include <functions/gsystem.h>

#include <boost/process.hpp>

namespace bp = ::boost::process;

void gsystem_test()
{
    std::string cmd;
    std::string out;

    // exec
    out = "";
    cmd = "echo TEST";
    GameAP::exec(cmd, out);
    assert(out == "TEST\n");

    out = "";
    cmd = "echo ERROR > /dev/stderr";
    GameAP::exec(cmd, out);
    assert(out == "ERROR\n");

    // pipes
    cmd = "echo 1;sleep 5;echo 2;echo 3;sleep 5;echo 4;";
    out = "";
    bp::pipe out_pipe = bp::pipe();
    bp::child c = GameAP::exec(cmd, out_pipe);

    bp::ipstream is(out_pipe);
    std::string line;

    std::string s;
    time_t start_time;
    time(&start_time);
    int counter = 0;
    while (c.running() && std::getline(is, line)) {

        switch (counter) {
            case 0:
                assert(line == "1");
                assert((time(nullptr)-start_time) == 0);
                break;

            case 1:
                assert(line == "2");
                assert((time(nullptr)-start_time) == 5);
                break;

            case 2:
                assert(line == "3");
                assert((time(nullptr)-start_time) == 5);
                break;

            case 3:
                assert(line == "4");
                assert((time(nullptr)-start_time) == 10);
                break;

            case 4:
                assert(line.empty());
                time_t tim = time(nullptr);
                assert((time(nullptr)-start_time) == 10);
                break;
        }

        counter++;
    }

    c.wait();
}


int main()
{
    std::cout << "Tests Started" << std::endl;

    gsystem_test();
}
