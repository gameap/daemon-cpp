#include "gtest/gtest.h"
#include "state.h"

namespace {
    TEST(state_test, set_get_test)
    {
        State& state = State::getInstance();

        state.set("set_get:some-key", "some-value");
        state.set("set_get:some-key2", "some-value2");

        ASSERT_EQ("some-value", state.get("set_get:some-key"));
        ASSERT_EQ("some-value2", state.get("set_get:some-key2"));

        ASSERT_THROW(state.get("set_get:invalid-key"), std::out_of_range);
    }

    TEST(state_test, exists_test)
    {
        State& state = State::getInstance();

        ASSERT_FALSE(state.exists("exists:some_key"));
        ASSERT_FALSE(state.exists("exists:some_key2"));

        state.set("exists:some_key", "some-value");
        ASSERT_TRUE(state.exists("exists:some_key"));
        ASSERT_FALSE(state.exists("exists:some_key2"));
    }

    TEST(state_test, remove_test)
    {
        State& state = State::getInstance();

        state.set("remove:some-key1", "some-value");
        state.set("remove:some-key2", "some-value");

        ASSERT_TRUE(state.exists("remove:some-key1"));

        state.remove("remove:some-key1");
        ASSERT_FALSE(state.exists("remove:some-key1"));
        ASSERT_TRUE(state.exists("remove:some-key2"));
    }
}