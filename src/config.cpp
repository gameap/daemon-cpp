#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include "log.h"
#include "typedefs.h"
#include "config.h"

namespace fs = boost::filesystem;

int Config::parse()
{
    boost::property_tree::ptree pt;
    std::string allowed_ip_str;

    if (!fs::exists(cfg_file)) {
        GAMEAP_LOG_ERROR << "Config file not found: " << cfg_file.c_str();
        return -1;
    }

    try {
        boost::optional<std::string> buf;

        GAMEAP_LOG_INFO << "Reading cfg file " << cfg_file.c_str();
        boost::property_tree::ini_parser::read_ini(cfg_file.c_str(), pt);

        ds_id           = pt.get_optional<uint>("ds_id").get_value_or(0);;

        listen_ip     = pt.get_optional<std::string>("listen_ip").get_value_or("");
        listen_port     = pt.get_optional<uint>("listen_port").get_value_or(31717);

        api_host        = pt.get_optional<std::string>("api_host").get_value_or("");
        api_key         = pt.get_optional<std::string>("api_key").get_value_or("");

        daemon_login     = pt.get_optional<std::string>("daemon_login").get_value_or("");
        daemon_password  = pt.get_optional<std::string>("daemon_password").get_value_or("");

        password_authentication  = pt.get_optional<bool>("password_authentication").get_value_or(true);

        ca_certificate_file = pt.get<std::string>("ca_certificate_file");
        certificate_chain_file = pt.get<std::string>("certificate_chain_file");
        private_key_file = pt.get<std::string>("private_key_file");
        private_key_password = pt.get_optional<std::string>("private_key_password").get_value_or("");
        dh_file = pt.get<std::string>("dh_file");

#ifdef _WIN32
        path_7zip = pt.get_optional<std::string>("7zip_path").get_value_or("C:\\gameap\\tools\\7zip\\7za.exe");
        path_starter = pt.get_optional<std::string>("starter_path").get_value_or("C:\\gameap\\daemon\\gameap-starter.exe");

        if (path_7zip.empty()) {
            // TODO: Replace to "7za.exe" after installator fix
            path_7zip = "C:\\gameap\\tools\\7zip\\7za.exe";
        }

        if (path_starter.empty()) {
            // TODO: Replace to "gameap-starter.exe" after installator fix
            path_starter = "C:\\gameap\\daemon\\gameap-starter.exe";
        }
#endif
        std::string if_list_cfg = pt.get_optional<std::string>("if_list").get_value_or("");
        if (!if_list_cfg.empty()) {
            boost::split(if_list, if_list_cfg, boost::is_any_of(" "));
        }

        std::string drives_list_cfg = pt.get_optional<std::string>("drives_list").get_value_or("");
        if (!drives_list_cfg.empty()) {
            boost::split(drives_list, drives_list_cfg, boost::is_any_of(" "));
        }

        boost::optional<ushort> bufushort;

        bufushort = pt.get_optional<ushort>("stats_update_period");
        if (*bufushort > 0) stats_update_period = *bufushort;

        bufushort = pt.get_optional<ushort>("stats_db_update_period");
        if (*bufushort > 0) stats_db_update_period = *bufushort;

        log_level = pt.get_optional<std::string>("log_level").get_value_or("debug");

    }
    catch (std::exception &e) {
        GAMEAP_LOG_ERROR << "Parse config error: " << e.what();
        return -1;
    }

    return 0;
}
