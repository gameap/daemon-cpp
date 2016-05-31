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
