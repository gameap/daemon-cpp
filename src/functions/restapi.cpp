#include "restapi.h"

#include <memory>

#include <restclient-cpp/restclient.h>
#include <json/json.h>

#include "consts.h"
#include "log.h"
#include "restapi.h"
#include "config.h"
#include "state.h"
#include "gstring.h"

namespace GameAP {
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
        Config& config = Config::getInstance();
        auto conn = RestClient::Connection(config.api_host);

        std::string uri = "/gdaemon_api/get_token";
        conn.AppendHeader("Authorization", "Bearer " + config.api_key);
        conn.AppendHeader("Accept", "application/json");
        conn.SetUserAgent("GameAP Daemon");
        conn.FollowRedirects(true);
        conn.FollowRedirects(true, 3);

        RestClient::Response response = conn.get(uri);

        if (response.code != 200) {
            GAMEAP_LOG_ERROR << "RestClient HTTP response code (GET): " << response.code;
            GAMEAP_LOG_DEBUG_ERROR << "URL: " << config.api_host << uri;

            if (!response.body.empty()) {
                if (response.body.length() > 200) {
                    GAMEAP_LOG_DEBUG_ERROR << "RestClient HTTP response: " << response.body.substr(0, 200) << " ...";
                } else {
                    GAMEAP_LOG_DEBUG_ERROR << "RestClient HTTP response: " << response.body;
                }
            }

            m_errors_count++;
            throw Rest::RestapiException("RestClient error. Token getting error.");
        } else {
            m_errors_count = 0;

            Json::Value jvalue;
            Json::Reader jreader(Json::Features::strictMode());

            if (jreader.parse(response.body, jvalue, false)) {
                if (!jvalue["token"].isString()) {
                    throw Rest::RestapiException("Invalid token");
                }

                Rest::m_api_token = jvalue["token"].asString();

                State& state = State::getInstance();

                // Timediff

                if (!jvalue["timestamp"].isNull()) {
                    time_t panel_time = jvalue["timestamp"].isUInt()
                                             ? getJsonUInt(jvalue["timestamp"])
                                             : time(nullptr);

                    time_t diff = time(nullptr) - panel_time;
                    state.set(STATE_PANEL_TIMEDIFF, std::to_string(diff));
                }
            } else {
                throw Rest::RestapiException("JSON Parse error");
            }
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
        Config& config = Config::getInstance();
        auto conn = RestClient::Connection(config.api_host);

        conn.AppendHeader("X-Auth-Token", m_api_token);
        conn.AppendHeader("Accept", "application/json");

        conn.SetUserAgent("GameAP Daemon");
        conn.FollowRedirects(true);
        conn.FollowRedirects(true, 3);

        RestClient::Response response = conn.get(uri);

        if (response.code != 200) {
            if (response.code == 401) {
                get_token();
            }

            m_errors_count++;

            GAMEAP_LOG_ERROR << "RestClient HTTP response code (GET): " << response.code;
            GAMEAP_LOG_DEBUG_ERROR << "URL: " << config.api_host << uri;

            if (!response.body.empty()) {
                if (response.body.length() > 200) {
                    GAMEAP_LOG_DEBUG_ERROR << "RestClient HTTP response: " << response.body.substr(0, 200) << " ...";
                } else {
                    GAMEAP_LOG_DEBUG_ERROR << "RestClient HTTP response: " << response.body;
                }
            }

            throw Rest::RestapiException("RestClient error");
        } else {
            m_errors_count = 0;

            Json::Value jvalue;
            Json::Reader jreader(Json::Features::strictMode());

            if (jreader.parse(response.body, jvalue, false)) {
                return jvalue;
            } else {
                throw Rest::RestapiException("JSON Parse error");
            }
        }
    }

