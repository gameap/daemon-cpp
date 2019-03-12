#include <cstring>

#ifdef __linux__
    #include <pwd.h>
    #include <shadow.h>
    #include <crypt.h>
#endif

#include "config.h"

namespace gameap {
namespace auth {

#ifdef _WIN32
        bool check(const char *username, const char *password)
        {
            Config& config = Config::getInstance();

            if (!config.password_authentication) {
                return true;
            }

            if (config.daemon_login.length() > 0 && config.daemon_password.length() > 0) {
                if (username == config.daemon_login && password == config.daemon_password) {
                    return true;
                }
            }

            return false;
        }
#else

        bool check(const char *username, const char *password) {
            Config& config = Config::getInstance();

            if (!config.password_authentication) {
                return true;
            }

            if (config.daemon_login.length() > 0 && config.daemon_password.length() > 0) {
                // Override pam login
                if (username == config.daemon_login && password == config.daemon_password) {
                    return true;
                }
            }

            passwd *userinfo;
            struct spwd *passw;

            userinfo = getpwnam(username);

            if (userinfo == nullptr) {
                return false;
            }

            passw = getspnam(userinfo->pw_name);

            if (passw == nullptr) {
                return false;
            }

            if (strcmp(passw->sp_pwdp, crypt(password, passw->sp_pwdp)) == 0) {
                return true;
            }
        }
#endif

}/* namespace auth */
} /* namespace gameap */