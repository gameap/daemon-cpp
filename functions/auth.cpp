#include <cstring>
#include <pwd.h>
#include <shadow.h>
#include <crypt.h>

#include "config.h"

namespace gameap {
namespace auth {

#ifdef _WIN32
        bool check(const char *username, const char *password)
        {
            if (config.daemon_login.length() > 0 && config.daemon_password.length() > 0) {
                if (username == config.daemon_login && password == config.daemon_password) {
                    return true;
                }
            }
        }
#else

        bool check(const char *username, const char *password) {
            Config& config = Config::getInstance();

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

}/* namespace auth */ } /* namespace gameap */

#endif