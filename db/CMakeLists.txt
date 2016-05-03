# SET(CMAKE_SYSTEM_NAME      Windows)
# SET(CMAKE_SYSTEM_PROCESSOR i686)

# SET(CMAKE_C_COMPILER       /usr/bin/i686-w64-mingw32-gcc-win32 )
# SET(CMAKE_CXX_COMPILER     /usr/bin/i686-w64-mingw32-g++-win32 )

SET(CMAKE_C_COMPILER       gcc-4.9 )
SET(CMAKE_CXX_COMPILER     g++-4.9 )

cmake_minimum_required(VERSION 2.8)

add_definitions(-std=c++11)
# LINK_LIBRARIES(ws2_32 mswsock netapi32)
# add_definitions(-ldl)
# target_link_libraries(main dl)

if (LINUX)
    set(CMAKE_CXX_FLAGS ${CMAKE_CSS_FLAGS} "-lwsock32")
endif()

# set(Boost_USE_STATIC_LIBS ON)

# CMAKE_IMPORT_LIBRARY_PREFIX

add_library(mysql SHARED mysql.cpp)
target_link_libraries (mysql mysqlcppconn)
set_target_properties(mysql PROPERTIES PREFIX "")
