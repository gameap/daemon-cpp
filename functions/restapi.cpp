#include <cstdio>
#include <iostream>

#include "restclient-cpp/connection.h"
#include <restclient-cpp/restclient.h>

#include <jsoncpp/json/json.h>

#include "restapi.h"
#include "config.h"

namespace Gameap::Rest {
    RestapiException::RestapiException(std::string const& msg) : msg_(msg) {}
    RestapiException::~RestapiException() {}
    char const* RestapiException::what() const throw() { return msg_.c_str(); }

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

        conn->AppendHeader("Authorization", "Bearer " + config.api_key);
        RestClient::Response r = conn->get(uri);

        if (r.code != 200) {
            std::cerr << "RestClient error: " << r.code << std::endl;
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