    /**
     *
     * @param uri
     * @param data
     */
    void Rest::post(const std::string& uri, Json::Value data)
    {
        Config& config = Config::getInstance();

        Json::FastWriter jwriter;

        auto conn = RestClient::Connection(config.api_host);

        conn.AppendHeader("X-Auth-Token", m_api_token);
        conn.AppendHeader("Content-Type", "application/json");
        conn.AppendHeader("Accept", "application/json");

        conn.SetUserAgent("GameAP Daemon");
        conn.FollowRedirects(true);
        conn.FollowRedirects(true, 3);

        std::string write_data = jwriter.write(data);
        RestClient::Response response = conn.post(uri, write_data);

        if (response.code == 201) {
            m_errors_count = 0;
            GAMEAP_LOG_DEBUG << "API. Resource Created";
        } else if (response.code == 422) {
            GAMEAP_LOG_ERROR << "RestClient HTTP response code (POST): " << response.code;
            GAMEAP_LOG_DEBUG_ERROR << "URL: " << config.api_host << uri;

            Json::Value jvalue;
            Json::Reader jreader(Json::Features::strictMode());

            GAMEAP_LOG_DEBUG_ERROR << "RestClient Request: " << write_data;

            if (jreader.parse(response.body, jvalue, false)) {
                GAMEAP_LOG_DEBUG_ERROR << "Error: " << jvalue["message"].asString();
            } else {
                GAMEAP_LOG_DEBUG_ERROR << "Error: " << response.body;
            }

        } else {
            if (response.code == 401) {
                get_token();
            }

            m_errors_count++;

            GAMEAP_LOG_ERROR << "RestClient HTTP response code (POST): " << response.code;
            GAMEAP_LOG_DEBUG_ERROR << "URL: " << config.api_host << uri;
            GAMEAP_LOG_VERBOSE_ERROR << "RestClient Request: " << write_data;

            if (!response.body.empty()) {
                if (response.body.length() > 200) {
                    GAMEAP_LOG_DEBUG_ERROR << "RestClient HTTP response: " << response.body.substr(0, 200) << " ...";
                } else {
                    GAMEAP_LOG_DEBUG_ERROR << "RestClient HTTP response: " << response.body;
                }
            }

            throw Rest::RestapiException("RestClient error");
        }
    }

    /**
     *
     * @param uri
     * @param data
     */
    void Rest::put(const std::string& uri, Json::Value data)
    {
        Config& config = Config::getInstance();

        Json::FastWriter jwriter;
        auto conn = RestClient::Connection(config.api_host);

        conn.AppendHeader("X-Auth-Token", m_api_token);
        conn.AppendHeader("Accept", "application/json");
        conn.AppendHeader("Content-Type", "application/json");

        conn.SetUserAgent("GameAP Daemon");
        conn.FollowRedirects(true);
        conn.FollowRedirects(true, 3);

        std::string write_data = jwriter.write(data);
        RestClient::Response response = conn.put(uri, write_data);

        if (response.code == 200) {
            m_errors_count = 0;
            GAMEAP_LOG_DEBUG << "API. Resource Updated";
        } else if (response.code == 201) {
            m_errors_count = 0;
            GAMEAP_LOG_DEBUG << "API. Resource Created";
        } else {
            if (response.code == 401) {
                get_token();
            }

            m_errors_count++;

            GAMEAP_LOG_ERROR << "RestClient HTTP response code (PUT): " << response.code;
            GAMEAP_LOG_DEBUG_ERROR << "URL: " << config.api_host << uri;

            GAMEAP_LOG_VERBOSE_ERROR << "RestClient Request: " << write_data;

            if (!response.body.empty()) {
                if (response.body.length() > 200) {
                    GAMEAP_LOG_DEBUG_ERROR << "RestClient HTTP response: " << response.body.substr(0, 200) << " ...";
                } else {
                    GAMEAP_LOG_DEBUG_ERROR << "RestClient HTTP response: " << response.body;
                }
            }

            throw Rest::RestapiException("RestClient error");
        }
    }

    /**
     *
     * @param uri
     * @param data
     */
    void Rest::patch(const std::string& uri, Json::Value data)
    {
        Config& config = Config::getInstance();

        Json::FastWriter jwriter;
        auto conn = RestClient::Connection(config.api_host);

        conn.AppendHeader("X-Auth-Token", m_api_token);
        conn.AppendHeader("Accept", "application/json");
        conn.AppendHeader("Content-Type", "application/json");

        conn.SetUserAgent("GameAP Daemon");
        conn.FollowRedirects(true);
        conn.FollowRedirects(true, 3);

        std::string write_data = jwriter.write(data);
        RestClient::Response response = conn.patch(uri, write_data);

        if (response.code == 200) {
            GAMEAP_LOG_DEBUG << "API. Resource Updated";
        } else if (response.code == 201) {
            GAMEAP_LOG_DEBUG << "API. Resource Created";
        } else {
            if (response.code == 401) {
                get_token();
            }

            GAMEAP_LOG_ERROR << "RestClient HTTP response code (PATCH): " << response.code;
            GAMEAP_LOG_DEBUG_ERROR << "URL: " << config.api_host << uri;

            GAMEAP_LOG_VERBOSE_ERROR << "RestClient Request: " << write_data;

            if (!response.body.empty()) {
                if (response.body.length() > 200) {
                    GAMEAP_LOG_DEBUG_ERROR << "RestClient HTTP response: " << response.body.substr(0, 200) << " ...";
                } else {
                    GAMEAP_LOG_DEBUG_ERROR << "RestClient HTTP response: " << response.body;
                }
            }

            throw Rest::RestapiException("RestClient error");
        }
    }
}