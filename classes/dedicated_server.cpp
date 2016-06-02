#include "consts.h"

#include "config.h"
#include "db/db.h"
#include "dedicated_server.h"

#include <boost/format.hpp>

using namespace GameAP;

DedicatedServer::DedicatedServer()
{
    Config& config = Config::getInstance();

    ds_id = config.ds_id;

    stats_update_period = config.stats_update_period;
    db_update_period    = config.stats_db_update_period;

    std::cout << "stats_update_period: " << stats_update_period << std::endl;
    std::cout << "db_update_period: " << db_update_period << std::endl;
	
	#ifndef _WIN32
    // get cpu count
    cpu_count = std::thread::hardware_concurrency();

    struct sysinfo sysi;
    sysinfo(&sysi);

    ram_total = sysi.totalram;

    // Check interfaces
    for (std::vector<std::string>::iterator it = config.if_list.begin(); it != config.if_list.end(); ++it) {
        if (boost::filesystem::is_directory(str(boost::format("/sys/class/net/%s") % *it))) {
            interfaces.push_back(*it);
        }
    }
    
    boost::filesystem::space_info spi;

    // Check filesystem
    for (std::vector<std::string>::iterator it = config.drives_list.begin(); it != config.drives_list.end(); ++it) {
        try {
            spi = boost::filesystem::space(*it);
            drv_space[*it] = spi.capacity;
            std::cout << "Space capacity [" << *it << "]: " << drv_space[*it] << std::endl;
            drives.push_back(*it);
         } catch (boost::filesystem::filesystem_error &e) {
            std::cout << "Error get space: " << e.what() << std::endl;
            return;
        }
    }

    std::vector<float> cpu_percent;
    get_cpu_load(cpu_percent);

    std::map<std::string, ds_iftstats> ifstats;
    get_net_load(ifstats);

    last_stats_update = time(0);
	#endif

    std::string qstr = str(boost::format("SELECT `script_start`, `script_stop`, `script_restart`, `script_status`, `script_get_console`, `script_send_command`\
            FROM `{pref}dedicated_servers`\
            WHERE `id` = '%1%'") % ds_id);

    db_elems results;
    if (db->query(&qstr[0], &results) == -1) {
        fprintf(stdout, "Error query\n");
        return;
    }

    script_start        = results.rows[0].row["script_start"];
    script_stop         = results.rows[0].row["script_stop"];
    script_restart      = results.rows[0].row["script_restart"];
    script_status       = results.rows[0].row["script_status"];
    script_get_console  = results.rows[0].row["script_get_console"];
    script_send_command = results.rows[0].row["script_send_command"];
            
}

// ---------------------------------------------------------------------

int DedicatedServer::stats_process()
{
    #ifndef _WIN32
	if (time(0) - last_stats_update < stats_update_period) {
        return -1;
    }

    ds_stats cur_stats;
    stats.reserve(1);
    cur_stats.cpu_load.reserve(cpu_count);

    cur_stats.time = time(0);
    getloadavg(cur_stats.loa, 3);

    struct sysinfo sysi;
    sysinfo(&sysi);
    
    // Get cpu load
    get_cpu_load(cur_stats.cpu_load);
    
    // Get ram load
    cur_stats.ram_us = sysi.totalram - sysi.freeram;
    
    // Get drive space
    boost::filesystem::space_info spi;
    for (std::vector<std::string>::iterator it = drives.begin(); it != drives.end(); ++it) {
        spi = boost::filesystem::space(*it);
        cur_stats.drv_us_space[*it] = spi.capacity-spi.free;
        cur_stats.drv_free_space[*it] = spi.free;
    }

    // Get if stat
    get_net_load(cur_stats.ifstats);
    
    
    stats.insert(stats.end(), cur_stats);

    // std::cout
        // << "cur_stats.loa: " << cur_stats.loa[0] << " " << cur_stats.loa[1] << " " << cur_stats.loa[2] << std::endl
        // << "cur_stats.cpu_load: " << cur_stats.cpu_load << std::endl
        // << "cur_stats.ram_us: " << cur_stats.ram_us << std::endl
        // << "cur_stats.drv_us_space[\"/\"]: " << cur_stats.drv_us_space["/"] << std::endl
        // << "cur_stats.drv_free_space[\"/\"]: " << cur_stats.drv_free_space["/"] << std::endl
        // << "cur_stats.ifstats[lo].rxb: " << cur_stats.ifstats["lo"].rxb << std::endl
        // << "cur_stats.ifstats[lo].txb: " << cur_stats.ifstats["lo"].txb << std::endl
        // << "cur_stats.ifstats[lo].rxp: " << cur_stats.ifstats["lo"].txp << std::endl
        // << "cur_stats.ifstats[lo].txp: " << cur_stats.ifstats["lo"].txp << std::endl
    // << std::endl;

    last_stats_update = time(0);
	#endif
    return 0;
}

