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

        std::cout << "Reading cfg file " << cfg_file.c_str() << std::endl;
        boost::property_tree::ini_parser::read_ini(cfg_file.c_str(), pt);

        ds_id           = pt.get<uint>("ds_id");

        listen_port = pt.count("listen_port") > 0
                      ? pt.get<uint>("listen_port")
                      : 31717;

        api_host        = pt.get<std::string>("api_host");
        api_key         = pt.get<std::string>("api_key");

        daemon_login     = pt.get<std::string>("daemon_login");
        daemon_password  = pt.get<std::string>("daemon_password");

        pub_key_file    = pt.get<std::string>("pub_key_file");

        certificate_chain_file = pt.get<std::string>("certificate_chain_file");
        private_key_file = pt.get<std::string>("private_key_file");
        private_key_password = pt.get_optional<std::string>("private_key_password").get();
        tmp_dh_file = pt.get<std::string>("tmp_dh_file");

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
