#ifndef _TYPEDEFS_H
#define _TYPEDEFS_H

#ifdef _WIN32
    typedef unsigned int uint;
    typedef unsigned short ushort;
	typedef unsigned long ulong;
#endif

struct netstats {
    __uintmax_t rxb;
    __uintmax_t txb;

    __uintmax_t rxp;
    __uintmax_t txp;
};

#endif