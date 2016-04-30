#include "dedicated_server.h"

using namespace GameAP;

//

DedicatedServer::DedicatedServer()
{
    // get cpu count
    cpu_count = std::thread::hardware_concurrency();

    struct sysinfo sysi;
    sysinfo(&sysi);

    ram_total = sysi.totalram;

    // std::cout
        // << "totalram: " << sysi.totalram << std::endl
        // << "freeram: " << sysi.freeram << std::endl
        // << "bufferram: " << sysi.bufferram << std::endl
    // << std::endl;

    boost::filesystem::space_info spi;
    
    try {
        spi = boost::filesystem::space("/");
        drv_space["/"] = spi.capacity;
    }
    catch (boost::filesystem::filesystem_error &e) {
        std::cout << "Error get space: " << e.what() << std::endl;
        return;
    }

    std::cout
        << "Space capacity: " << drv_space["/"]
        << std::endl;
}

// ---------------------------------------------------------------------

int DedicatedServer::stats_process()
{
    ds_stats cur_stats;
    stats.reserve(1);

    cur_stats.time = time(0);
    getloadavg(cur_stats.loa, 3);

    struct sysinfo sysi;
    sysinfo(&sysi);
    
    // Get cpu load

    // Get ram load
    cur_stats.ram_us = sysi.totalram - sysi.freeram;
    
    // Get drive space
    boost::filesystem::space_info spi;
    spi = boost::filesystem::space("/");
    
    cur_stats.drv_us_space["/"] = spi.capacity-spi.free;
    cur_stats.drv_free_space["/"] = spi.free;
    
    // Get current tx rx
    char bufread[32];
    
    std::ifstream netstats;
    netstats.open("/sys/class/net/lo/statistics/rx_bytes", std::ios::in);
    // netstats.read(bufread, (std::streamsize)32);
    netstats.getline(bufread, 32);
    cur_stats.ifstats["lo"].rxb = atoi(bufread);
    netstats.close();
    
    netstats.open("/sys/class/net/lo/statistics/tx_bytes", std::ios::in);
    // netstats.read(bufread, (std::streamsize)32);
    netstats.getline(bufread, 32);
    cur_stats.ifstats["lo"].txb = atoi(bufread);
    netstats.close();
    
    netstats.open("/sys/class/net/lo/statistics/rx_packets", std::ios::in);
    // netstats.read(bufread, (std::streamsize)32);
    netstats.getline(bufread, 32);
    cur_stats.ifstats["lo"].rxp = atoi(bufread);
    netstats.close();
    
    netstats.open("/sys/class/net/lo/statistics/tx_packets", std::ios::in);
    // netstats.read(bufread, (std::streamsize)32);
    netstats.getline(bufread, 32);
    cur_stats.ifstats["lo"].txp = atoi(bufread);
    netstats.close();
    
    stats.insert(stats.end(), cur_stats);

    float cpu_load;
    get_cpu_load(&cpu_load);

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
}

// ---------------------------------------------------------------------

int DedicatedServer::get_cpu_load(float *cpu_percent)
{
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

            // std::cout << "CPU #" << i << ": " << perc[i] << std::endl;
        }

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
}

// ---------------------------------------------------------------------

int DedicatedServer::get_net_load()
{
    int sock;                                                          // дескриптор сокета
    struct sockaddr_in *in_addr;                             // структура интернет адреса (поля)
    struct ifreq ifdata;                                            // структура - параметр
    struct if_nameindex*     ifNameIndex;                // структура интерфейсов и их индексов
    sock = socket(AF_INET, SOCK_DGRAM, 0);     // открываем дескриптор сокета
    
    if (sock < 0) {
        printf("Не удалось открыть сокет, ошибка: %s\n", strerror(errno));
        return 1;
    }

    if (sock < 0) {
        printf("Не удалось открыть сокет, ошибка: %s\n", strerror(errno));
        return 1;
    }
    
    ifNameIndex = if_nameindex();
    
    if (ifNameIndex) {                                                  // если удалось получить данные
        while (ifNameIndex->if_index) {                                 // пока имеются данные
            memset(&ifdata, 0, sizeof(ifdata));                             // очищаем структуру
            strncpy(ifdata.ifr_name, ifNameIndex->if_name, IFNAMSIZ);       // получаем имя следующего интерфейса
            
            // получаем IP адрес с помощью SIOCGIFADDR, одновременно проверяя результат
            if (ioctl(sock, SIOCGIFADDR, &ifdata) < 0) {
                // printf("Не получить IP адрес для %s, ошибка: %s\n", ifdata.ifr_name, strerror(errno));
                // close(sock);
                // return 1;
            }
            // преобразовываем из массива байт в структуру sockaddr_in
            in_addr = (struct sockaddr_in *) &ifdata.ifr_addr;
            // printf("Интерфейс %s индекес %i IP адрес: %s\n", ifdata.ifr_name, ifNameIndex->if_index, inet_ntoa(in_addr->sin_addr));
            ++ifNameIndex;                                   // переходим к следующему интерфейсу
        }
    }
    
    close(sock);
    return 0;
}
