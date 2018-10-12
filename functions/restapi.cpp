#include <cstdio>
#include <iostream>

#include <restclient-cpp/restclient.h>

#include <jsoncpp/json/json.h>

#include "restapi.h"
#include "config.h"

namespace Gameap { namespace Rest {

    /**
     * API Token
     */
    std::string api_token;

    RestapiException::RestapiException(std::string const& msg) : msg_(msg) {}
    RestapiException::~RestapiException() {}
    char const* RestapiException::what() const throw() { return msg_.c_str(); }

    /**
     * Get Token. Set Token to variable api_token
     *
     * @return int
     */
    int get_token()
    {
        Config& config = Config::getInstance();

        RestClient::Connection* conn = new RestClient::Connection(config.api_host);

        std::string uri = "/gdaemon_api/get_token";
        conn->AppendHeader("Authorization", "Bearer " + config.api_key);
        conn->AppendHeader("Content-Type", "application/json");
        RestClient::Response response = conn->get(uri);

        if (response.code != 200) {
            std::cerr << "RestClient HTTP response code: " << response.code << std::endl;
            std::cerr << "URL: " << config.api_host << uri << std::endl;
            throw RestapiException("RestClient error. Token getting error.");
        } else {
            Json::Value jvalue;
            Json::Reader jreader(Json::Features::strictMode());

            if (jreader.parse(response.body, jvalue, false)) {
                if (!jvalue["token"].isString()) {
                    throw RestapiException("Invalid token");
                }

                /*
                if (jvalue["token"].size() == 0 ) {
                    throw RestapiException("Token is null");
                }
                */

                api_token = jvalue["token"].asString();

            } else {
                throw RestapiException("JSON Parse error");
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
    Json::Value get(const std::string& uri)
    {
        Config& config = Config::getInstance();

        RestClient::Connection* conn = new RestClient::Connection(config.api_host);

        conn->AppendHeader("X-Auth-Token", api_token);
        conn->AppendHeader("Content-Type", "application/json");
        RestClient::Response response = conn->get(uri);

        if (response.code != 200) {
            std::cerr << "RestClient HTTP response code: " << response.code << std::endl;
            std::cerr << "URL: " << config.api_host << uri << std::endl;

            throw RestapiException("RestClient error");
        } else {
            Json::Value jvalue;
            Json::Reader jreader(Json::Features::strictMode());

            if (jreader.parse(response.body, jvalue, false)) {
                return jvalue;
            } else {
                throw RestapiException("JSON Parse error");
            }
        }
    }

    /**
     *
     * @param uri
     * @param data
     */
    void post(const std::string& uri, Json::Value data)
    {
        Config& config = Config::getInstance();

        Json::FastWriter jwriter;

        RestClient::Connection* conn = new RestClient::Connection(config.api_host);

        conn->AppendHeader("X-Auth-Token", api_token);
        conn->AppendHeader("Content-Type", "application/json");

        std::string dt = jwriter.write(data);

        RestClient::Response response = conn->post(uri, jwriter.write(data));

        if (response.code == 201) {
            std::cout << "API. Resource Created" << std::endl;
        } else {
            std::cerr << "RestClient HTTP response code: " << response.code << std::endl;
            std::cerr << "URL: " << config.api_host << uri << std::endl;

            throw RestapiException("RestClient error");
        }
    }

    /**
     *
     * @param uri
     * @param data
     */
    void put(const std::string& uri, Json::Value data)
    {
        Config& config = Config::getInstance();

        Json::FastWriter jwriter;

        RestClient::Connection* conn = new RestClient::Connection(config.api_host);

        conn->AppendHeader("X-Auth-Token", api_token);
        conn->AppendHeader("Content-Type", "application/json");

        RestClient::Response response = conn->put(uri, jwriter.write(data));

        if (response.code == 200) {
            std::cout << "API. Resource Updated" << std::endl;
        } else if (response.code == 201) {
            std::cout << "API. Resource Created" << std::endl;
        } else {
            std::cerr << "RestClient HTTP response code: " << response.code << std::endl;
            std::cerr << "URL: " << config.api_host << uri << std::endl;

            throw RestapiException("RestClient error");
        }
    }
}}