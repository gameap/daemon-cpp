cmake_minimum_required(VERSION 2.8)
project(GDaemon)

#if (WIN32)
#    SET(CMAKE_SYSTEM_NAME      Windows)
#    # SET(CMAKE_SYSTEM_PROCESSOR i686)
#
#    SET(CMAKE_C_COMPILER       gcc)
#    SET(CMAKE_CXX_COMPILER     g++)
#
#    # SET(CMAKE_C_COMPILER       /usr/bin/i686-w64-mingw32-gcc-win32 )
#    # SET(CMAKE_CXX_COMPILER     /usr/bin/i686-w64-mingw32-g++-win32 )
#endif()
#
#if (UNIX)
#    SET(CMAKE_C_COMPILER       gcc)
#    SET(CMAKE_CXX_COMPILER     g++)
#endif()

set (CMAKE_CXX_STANDARD 11)
add_definitions(-std=c++11)

add_subdirectory(src)
#add_subdirectory(tests)