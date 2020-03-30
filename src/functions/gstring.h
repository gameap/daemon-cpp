#include <string>

#include <json/json.h>

std::string str_replace(const std::string& search, const std::string& replace, const std::string& subject);

std::string string_format(const std::string fmt_str, ...);

std::string random(size_t size);

unsigned int getJsonUInt(Json::Value json);

time_t human_to_timestamp(const std::string &str);