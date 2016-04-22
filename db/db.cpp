#include "../dl.h"
#include "db.h"

#include <iostream>

int load_db(Db* &db, char * driver)
{
    char lib[16];

    #ifdef WIN32
        sprintf(lib, "db/%s.dll", driver);
    #else
        sprintf(lib, "./db/%s.so", driver);
    #endif

    DLHANDLE handle = DLOPEN(lib);

    if (!handle) {
        std::cerr << "Cannot load library: " << DLERROR() << '\n';
        return -1;
    }
    
    DLERROR();
    
    create_t* create_db = (create_t*) DLSYM(handle, "create");
    const char* dlsym_error = DLERROR();
    if (dlsym_error) {
        std::cerr << "Cannot load symbol create: " << dlsym_error << '\n';
        return -1;
    }

    destroy_t* destroy_db = (destroy_t*) DLSYM(handle, "destroy");
    dlsym_error = DLERROR();
    if (dlsym_error) {
        std::cerr << "Cannot load symbol destroy: " << dlsym_error << '\n';
        return -1;
    }

    db = create_db();

    return 0;
}
