#ifndef CONFIG_H
#define CONFIG_H

class Config
{
    public:
        std::string db_driver;
        std::string db_host;
        std::string db_user;
        std::string db_passwd;
        std::string db_name;
        int db_port;

        std::string crypt_key;
        std::string allowed_ip_str;
        int port;
        
        int parse();
};

#endif
