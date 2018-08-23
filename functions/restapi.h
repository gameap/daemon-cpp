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

    Json::Value get(const std::string& uri);
}

#endif //GDAEMON_RESTAPI_H
