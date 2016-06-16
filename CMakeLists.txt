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
    SET(CMAKE_C_COMPILER       gcc-4.8)
    SET(CMAKE_CXX_COMPILER     g++-4.8)
endif()

cmake_minimum_required(VERSION 2.8)

set (CMAKE_CXX_STANDARD 11)
add_definitions(-std=c++11)

if (WIN32)
	set(Boost_USE_STATIC_LIBS ON)
endif()

if (LINUX)
	set(Boost_USE_STATIC_LIBS ON)
endif()

find_package(Boost COMPONENTS system filesystem thread REQUIRED)
if(NOT Boost_FOUND)
	message(SEND_ERROR "Failed to find boost components")
	return()
endif()

# find_package(Boost COMPONENTS thread REQUIRED)
# if(NOT Boost_FOUND)
	# message(SEND_ERROR "Failed to find boost::thread.")
	# return()
# endif()

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs)

set(SOURCES_CPP
    config.cpp
    daemon.cpp
    db/db.cpp
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
)

if (WIN32)
    list(APPEND SOURCES_CPP control_windows.cpp)
endif()

if (UNIX)
    list(APPEND SOURCES_CPP control_linux.cpp)
endif()

add_executable(daemon ${SOURCES_CPP})

if (WIN32)
	target_link_libraries (daemon gcc stdc++ ws2_32 wsock32 binn boost_system boost_filesystem boost_iostreams boost_thread crypto ssl jsoncpp)

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -march=i686 -static" )
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -march=i686 -static" )
endif()

if (UNIX)
	target_link_libraries (daemon stdc++ m dl binn boost_system boost_filesystem boost_iostreams boost_thread pthread crypto mcrypt jsoncpp)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -g" )
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -g" )
endif()
