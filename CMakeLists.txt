cmake_minimum_required(VERSION 2.8)
project(GDaemon)

if (WIN32)
    SET(CMAKE_SYSTEM_NAME      Windows)
    # SET(CMAKE_SYSTEM_PROCESSOR i686)

    SET(CMAKE_C_COMPILER       gcc)
    SET(CMAKE_CXX_COMPILER     g++)

    # SET(CMAKE_C_COMPILER       /usr/bin/i686-w64-mingw32-gcc-win32 )
    # SET(CMAKE_CXX_COMPILER     /usr/bin/i686-w64-mingw32-g++-win32 )
endif()

if (UNIX)
    SET(CMAKE_C_COMPILER       gcc)
    SET(CMAKE_CXX_COMPILER     g++)
endif()

cmake_minimum_required(VERSION 2.8)

set (CMAKE_CXX_STANDARD 11)
add_definitions(-std=c++11)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/build/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/build/lib)
set(LIBRARY_OUTPUT_PATH ${CMAKE_HOME_DIRECTORY}/build/lib)

if( "${GDAEMON_LIBPATH}" STREQUAL "" )
    set(GDAEMON_LIBPATH /usr/lib/gameap-daemon)
endif()

if (WIN32)
	set(Boost_USE_STATIC_LIBS ON)
endif()

#if (UNIX)
#	set(Boost_USE_STATIC_LIBS ON)
#endif()

find_package(Boost REQUIRED COMPONENTS system filesystem thread)
if(NOT Boost_FOUND)
	message(SEND_ERROR "Failed to find boost components")
	return()
endif()

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs)

set(SOURCES_CPP
    config.cpp
    daemon.cpp
    daemon_server.cpp
    file_server.cpp
    functions/gcrypt.cpp
    functions/gstring.cpp
    functions/gsystem.cpp
    #classes/crypto.cpp
    #classes/base64.cpp
    classes/dedicated_server.cpp
    classes/game_server.cpp
    classes/tasks.cpp
    functions/restapi.cpp
    functions/restapi.h
    status.cpp
    status.h
)

if (WIN32)
    list(APPEND SOURCES_CPP control_windows.cpp)
endif()

if (UNIX)
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs)

    list(APPEND SOURCES_CPP control_linux.cpp)
endif()

add_executable(gameap-daemon ${SOURCES_CPP})

if (WIN32)
	target_link_libraries (gameap-daemon gcc stdc++ ws2_32 wsock32 binn boost_system boost_filesystem boost_iostreams boost_thread crypto ssl jsoncpp restclient-cpp curl)

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -march=i686 -static" )
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -march=i686 -static" )
endif()

if (UNIX)
    target_compile_definitions(gameap-daemon PRIVATE NON_DAEMON)
    # target_compile_definitions(gameap-daemon PRIVATE SYSCTL_DAEMON)
    target_compile_definitions(gameap-daemon PRIVATE GDADEMON_LIB_PATH="${GDAEMON_LIBPATH}")

	target_link_libraries (gameap-daemon
        binn
        ${Boost_LIBRARIES}
        pthread
        crypto
        jsoncpp
        dl
        crypto
        restclient-cpp
        curl
    )

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -g" )
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -g" )
endif()
