#ifndef DL_H
#define DL_H

#ifdef WIN32
    typedef HINSTANCE DLHANDLE;
    typedef FARPROC DLFUNC;
    inline DLHANDLE DLOPEN(const char *filename) {
        return(LoadLibrary(filename));
    }
    inline DLFUNC DLSYM(DLHANDLE handle, const char *string) {
        return(GetProcAddress(handle, string));
    }
    inline int DLCLOSE(DLHANDLE handle) {
        if (!handle) {
            return(1);
        }
        return(!FreeLibrary(handle));
    }
    const char *str_GetLastError(void);
    inline const char* DLERROR(void) {
        // if (dlclose_handle_invalid)
            // return("Invalid handle.");
        return(str_GetLastError());
    }
#else
    #include <dlfcn.h>
    
    typedef void* DLHANDLE;
    typedef void* DLFUNC;

    inline DLHANDLE DLOPEN(const char *filename) {
        return(dlopen(filename, RTLD_NOW));
    }
    inline DLFUNC DLSYM(DLHANDLE handle, const char *string) {
        return(dlsym(handle, string));
    }
    inline int DLCLOSE(DLHANDLE handle) {
        if (!handle) {
            return(1);
        }
        return(dlclose(handle));
    }
    inline const char* DLERROR(void) {
        // if (dlclose_handle_invalid)
            // return("Invalid handle.");
        return(dlerror());
    }
#endif

#if defined(WIN32)
    //  Microsoft 
    #define DLLEXPORT __declspec(dllexport)
    #define DLLIMPORT __declspec(dllimport)
#else
    //  GCC
    #define DLLEXPORT __attribute__((visibility("default")))
    #define DLLIMPORT
#endif

#endif
