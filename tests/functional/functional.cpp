#include "config.h"
#include "gtest/gtest.h"
#include "log.h"

#include <httplib.h>
#include <plog/Appenders/ConsoleAppender.h>

#ifdef __GNUC__
#include <thread>
#endif

int run_daemon();

namespace GameAP {
	TEST(functional, get_token_error_test)
	{
		static plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;

		plog::init<GameAP::MainLog>(plog::verbose, &consoleAppender);
		plog::init<GameAP::ErrorLog>(plog::verbose, &consoleAppender);

		using namespace httplib;

		Config& config = Config::getInstance();

		config.api_host = "localhost";
		config.api_key = "test-key";
		config.log_level = "verbose";

		bool webCalled = false;
		Server webServer;

		std::thread web_server([&]() {
			webServer.Get("/gdaemon_api/get_token", [&](const Request& req, httplib::Response& res) {
				res.status = 500;
				res.set_content("Test 500", "text/plain");

				webCalled = true;
			});

			webServer.listen("localhost", 80);
		});

		int runResult = run_daemon();

		ASSERT_EQ(-1, runResult);
		ASSERT_TRUE(webCalled);

		webServer.stop();

		web_server.join();
	}
}