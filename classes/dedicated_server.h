#ifndef DEDICATED_SERVER_H
#define DEDICATED_SERVER_H

#include <iostream>
#include <stdlib.h>

#include <vector>
#include <map>

#include "sys/types.h"

#ifdef __linux__
	#include "sys/sysinfo.h"

	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <sys/ioctl.h>
	#include <sys/time.h>

	#include <net/if.h>
#endif

#include <string.h>
#include <errno.h>
#include <stdio.h>

#ifdef __GNUC__
#include <unistd.h>
#include <thread>
#endif

#ifdef _WIN32
    #define OS "windows"
#elif __linux__
    #define OS "linux"
#else
    #define OS "unknown"
#endif

#include <fstream>

#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "typedefs.h"

#define DS_PREFER_INSTALL_METHOD_AUTO         0
#define DS_PREFER_INSTALL_METHOD_COPY         1
#define DS_PREFER_INSTALL_METHOD_DOWNLOAD     2
#define DS_PREFER_INSTALL_METHOD_SCRIPT       3
#define DS_PREFER_INSTALL_METHOD_STEAM        4
#define DS_PREFER_INSTALL_METHOD_NONE         5

#define DS_SCRIPT_INSTALL		1
#define DS_SCRIPT_REINSTALL		2
#define DS_SCRIPT_UPDATE		3
#define DS_SCRIPT_START         4
#define DS_SCRIPT_PAUSE			5
#define DS_SCRIPT_UNPAUSE		6
#define DS_SCRIPT_STOP          7
#define DS_SCRIPT_KILL          8
#define DS_SCRIPT_RESTART       9
#define DS_SCRIPT_STATUS        10
#define DS_SCRIPT_GET_CONSOLE   11
#define DS_SCRIPT_SEND_CMD      12

// Daemon server
#define DAEMON_SERVER_MODE_NOAUTH   0
#define DAEMON_SERVER_MODE_AUTH     1
#define DAEMON_SERVER_MODE_CMD      2
#define DAEMON_SERVER_MODE_FILES    3


namespace GameAP {

// struct ds_res {
    // int cpu_count;
    // std::vector<uint> drv_space;
// }

struct ds_stats {
    time_t time;

    double loa[3];
    std::vector<float> cpu_load;
    uintmax_t ram_us;
    uintmax_t ram_cache;

    std::map<std::string, netstats> ifstats;
    std::map<std::string, uintmax_t> drv_us_space;
    std::map<std::string, uintmax_t> drv_free_space;
};

class DedicatedServer {
public:
	static DedicatedServer& getInstance() {
		static DedicatedServer instance;
		return instance;
	}

	int stats_process();
	int update_db();

	int get_net_load(std::map<std::string, netstats> &ifstat);
	int get_cpu_load(std::vector<float> &cpu_percent);

	std::string get_work_path();

	int get_ping(ushort &ping);

	std::string get_script_cmd(ushort script);
private:
    ushort cpu_count;
    uintmax_t ram_total;
    std::map<std::string, uintmax_t> drv_space;

    std::vector<ds_stats> stats;
    std::string ds_ip;

    time_t last_stats_update;
    time_t last_db_update;

    int db_timediff;

    ushort stats_update_period;
    ushort db_update_period;

    time_t last_cpustat_time;
    std::map<ushort, ulong> last_cpustat[4];

    time_t last_ifstat_time;
    std::map<std::string, netstats> last_ifstats;

    std::vector<std::string> interfaces;
    std::vector<std::string> drives;

    ulong ds_id;

	std::string work_path;

    ushort prefer_installation_method;
    std::string script_install;
    std::string script_reinstall;
    std::string script_update;
    std::string script_start;
    std::string script_pause;
    std::string script_unpause;
    std::string script_stop;
    std::string script_kill;
    std::string script_restart;
    std::string script_status;
    std::string script_get_console;
    std::string script_send_command;
    std::string script_delete;

    DedicatedServer();
    DedicatedServer( const DedicatedServer&);
    DedicatedServer& operator=( DedicatedServer& );
};

/* End namespace GameAP */
}

#endif
