#ifndef DEDICATED_SERVER_H
#define DEDICATED_SERVER_H

#include <iostream>
#include <stdlib.h>

#include <vector>
#include <map>

#include "sys/types.h"
#include "sys/sysinfo.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <string.h>
#include <net/if.h>
#include <errno.h>
#include <stdio.h>

#include <unistd.h>
#include <thread>

#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

namespace GameAP {

// struct ds_res {
    // int cpu_count;
    // std::vector<uint> drv_space;
// }

struct ds_iftstats {
    ulong rxb;
    ulong txb;
    
    ulong rxp;
    ulong txp;
};

struct ds_stats {
    time_t time;

    double loa[3];
    std::vector<float> cpu_load;
    ulong ram_us;
    
    std::map<std::string, ds_iftstats> ifstats;
    std::map<std::string, ulong> drv_us_space;
    std::map<std::string, ulong> drv_free_space;
};

class DedicatedServer {
private:
    ushort cpu_count;
    ulong ram_total;
    std::map<std::string, ulong> drv_space;

    std::vector<ds_stats> stats;
    std::string ds_ip;
    
    time_t last_stats_update        = 0;
    time_t last_db_update           = 0;

    ushort stats_update_period =    300;
    ushort db_update_period =       300;

    #ifdef __linux__
        time_t last_cpustat_time = 0;
        std::map<ushort, ulong> last_cpustat[4];

        time_t last_ifstat_time = 0;
        std::map<std::string, ds_iftstats> last_ifstats;
    #endif

    std::vector<std::string> interfaces;
    std::vector<std::string> drives;

    ulong ds_id = 1;
public:
    DedicatedServer();

    int stats_process();
    int update_db();

    int get_net_load(std::map<std::string, ds_iftstats> &ifstat);
    int get_cpu_load(std::vector<float> &cpu_percent);
};

/* End namespace GameAP */
}

#endif
