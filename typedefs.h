#ifndef _TYPEDEFS_H
#define _TYPEDEFS_H

#include <cstdint>

#ifdef _WIN32
    typedef unsigned int uint;
    typedef unsigned short ushort;
	typedef unsigned long ulong;
#endif

struct netstats {
    uint64_t rxb;
    uint64_t txb;

    uint64_t rxp;
    uint64_t txp;
};

#endif