#include "config.h"
#include "gtest/gtest.h"
#include "log.h"
#include "functions/restapi.h"

#include <functional>
#include <queue>

#include <plog/Appenders/ConsoleAppender.h>

#ifdef __GNUC__
#include <thread>
#endif

int run_daemon();

extern std::queue<std::function<int ()>> restapi_mock_get_token;
extern std::queue<std::function<Json::Value ()>> restapi_mock_get;
extern std::queue<std::function<void (const std::string& uri, Json::Value data)>> restapi_mock_post;
extern std::queue<std::function<void (const std::string& uri, Json::Value data)>> restapi_mock_put;
extern std::queue<std::function<void (const std::string& uri, Json::Value data)>> restapi_mock_patch;

namespace GameAP {
	TEST(common, get_token_error_test)
	{
		static plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;

		plog::init<GameAP::MainLog>(plog::verbose, &consoleAppender);
		plog::init<GameAP::ErrorLog>(plog::verbose, &consoleAppender);

        bool restCalled = false;

        restapi_mock_get_token.push([&]() -> int
        {
            restCalled = true;
            throw Rest::RestapiException("RestClient error");
        });

		Config& config = Config::getInstance();

		config.api_host = "localhost";
		config.api_key = "test-key";
		config.log_level = "verbose";

		int runResult = run_daemon();

		ASSERT_EQ(-1, runResult);
		ASSERT_TRUE(restCalled);
	}
}