#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/string.hpp>

#include "typedefs.h"
#include "config.h"

int Config::parse()
{
    boost::property_tree::ptree pt;
    std::string allowed_ip_str;

    try {
        boost::optional<std::string> buf;
        boost::property_tree::ini_parser::read_ini("daemon.cfg", pt);

        ds_id           = pt.get<uint>("ds_id");
        listen_port     = pt.get<uint>("listen_port");

        daemon_login     = pt.get<std::string>("daemon_login");
        daemon_password  = pt.get<std::string>("daemon_password");

        db_driver       = pt.get<std::string>("db_driver");
        db_host         = pt.get<std::string>("db_host");
        db_user         = pt.get<std::string>("db_user");
        db_passwd       = pt.get<std::string>("db_passwd");
        db_name         = pt.get<std::string>("db_name");
        db_port         = pt.get<uint>("db_port");
        db_prefix       = pt.get<std::string>("db_prefix");

        buf = pt.get_optional<std::string>("if_list");
        if (*buf != "") {
            boost::split(if_list, *buf, boost::is_any_of(" "));
        }

        buf = pt.get_optional<std::string>("drives_list");
        if (*buf != "") {
            boost::split(drives_list, *buf, boost::is_any_of(" "));
        }

        boost::optional<ushort> bufushort;

        bufushort = pt.get_optional<ushort>("stats_update_period");
        if (*bufushort > 0) stats_update_period = *bufushort;

        bufushort = pt.get_optional<ushort>("stats_db_update_period");
        if (*bufushort > 0) stats_db_update_period = *bufushort;

        /*
        crypt_key = pt.get<std::string>("crypt_key");
        allowed_ip_str = pt.get<std::string>("allowed_ip");
        port = pt.get<int>("server_port");
        */

    }
    catch (std::exception &e) {
        std::cerr << "Parse config error: " << e.what() << std::endl;
        return -1;
    }

    /*
    if (allowed_ip_str != "") {
        allowed_ip = explode(",", allowed_ip_str);

        int size = allowed_ip.size();
        for (int i = 0; i < size; i++) {
            allowed_ip[i] = trim(allowed_ip[i]);
        }
    }
    */

    return 0;
}
