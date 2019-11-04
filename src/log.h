#ifndef GDAEMON_LOG_H
#define GDAEMON_LOG_H

#include <plog/Log.h>

namespace GameAP {
    enum {
        DefaultLog = 0,
        ErrorLog = 1
    };
}

#define GAMEAP_LOG_VERBOSE                     LOG_VERBOSE_(GameAP::DefaultLog)
#define GAMEAP_LOG_DEBUG                       LOG_DEBUG_(GameAP::DefaultLog)
#define GAMEAP_LOG_INFO                        LOG_INFO_(GameAP::DefaultLog)
#define GAMEAP_LOG_WARNING                     LOG_WARNING_(GameAP::DefaultLog)
#define GAMEAP_LOG_ERROR                       LOG_ERROR_(GameAP::ErrorLog)
#define GAMEAP_LOG_FATAL                       LOG_FATAL_(GameAP::ErrorLog)
#define GAMEAP_LOG_NONE                        LOG_NONE

#endif
