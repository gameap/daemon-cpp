#include <cstdio>
#include <iostream>

#include "restclient-cpp/connection.h"
#include <restclient-cpp/restclient.h>

#include <jsoncpp/json/json.h>

#include "restapi.h"
#include "config.h"

namespace Gameap::Rest {

    std::string api_token;

    RestapiException::RestapiException(std::string const& msg) : msg_(msg) {}
    RestapiException::~RestapiException() {}
    char const* RestapiException::what() const throw() { return msg_.c_str(); }

    int get_token()
    {
        Config& config = Config::getInstance();

        RestClient::Connection* conn = new RestClient::Connection(config.api_host);

        conn->AppendHeader("Authorization", "Bearer " + config.api_key);
        RestClient::Response r = conn->get("gdaemon_api/get_token");

        if (r.code != 200) {
            std::cerr << "RestClient HTTP response code: " << r.code << std::endl;
            throw RestapiException("RestClient error. Token getting error.");
        } else {
            Json::Value jvalue;
            Json::Reader jreader(Json::Features::strictMode());

            if (jreader.parse(r.body, jvalue, false)) {
                if (!jvalue["token"].isString()) {
                    throw RestapiException("Invalid token");
                }

                if (jvalue["token"].size() == 0 ) {
                    throw RestapiException("Token is null");
                }

                api_token = jvalue.asString();

            } else {
                throw RestapiException("JSON Parse error");
            }
        }
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
        RestClient::Response r = conn->get(uri);

        if (r.code != 200) {
            std::cerr << "RestClient HTTP response code: " << r.code << std::endl;
            throw RestapiException("RestClient error");
        } else {
            Json::Value jvalue;
            Json::Reader jreader(Json::Features::strictMode());

            if (jreader.parse(r.body, jvalue, false)) {
                return jvalue;
            } else {
                throw RestapiException("JSON Parse error");
            }
        }
    }
}