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
elseif(UNIX)
    if(${USE_STATIC_BOOST})
	    set(Boost_USE_STATIC_LIBS ON)
    endif()
endif()

find_package(Boost REQUIRED COMPONENTS system filesystem iostreams thread)
if(NOT Boost_FOUND)
	message(SEND_ERROR "Failed to find boost components")
	return()
endif()
message( STATUS "Found boost: ${Boost_LIBRARIES}" )

find_package(PkgConfig REQUIRED)
#if(NOT PkgConfig_FOUND)
#    message(SEND_ERROR "Failed to find PkgConfig")
#    return()
#endif()

pkg_check_modules(JSONCPP jsoncpp REQUIRED)
link_libraries(${JSONCPP_LIBRARIES})

find_library(BINN_LIBRARY
    NAMES libbinn binn
    HINTS "${CMAKE_PREFIX_PATH}"
)

if( "${BINN_LIBRARY}" STREQUAL "NOTFOUND" )
    message( FATAL_ERROR "Failed to find binn" )
endif()
message( STATUS "Found binn: ${BINN_LIBRARY}" )

find_library(RESTCLIENT_LIBRARY
    NAMES librestclient-cpp restclient-cpp
    HINTS "${CMAKE_PREFIX_PATH}"
)

if( "${RESTCLIENT_LIBRARY}" STREQUAL "RESTCLIENT_LIBRARY-NOTFOUND" )
    message( FATAL_ERROR "Failed to find restclient-cpp" )
endif()
message( STATUS "Found restclient-cpp: ${RESTCLIENT_LIBRARY}" )

find_library(CRYPT_LIBRARY
    NAMES libcrypt crypt
    HINTS "${CMAKE_PREFIX_PATH}"
)

if( "${CRYPT_LIBRARY}" STREQUAL "CRYPT_LIBRARY-NOTFOUND" )
    message( FATAL_ERROR "Failed to find crypt" )
endif()
message( STATUS "Found crypt: ${CRYPT_LIBRARY}" )

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs)

set(SOURCES_CPP
    config.cpp
    daemon.cpp
    daemon_server.cpp
    functions/gstring.cpp
    functions/gsystem.cpp
    classes/dedicated_server.cpp
    classes/game_server.cpp
    classes/tasks.cpp
    functions/restapi.cpp
    functions/auth.cpp
    status.cpp
    components/server/commands_component.cpp
    components/server/files_component.cpp
)

if (WIN32)
    set(GDAEMON_SOURCES_CPP control_windows.cpp)
elseif(UNIX)
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs)
    set(GDAEMON_SOURCES_CPP control_linux.cpp)
endif()

add_executable(gameap-daemon ${SOURCES_CPP} ${GDAEMON_SOURCES_CPP})

if (WIN32)
	target_link_libraries (gameap-daemon gcc stdc++ ws2_32 wsock32 binn boost_system boost_filesystem boost_iostreams boost_thread crypto ssl jsoncpp restclient-cpp curl)

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -march=i686 -static" )
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -march=i686 -static" )
elseif(UNIX)
    target_compile_definitions(gameap-daemon PRIVATE NON_DAEMON)
    # target_compile_definitions(gameap-daemon PRIVATE SYSCTL_DAEMON)
    target_compile_definitions(gameap-daemon PRIVATE GDADEMON_LIB_PATH="${GDAEMON_LIBPATH}")

	target_link_libraries (gameap-daemon
        ${BINN_LIBRARY}
        ${Boost_LIBRARIES}
        ${JSONCPP_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        ${OPENSSL_LIBRARIES}
        ${RESTCLIENT_LIBRARY}
        ${CURL_LIBRARIES}
        ${CMAKE_DL_LIBS}
        ${CRYPT_LIBRARY}
    )

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -g" )
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -g" )
endif()

#--------------------------------------------------------------------------------------------------------
# Tests
#--------------------------------------------------------------------------------------------------------

add_executable(run_tests ${SOURCES_CPP} tests/run_tests.cpp)

if (WIN32)
    target_link_libraries (run_tests gcc stdc++ ws2_32 wsock32 binn boost_system boost_filesystem boost_iostreams boost_thread crypto ssl jsoncpp restclient-cpp curl)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -march=i686 -static" )
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -march=i686 -static" )
elseif(UNIX)
    target_compile_definitions(run_tests PRIVATE NON_DAEMON)
    # target_compile_definitions(gameap-daemon PRIVATE SYSCTL_DAEMON)
    target_compile_definitions(run_tests PRIVATE GDADEMON_LIB_PATH="${GDAEMON_LIBPATH}")

    target_link_libraries (run_tests
        ${BINN_LIBRARY}
        ${Boost_LIBRARIES}
        ${JSONCPP_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        ${OPENSSL_LIBRARIES}
        ${RESTCLIENT_LIBRARY}
        ${CURL_LIBRARIES}
        ${CMAKE_DL_LIBS}
        ${CRYPT_LIBRARY}
    )

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -g" )
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -g" )
endif()