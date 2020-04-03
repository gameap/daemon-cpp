#include "gtest/gtest.h"
#include "commands/dedicated_server_cmd.h"

namespace GameAP {
    TEST(dedicated_server_cmd, success)
    {
        DedicatedServerCmd cmd = DedicatedServerCmd(DedicatedServerCmd::CMD_EXECUTE);
        cmd.set_option(DedicatedServerCmd::OPTION_SHELL_COMMAND, "./unit test");

        cmd.execute();

        ASSERT_TRUE(cmd.is_complete());
        ASSERT_TRUE(cmd.result());

        std::string out;
        cmd.output(&out);

        ASSERT_TRUE(out.find("./unit test\n\nExited with 0") != std::string::npos);
    }

    TEST(dedicated_server_cmd, empty_shell_command)
    {
        DedicatedServerCmd cmd = DedicatedServerCmd(DedicatedServerCmd::CMD_EXECUTE);

        cmd.execute();

        ASSERT_TRUE(cmd.is_complete());
        ASSERT_FALSE(cmd.result());

        std::string out;
        cmd.output(&out);

        ASSERT_EQ("Empty shell command\n", out);
    }

    TEST(dedicated_server_cmd, invalid_command)
    {
        DedicatedServerCmd cmd = DedicatedServerCmd(255);
        cmd.set_option(DedicatedServerCmd::OPTION_SHELL_COMMAND, "./unit test");

        cmd.execute();

        ASSERT_TRUE(cmd.is_complete());
        ASSERT_FALSE(cmd.result());

        std::string out;
        cmd.output(&out);

        ASSERT_EQ("Invalid dedicated_server_cmd command\n", out);
    }
}