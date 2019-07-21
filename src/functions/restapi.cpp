#include <cstdio>
#include <iostream>
#include <memory>

#include <restclient-cpp/restclient.h>

#include <json/json.h>

#include "restapi.h"
#include "config.h"

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
        Config& config = Config::getInstance();

        auto conn = std::make_shared<RestClient::Connection>(config.api_host);

        std::string uri = "/gdaemon_api/get_token";
        conn->AppendHeader("Authorization", "Bearer " + config.api_key);
        conn->AppendHeader("Accept", "application/json");
        RestClient::Response response = conn->get(uri);

        if (response.code != 200) {
            std::cerr << "RestClient HTTP response code (GET): " << response.code << std::endl;
            std::cerr << "URL: " << config.api_host << uri << std::endl;

            if (!response.body.empty()) {
                if (response.body.length() > 200) {
                    std::cerr << "RestClient HTTP response: " << response.body.substr(0, 200) << " ..." << std::endl;
                } else {
                    std::cerr << "RestClient HTTP response: " << response.body << std::endl;
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

        auto conn = std::make_shared<RestClient::Connection>(config.api_host);

        conn->AppendHeader("X-Auth-Token", m_api_token);
        conn->AppendHeader("Accept", "application/json");
        RestClient::Response response = conn->get(uri);

        if (response.code != 200) {
            if (response.code == 401) {
                get_token();
            }

            m_errors_count++;

            std::cerr << "RestClient HTTP response code (GET): " << response.code << std::endl;
            std::cerr << "URL: " << config.api_host << uri << std::endl;

            if (!response.body.empty()) {
                if (response.body.length() > 200) {
                    std::cerr << "RestClient HTTP response: " << response.body.substr(0, 200) << " ..." << std::endl;
                } else {
                    std::cerr << "RestClient HTTP response: " << response.body << std::endl;
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

        auto conn = std::make_shared<RestClient::Connection>(config.api_host);

        conn->AppendHeader("X-Auth-Token", m_api_token);
        conn->AppendHeader("Content-Type", "application/json");
        conn->AppendHeader("Accept", "application/json");

        std::string write_data = jwriter.write(data);
        RestClient::Response response = conn->post(uri, write_data);

        if (response.code == 201) {
            m_errors_count = 0;
            std::cout << "API. Resource Created" << std::endl;
        } else if (response.code == 422) {
            std::cerr << "RestClient HTTP response code (POST): " << response.code << std::endl;
            std::cerr << "URL: " << config.api_host << uri << std::endl;

            Json::Value jvalue;
            Json::Reader jreader(Json::Features::strictMode());

            std::cerr << "RestClient Request: " << write_data << '\n';

            if (jreader.parse(response.body, jvalue, false)) {
                std::cerr << "Error: " << jvalue["message"].asString() << std::endl;
            } else {
                std::cerr << "Error: " << response.body << std::endl;
            }

        } else {
            if (response.code == 401) {
                get_token();
            }

            m_errors_count++;

            std::cerr << "RestClient HTTP response code (POST): " << response.code << std::endl;
            std::cerr << "URL: " << config.api_host << uri << std::endl;

            if (!response.body.empty()) {
                if (response.body.length() > 200) {
                    std::cerr << "RestClient HTTP response: " << response.body.substr(0, 200) << " ..." << std::endl;
                } else {
                    std::cerr << "RestClient HTTP response: " << response.body << std::endl;
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

        auto conn = std::make_shared<RestClient::Connection>(config.api_host);

        conn->AppendHeader("X-Auth-Token", m_api_token);
        conn->AppendHeader("Accept", "application/json");
        conn->AppendHeader("Content-Type", "application/json");

        std::string write_data = jwriter.write(data);
        RestClient::Response response = conn->put(uri, write_data);

        if (response.code == 200) {
            m_errors_count = 0;
            std::cout << "API. Resource Updated" << std::endl;
        } else if (response.code == 201) {
            m_errors_count = 0;
            std::cout << "API. Resource Created" << std::endl;
        } else {
            if (response.code == 401) {
                get_token();
            }

            m_errors_count++;

            std::cerr << "RestClient HTTP response code (PUT): " << response.code << std::endl;
            std::cerr << "URL: " << config.api_host << uri << std::endl;

            std::cerr << "RestClient Request: " << write_data << '\n';

            if (!response.body.empty()) {
                if (response.body.length() > 200) {
                    std::cerr << "RestClient HTTP response: " << response.body.substr(0, 200) << " ..." << std::endl;
                } else {
                    std::cerr << "RestClient HTTP response: " << response.body << std::endl;
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

        auto conn = std::make_shared<RestClient::Connection>(config.api_host);

        conn->AppendHeader("X-Auth-Token", m_api_token);
        conn->AppendHeader("Accept", "application/json");
        conn->AppendHeader("Content-Type", "application/json");

        std::string write_data = jwriter.write(data);
        RestClient::Response response = conn->patch(uri, write_data);

        if (response.code == 200) {
            std::cout << "API. Resource Updated" << std::endl;
        } else if (response.code == 201) {
            std::cout << "API. Resource Created" << std::endl;
        } else {
            if (response.code == 401) {
                get_token();
            }

            std::cerr << "RestClient HTTP response code (PATCH): " << response.code << std::endl;
            std::cerr << "URL: " << config.api_host << uri << std::endl;

            std::cerr << "RestClient Request: " << write_data << '\n';

            if (!response.body.empty()) {
                if (response.body.length() > 200) {
                    std::cerr << "RestClient HTTP response: " << response.body.substr(0, 200) << " ..." << std::endl;
                } else {
                    std::cerr << "RestClient HTTP response: " << response.body << std::endl;
                }
            }

            throw Rest::RestapiException("RestClient error");
        }
    }
}