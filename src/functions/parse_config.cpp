#include "log.h"

/**
* Парсер конфигурации
*/
int parse_config()
{
	boost::property_tree::ptree pt;
	std::string allowed_ip_str;

	try {
		boost::property_tree::ini_parser::read_ini("daemon.cfg", pt);
        
		crypt_key = pt.get<std::string>("crypt_key");
		allowed_ip_str = pt.get<std::string>("allowed_ip");
		port = pt.get<int>("server_port");
	}
	catch (std::exception &e) {
		GAMEAP_LOG_ERROR << "Parse config error: " << e.what();
        return -1;
	}

	if (allowed_ip_str != "") {
		allowed_ip = explode(",", allowed_ip_str);

		int size = allowed_ip.size();
		for (int i = 0; i < size; i++) {
			allowed_ip[i] = trim(allowed_ip[i]);
		}
	}
}
