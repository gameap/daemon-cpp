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

        listen_port     = pt.get_optional<uint>("listen_port").get_value_or(31717);

        api_host        = pt.get<std::string>("api_host");
        api_key         = pt.get<std::string>("api_key");

        daemon_login     = pt.get_optional<std::string>("daemon_login").get();
        daemon_password  = pt.get_optional<std::string>("daemon_password").get();

        certificate_chain_file = pt.get<std::string>("certificate_chain_file");
        private_key_file = pt.get<std::string>("private_key_file");
        private_key_password = pt.get_optional<std::string>("private_key_password").get();
        dh_file = pt.get<std::string>("dh_file");

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

    }
    catch (std::exception &e) {
        std::cerr << "Parse config error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