// ---------------------------------------------------------------------

int DedicatedServer::get_net_load(std::map<std::string, ds_iftstats> &ifstats)
{
    #ifndef _WIN32
	// Get current tx rx
    char bufread[32];

    std::map<std::string, ds_iftstats> current_ifstats;

    for (std::vector<std::string>::iterator it = interfaces.begin(); it != interfaces.end(); ++it) {
        std::ifstream netstats;
        netstats.open(str(boost::format("/sys/class/net/%s/statistics/rx_bytes") % *it), std::ios::in);
        netstats.getline(bufread, 32);
        current_ifstats[*it].rxb = atoi(bufread);
        netstats.close();
        
        netstats.open(str(boost::format("/sys/class/net/%s/statistics/tx_bytes") % *it), std::ios::in);
        netstats.getline(bufread, 32);
        current_ifstats[*it].txb = atoi(bufread);
        netstats.close();
        
        netstats.open(str(boost::format("/sys/class/net/%s/statistics/rx_packets") % *it), std::ios::in);
        netstats.getline(bufread, 32);
        current_ifstats[*it].rxp = atoi(bufread);
        netstats.close();
        
        netstats.open(str(boost::format("/sys/class/net/%s/statistics/tx_packets") % *it), std::ios::in);
        netstats.getline(bufread, 32);
        current_ifstats[*it].txp = atoi(bufread);
        netstats.close();
    }

    time_t current_time = time(0);

    if (last_ifstat_time != 0 && current_time > last_ifstat_time) {
        
        int time_diff = current_time - last_ifstat_time;
        
        for (std::vector<std::string>::iterator it = interfaces.begin(); it != interfaces.end(); ++it) {
            if (current_ifstats.count(*it) > 0  && last_ifstats.count(*it) > 0) {
                
                ifstats[*it].rxb = (current_ifstats[*it].rxb - last_ifstats[*it].rxb) / time_diff;
                ifstats[*it].txb = (current_ifstats[*it].txb - last_ifstats[*it].txb) / time_diff;
                ifstats[*it].rxp = (current_ifstats[*it].rxp - last_ifstats[*it].rxp) / time_diff;
                ifstats[*it].txp = (current_ifstats[*it].txp - last_ifstats[*it].txp) / time_diff;

                // std::cout << "ifstats[*it].rxb: " << ifstats[*it].rxb << std::endl;
                // std::cout << "ifstats[*it].txb: " << ifstats[*it].txb << std::endl;
                // std::cout << "ifstats[*it].rxp: " << ifstats[*it].rxp << std::endl;
                // std::cout << "ifstats[*it].txp: " << ifstats[*it].txp << std::endl;
            }
        }
    }

    last_ifstats        = current_ifstats;
    last_ifstat_time    = time(0);
	#endif
	
    return 0;
}

// ---------------------------------------------------------------------

