#ifndef DL_H
#define DL_H

#ifdef WIN32
	#include <windows.h>

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

    inline const char* DLERROR(void) {
        // if (dlclose_handle_invalid)
            // return("Invalid handle.");
		static char buf[128];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 
				NULL, GetLastError(), 
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR) &buf, 128-1, NULL);

		return(buf);
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

#define C_DLLEXPORT	extern "C" DLLEXPORT

#endif
