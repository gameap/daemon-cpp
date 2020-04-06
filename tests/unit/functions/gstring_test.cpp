#include "gtest/gtest.h"

#include <stdlib.h>
#include <time.h>

#include "functions/gstring.h"


namespace GameAP {

    TEST(gstring, human_to_timestamp)
    {
        putenv("TZ=UTC");
        tzset();

        ASSERT_EQ(0, human_to_timestamp("1970-01-01 00:00:00"));
        ASSERT_EQ(1, human_to_timestamp("1970-01-01 00:00:01"));
        ASSERT_EQ(1586202033, human_to_timestamp("2020-04-06 19:40:33"));

        // TODO: Windows not working. Check
        // ASSERT_EQ(1586131200, human_to_timestamp("2020-04-06"));
        // ASSERT_EQ(-1, human_to_timestamp("INVALID VALUE"));
    }
}