int DedicatedServer::get_cpu_load(std::vector<float> &cpu_percent)
{
    #ifndef _WIN32
	std::ifstream cpustat;

    std::map<ushort, ulong> oldcpuload[4];
    std::map<ushort, ulong> cpuload[4];

    for (int i = 0; i < 4; i++) {
        oldcpuload[i] = last_cpustat[i];
        last_cpustat[i].clear();
    }
    
    ushort readsize = 128 * cpu_count;
    char buf[10240];

    time_t cpustat_time = time(0);
    cpustat.open("/proc/stat", std::ios::in);

    cpustat.read(buf, (std::streamsize)readsize);
    
    std::vector<std::string> split_lines;
    split_lines.reserve(cpu_count+8);

    std::vector<std::string> split_spaces;
    split_spaces.reserve(11);
    
    boost::split(split_lines, buf, boost::is_any_of("\n\r"));

    ushort cpuid = 0;
    for (std::vector<std::string>::iterator itl = split_lines.begin()+1; itl != split_lines.end(); ++itl) {
        if (cpuid > cpu_count) break;
        // Line must begin only "cpu  ..."
        if (itl[0][0] != 'c') break;
        
        // std::cout << *itl << std::endl;
        
        boost::split(split_spaces, *itl, boost::is_any_of("  "));
        cpuload[0][cpuid] = atoi(&split_spaces[1][0]); // User
        cpuload[1][cpuid] = atoi(&split_spaces[2][0]); // Nice
        cpuload[2][cpuid] = atoi(&split_spaces[3][0]); // System
        cpuload[3][cpuid] = atoi(&split_spaces[4][0]); // Idle

        cpuid++;
        split_spaces.clear();
    }

    std::map<ushort, ulong> loaddiff[4];
    std::vector<float> perc;
    perc.reserve(cpu_count);

    if (last_cpustat_time != 0) {

        float time_between = cpustat_time - last_cpustat_time;

        for (int i = 0; i < cpu_count; i++) {
            loaddiff[0][i] = cpuload[0][i] - oldcpuload[0][i]; // User
            loaddiff[1][i] = cpuload[1][i] - oldcpuload[1][i]; // Nice
            loaddiff[2][i] = cpuload[2][i] - oldcpuload[2][i]; // System
            loaddiff[3][i] = cpuload[3][i] - oldcpuload[3][i]; // Idle

            last_cpustat[0][i] = cpuload[0][i];
            last_cpustat[1][i] = cpuload[1][i];
            last_cpustat[2][i] = cpuload[2][i];
            last_cpustat[3][i] = cpuload[3][i];

            // std::cout << "cpuload[0][i]" << cpuload[0][i] << std::endl;
            // std::cout << "cpuload[1][i]" << cpuload[1][i] << std::endl;
            // std::cout << "cpuload[2][i]" << cpuload[2][i] << std::endl;
            // std::cout << "cpuload[3][i]" << cpuload[3][i] << std::endl;
            
            // std::cout << "oldcpuload[0][i]" << oldcpuload[0][i] << std::endl;
            // std::cout << "oldcpuload[1][i]" << oldcpuload[1][i] << std::endl;
            // std::cout << "oldcpuload[2][i]" << oldcpuload[2][i] << std::endl;
            // std::cout << "oldcpuload[3][i]" << oldcpuload[3][i] << std::endl;
            
            // std::cout << "loaddiff[0][i]" << loaddiff[0][i] << std::endl;
            // std::cout << "loaddiff[1][i]" << loaddiff[1][i] << std::endl;
            // std::cout << "loaddiff[2][i]" << loaddiff[2][i] << std::endl;
            // std::cout << "loaddiff[3][i]" << loaddiff[3][i] << std::endl;

            if ((loaddiff[0][i] != 0 || loaddiff[1][i] != 0 || loaddiff[2][i] != 0 || loaddiff[3][i] != 0) && time_between >= 1) {
                try {
                    perc[i] = (
                        ((loaddiff[0][i] + loaddiff[1][i] + loaddiff[2][i]) * (time_between/100)) /
                        ((loaddiff[0][i] + loaddiff[1][i] + loaddiff[2][i] + loaddiff[3][i]) * (time_between/100))
                    ) * 100;
                }
                catch (std::logic_error &e) {
                    std::cout << "CPU Load calculation error: " << e.what() << std::endl;
                }
                
            } else {
                perc[i] = 0;
            }

            cpu_percent.push_back(perc[i]);
            // std::cout << "CPU #" << i << ": " << perc[i] << std::endl;
        }
        
        // cpu_percent = &perc;
        // std::cout << "perc[0]: " << perc[0] << std::endl;
        // std::cout << "cpu_percent[0]: " << (*cpu_percent)[0] << std::endl;

        last_cpustat_time = cpustat_time;
        return 0;
    } else {
        for (int i = 0; i < cpu_count; i++) {
            last_cpustat[0][i] = cpuload[0][i];
            last_cpustat[1][i] = cpuload[1][i];
            last_cpustat[2][i] = cpuload[2][i];
            last_cpustat[3][i] = cpuload[3][i];
        }
        
        last_cpustat_time = cpustat_time;
        return -1;
    }
	#endif

	return 0;
}

