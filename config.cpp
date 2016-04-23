#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include "config.h"

int Config::parse()
{
    boost::property_tree::ptree pt;
    std::string allowed_ip_str;

    try {
        boost::property_tree::ini_parser::read_ini("daemon.cfg", pt);
        
        db_driver = pt.get<std::string>("db_driver");
        db_host = pt.get<std::string>("db_host");
        db_user = pt.get<std::string>("db_user");
        db_passwd = pt.get<std::string>("db_passwd");
        db_name = pt.get<std::string>("db_name");
        db_port = pt.get<int>("db_port");

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
