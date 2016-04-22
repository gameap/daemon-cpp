#include "../dl.h"
#include "db.h"

#include <iostream>

int load_db(Db* &db)
{
    void* handle = DLOPEN("./db/mysql.so");

    if (!handle) {
        std::cerr << "Cannot load library: " << dlerror() << '\n';
        return 1;
    }
    
    dlerror();
    
    create_t* create_db = (create_t*) DLSYM(handle, "create");
    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        std::cerr << "Cannot load symbol create: " << dlsym_error << '\n';
        return 1;
    }

    destroy_t* destroy_db = (destroy_t*) DLSYM(handle, "destroy");
    dlsym_error = dlerror();
    if (dlsym_error) {
        std::cerr << "Cannot load symbol destroy: " << dlsym_error << '\n';
        return 1;
    }

    db = create_db();
}
