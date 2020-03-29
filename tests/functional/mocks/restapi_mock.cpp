#include <memory>

#include <restclient-cpp/restclient.h>
#include <json/json.h>
#include <functional>
#include <queue>
#include <string>
#include <map>

#include "consts.h"
#include "log.h"
#include "config.h"
#include "state.h"
#include "functions/restapi.h"

std::queue<std::function<int ()>> restapi_mock_get_token;
std::queue<std::function<Json::Value ()>> restapi_mock_get;
std::queue<std::function<void ()>> restapi_mock_post;
std::queue<std::function<void ()>> restapi_mock_put;
std::queue<std::function<void ()>> restapi_mock_patch;

namespace Gameap {
    Rest::RestapiException::RestapiException(std::string const& msg) : msg_(msg) {}
    Rest::RestapiException::~RestapiException() {}
    char const* Rest::RestapiException::what() const throw() { return msg_.c_str(); }

    std::string Rest::m_api_token = "";
    int Rest::m_errors_count = 0;

    /**
     * Get Token. Set Token to variable api_token
     *
     * @return int
     */
    int Rest::get_token()
    {
        if (!restapi_mock_get_token.empty()) {
            auto f = restapi_mock_get_token.front();
            restapi_mock_get_token.pop();

            return f();
        }


        return 0;
    }

    /**
     * Query API. Return json value
     *
     * @param uri
     * @return JSON value
     */
    Json::Value Rest::get(const std::string& uri)
    {
        if (!restapi_mock_get.empty()) {
            auto f = restapi_mock_get.front();
            restapi_mock_get.pop();

            return f();
        }

        return nullptr;
    }

    /**
     *
     * @param uri
     * @param data
     */
    void Rest::post(const std::string& uri, Json::Value data)
    {
        if (!restapi_mock_post.empty()) {
            auto f = restapi_mock_post.front();
            restapi_mock_post.pop();

            f();
        }
    }

    /**
     *
     * @param uri
     * @param data
     */
    void Rest::put(const std::string& uri, Json::Value data)
    {
        if (!restapi_mock_put.empty()) {
            auto f = restapi_mock_put.front();
            restapi_mock_put.pop();

            f();
        }
    }

    /**
     *
     * @param uri
     * @param data
     */
    void Rest::patch(const std::string& uri, Json::Value data)
    {
        if (!restapi_mock_patch.empty()) {
            auto f = restapi_mock_patch.front();
            restapi_mock_patch.pop();

            f();
        }
    }
}