#ifdef _WIN32
    #define SLH "\\"
#else
    #define SLH "/"
#endif

#if defined __UINT32_MAX__ || defined UINT32_MAX
#include <inttypes.h>
#else
	typedef unsigned char uint8;
	typedef unsigned short uint16;
	typedef unsigned long uint32;
	typedef unsigned long long uint64;

	typedef unsigned int uint;
#endif

#define SUCCESS_STATUS_INT 			0
#define ERROR_STATUS_INT	       -1

#define GAMEAP_DAEMON_VERSION "2.1.0"

// State keys

#define STATE_PANEL_TIMEDIFF "gameap:timediff"
