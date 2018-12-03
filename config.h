#ifndef CONFIG_H
#define CONFIG_H

#include "typedefs.h"
#include <vector>
#include <string>

class Config
{
private:
    Config() {}
    Config( const Config&);
    Config& operator=( Config& );
public:
    static Config& getInstance() {
        static Config instance;
        return instance;
    }

    std::string cfg_file;
    std::string output_log;
    std::string error_log;

    uint ds_id;
    uint listen_port;

    std::string api_host;
    std::string api_key;

    std::string daemon_login;
    std::string daemon_password;
    bool password_authentication;

    std::string client_certificate_file;
    std::string certificate_chain_file;
    std::string private_key_file;
    std::string private_key_password;
    std::string dh_file;

    std::vector<std::string> if_list;
    std::vector<std::string> drives_list;

    ushort stats_update_period;
    ushort stats_db_update_period;

    int parse();
};

#endif
