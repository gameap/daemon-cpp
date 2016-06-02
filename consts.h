#ifdef _WIN32
    #define SLH "\\"
#else
    #define SLH "/"
#endif

#define DS_SCRIPT_START         1
#define DS_SCRIPT_STOP          2
#define DS_SCRIPT_RESTART       3
#define DS_SCRIPT_STATUS        4
#define DS_SCRIPT_GET_CONSOLE   5
#define DS_SCRIPT_SEND_CMD      6

// Daemon server 
#define DAEMON_SERVER_MODE_NOAUTH   0
#define DAEMON_SERVER_MODE_AUTH     1
#define DAEMON_SERVER_MODE_CMD      2
#define DAEMON_SERVER_MODE_FILES    3

#if defined __UINT32_MAX__ or UINT32_MAX
  #include <inttypes.h>
#else
	typedef unsigned char uint8;
	typedef unsigned short uint16;
	typedef unsigned long uint32;
	typedef unsigned long long uint64;
	
	typedef unsigned int uint;
#endif