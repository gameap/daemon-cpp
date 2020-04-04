#include "gstring.h"

#include <cstring>
#include <stdarg.h>  // For va_start, etc.
#include <memory>    // For std::unique_ptr
#include <iomanip>
#include <sstream>

#include <time.h>

#include <algorithm>

#include <json/json.h>

// ---------------------------------------------------------------------

std::string str_replace(const std::string& search, const std::string& replace, const std::string& subject)
{
    std::string str = subject;
    size_t pos = 0;
    while((pos = str.find(search, pos)) != std::string::npos)
    {
        str.replace(pos, search.length(), replace);
        pos += replace.length();
    }
    return str;
}

std::string string_format(const std::string fmt_str, ...) {
    int final_n, n = ((int)fmt_str.size()) * 2; 
    std::string str;
    std::unique_ptr<char[]> formatted;
    va_list ap;
    while(1) {
        formatted.reset(new char[n]);
        strcpy(&formatted[0], fmt_str.c_str());
        va_start(ap, fmt_str);
        final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
        va_end(ap);
        if (final_n < 0 || final_n >= n)
            n += abs(final_n - n + 1);
        else
            break;
    }
    return std::string(formatted.get());
}

// ---------------------------------------------------------------------

std::string getFileExt(const std::string& s)
{

   size_t i = s.rfind('.', s.length());
   if (i != std::string::npos) {
      return(s.substr(i+1, s.length() - i));
   }

   return("");
}

// ---------------------------------------------------------------------

std::string random(size_t size)
{
    srand(time(nullptr));
    auto randchar = []() -> char
    {
        const char charset[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset)-1);
        return charset[rand() % max_index];
    };

    std::string rstr(16, 0);
    std::generate_n(rstr.begin(), size, randchar);

    return rstr;
}

unsigned int getJsonUInt(const Json::Value json)
{
    if (json.isNull()) {
        return 0;
    }

    if (json.isUInt()) {
        return json.asUInt();
    }

    if (json.isString()) {
        return (unsigned int)strtoul(json.asString().c_str(), nullptr, 10);
    }
    
    return 0;
}

time_t human_to_timestamp(const std::string &str)
{
    std::tm t = {};
    std::istringstream ss(str);

    ss >> std::get_time(&t, "%Y-%m-%d %H:%M:%S");

    if (ss.fail()) {
        return 0;
    }

    return timegm(&t);
}