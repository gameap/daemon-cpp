#include "gtest/gtest.h"
#include "commands/output/ipc_cmd_output.h"

namespace GameAP {
    TEST(ipc_cmd_output, append_get_clear_test)
    {
        IpcCmdOutput output;
        output.init();

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

        output.append("h");
        output.append("e");
        output.append("l");
        output.append("lo");

        output.get(&output_content);
        ASSERT_EQ("h\ne\nl\nlo\n", output_content);

        output.clear();

        // Over buffer size
        for (auto i = 0; i < IPC_OUTPUT_BUFFER_SIZE + 1; i++) {
            output.append("+");
        }

        output.get(&output_content);

        ASSERT_EQ(IPC_OUTPUT_BUFFER_SIZE, output_content.length());
        ASSERT_EQ("+\n", output_content.substr(0, 2));

        output.clear();
        output.get(&output_content);
        ASSERT_EQ("", output_content);

        std::string big_string;
        for (auto i = 0; i < IPC_OUTPUT_BUFFER_SIZE + 1; i++) {
            big_string.append("~");
        }

        output.append(big_string);

        output.get(&output_content);

        ASSERT_EQ(IPC_OUTPUT_BUFFER_SIZE, output_content.length());
        ASSERT_EQ("~\n", output_content.substr(0, 2));
    }
}