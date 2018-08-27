#ifndef GDAEMON_RESTAPI_H
#define GDAEMON_RESTAPI_H

#include <cstdio>
#include <iostream>

#include "restclient-cpp/connection.h"
#include <restclient-cpp/restclient.h>

#include <jsoncpp/json/json.h>


namespace Gameap::Rest {
    class RestapiException: public std::exception
    {
    public:
        RestapiException(std::string const& msg);
        ~RestapiException() override;
        char const* what() const throw() override;
    protected:
        std::string msg_;
    };

    int get_token();
    Json::Value get(const std::string& uri);
    void post(const std::string& uri, Json::Value data);
    void put(const std::string& uri, Json::Value data);
}

#endif //GDAEMON_RESTAPI_H
