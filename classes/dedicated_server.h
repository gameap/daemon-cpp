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
    std::vector<ushort> cpu_load;
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

    #ifdef __linux__
        time_t last_cpustat_time = 0;
        std::map<ushort, ulong> last_cpustat[4];
    #endif
public:
    DedicatedServer();
    int ds_id;

    int stats_process();

    int get_cpu_load(float *cpu_percent);
    int get_net_load();

};

/* End namespace GameAP */
}

#endif
