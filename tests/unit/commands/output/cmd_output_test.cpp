#include "gtest/gtest.h"
#include "commands/output/cmd_output.h"

namespace GameAP {
    TEST(cmd_output, append_get_clear_test)
    {
        CmdOutput output;

        output.append("some-line");

        std::string output_content;
        output.get(&output_content);

        ASSERT_EQ("some-line\n", output_content);

        output.append("another-line");
        output.get(&output_content);

        ASSERT_EQ("some-line\nanother-line\n", output_content);

        output.clear();
        output.get(&output_content);
        ASSERT_EQ("", output_content);
    }
}