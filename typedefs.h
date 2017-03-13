#ifndef _TYPEDEFS_H
#define _TYPEDEFS_H

#ifdef _WIN32
    typedef unsigned int uint;
    typedef unsigned short ushort;
	typedef unsigned long ulong;
#endif

#endif

struct netstats {
    uintmax_t rxb;
    uintmax_t txb;

    uintmax_t rxp;
    uintmax_t txp;
};