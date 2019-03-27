#ifndef GDAEMON_RESTAPI_H
#define GDAEMON_RESTAPI_H

#include <cstdio>
#include <iostream>

#include "restclient-cpp/connection.h"
#include <restclient-cpp/restclient.h>

#include <json/json.h>


namespace Gameap {

    class Rest {
    public:
        class RestapiException : public std::exception {
        public:
            RestapiException(std::string const &msg);

            ~RestapiException() override;

            char const *what() const throw() override;

        protected:
            std::string msg_;
        };

        static int get_token();
        static Json::Value get(const std::string &uri);
        static void post(const std::string &uri, Json::Value data);
        static void put(const std::string &uri, Json::Value data);
        static void patch(const std::string &uri, Json::Value data);
    private:
        static std::string m_api_token;
        static int m_errors_count;
    };
}


#endif //GDAEMON_RESTAPI_H