int DedicatedServer::update_db()
{
    if (time(0) - last_db_update < db_update_period) {
        return -1;
    }

    std::vector<std::vector<ds_stats>::iterator>insert_complete;

    for (std::vector<ds_stats>::iterator it = stats.begin();
        it != stats.end();
        ++it
    ) {
        ushort ping = 0;

        // Load average
        std::string loa = str(boost::format("%.2f %.2f %.2f") % (*it).loa[0] % (*it).loa[1] % (*it).loa[2]);

        // Ram
        std::string ram = str(boost::format("%1% %2%") % (*it).ram_us % ram_total);

        // Cpu
        std::string cpu = "";
        for (int i = 0; i < (*it).cpu_load.size(); ++i) {
            cpu +=  str(boost::format("%.2f") % (*it).cpu_load[i]) + " ";
        }

        // If stat
        std::string ifstat = "";
        for (std::map<std::string, ds_iftstats>::iterator itd = (*it).ifstats.begin();
            itd != (*it).ifstats.end();
            ++itd
        ) {
            ifstat +=  str(boost::format("%1% %2% %3% %4% %5%")
                % (*itd).first
                % (*itd).second.rxb
                % (*itd).second.txb
                % (*itd).second.rxp
                % (*itd).second.txp
            );
            
            ifstat += "\n";
        }

        // Drive space
        std::string drvspace = "";
        for (std::map<std::string, ulong>::iterator itd = (*it).drv_us_space.begin();
            itd != (*it).drv_us_space.end();
            ++itd
        ) {
            drvspace +=  str(boost::format("%1% %2% %3%") % (*itd).first % (*itd).second % drv_space[(*itd).first]) + "\n";
        }

        
        std::string qstr = str(
            boost::format(
                "INSERT INTO `{pref}ds_stats` (`ds_id`, `time`, `loa`, `ram`, `cpu`, `ifstat`, `ping`, `drvspace`)\
                VALUES ('%1%', %2%, '%3%', '%4%', '%5%', '%6%', %7%, '%8%')"
            ) % ds_id % (*it).time % loa % ram % cpu % ifstat % ping % drvspace
        );
        
        if (db->query(&qstr[0]) == 0) {
            insert_complete.push_back(it);
        } else {
            break;
        }
    }

    // Clear completed
    for (int i = 0; i < insert_complete.size(); ++i) {
        stats.erase(insert_complete[i]);
    }

    if (insert_complete.size() > 0) {
        insert_complete.clear();
        last_db_update = time(0);
    }
}

std::string DedicatedServer::get_script_cmd(ushort script)
{
    switch (script) {
        case DS_SCRIPT_START:
            return script_start;
            break;
            
        case DS_SCRIPT_STOP:
            return script_stop;
            break;
            
        case DS_SCRIPT_RESTART:
            return script_restart;
            break;
            
        case DS_SCRIPT_STATUS:
            return script_status;
            break;
            
        case DS_SCRIPT_GET_CONSOLE:
            return script_get_console;
            break;
            
        case DS_SCRIPT_SEND_CMD:
            return script_send_command;
            break;
    }
}
