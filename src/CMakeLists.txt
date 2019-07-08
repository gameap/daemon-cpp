cmake_minimum_required(VERSION 2.8)
project(GDaemon)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/build/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/build/lib)
set(LIBRARY_OUTPUT_PATH ${CMAKE_HOME_DIRECTORY}/build/lib)

if (BUILD_STATIC)
    set(BUILD_STATIC_BOOST ON)

    message( STATUS "Build static" )

    if (UNIX)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".a;.so")
    endif()

    set(BUILD_SHARED_LIBS OFF)
    set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
endif()

if( "${GDAEMON_LIBPATH}" STREQUAL "" )
    set(GDAEMON_LIBPATH /usr/lib/gameap-daemon)
endif()

if (WIN32)
    set(BUILD_STATIC_BOOST ON)
endif()

if (BUILD_STATIC_BOOST)
    set(Boost_USE_STATIC_LIBS ON)
    find_package(Boost REQUIRED COMPONENTS system filesystem iostreams thread)
else()
    find_package(Boost REQUIRED COMPONENTS system filesystem iostreams thread)
endif()

if(NOT Boost_FOUND)
    message(SEND_ERROR "Failed to find boost components")
    return()
endif()
message( STATUS "Found boost: ${Boost_LIBRARIES}" )

find_library(JSONCPP_LIBRARY
    NAMES libjson libjsoncpp jsoncpp
    HINTS "${CMAKE_PREFIX_PATH}"
    )

if( "${JSONCPP_LIBRARY}" STREQUAL "JSONCPP_LIBRARY-NOTFOUND" )
    message( FATAL_ERROR "Failed to find jsoncpp" )
endif()
message( STATUS "Found jsoncpp: ${JSONCPP_LIBRARY}" )

find_path(JSONCPP_INCLUDE_PATH NAMES ${CMAKE_INCLUDE_PATH}/jsoncpp)
if (NOT ${JSONCPP_INCLUDE_PATH} STREQUAL "JSONCPP_INCLUDE_PATH-NOTFOUND")
    include_directories(${JSONCPP_INCLUDE_PATH}/jsoncpp)
endif()

find_library(BINN_LIBRARY
    NAMES libbinn binn
    HINTS "${CMAKE_PREFIX_PATH}"
    )

if( "${BINN_LIBRARY}" STREQUAL "BINN_LIBRARY-NOTFOUND" )
    message( FATAL_ERROR "Failed to find binn" )
endif()
message( STATUS "Found binn: ${BINN_LIBRARY}" )

find_path(BINN_INCLUDE_PATH NAMES ${CMAKE_INCLUDE_PATH}/binn)
if (NOT ${BINN_INCLUDE_PATH} STREQUAL "BINN_INCLUDE_PATH-NOTFOUND")
    include_directories(${BINN_INCLUDE_PATH}/binn)
endif()

find_library(RESTCLIENT_LIBRARY
    NAMES librestclient-cpp restclient-cpp
    HINTS "${CMAKE_PREFIX_PATH}"
    )

if( "${RESTCLIENT_LIBRARY}" STREQUAL "RESTCLIENT_LIBRARY-NOTFOUND" )
    message( FATAL_ERROR "Failed to find restclient-cpp" )
endif()
message( STATUS "Found restclient-cpp: ${RESTCLIENT_LIBRARY}" )

if(UNIX)
    find_library(CRYPT_LIBRARY
        NAMES libcrypt crypt
        HINTS "${CMAKE_PREFIX_PATH}"
        )

    if( "${CRYPT_LIBRARY}" STREQUAL "CRYPT_LIBRARY-NOTFOUND" )
        message( FATAL_ERROR "Failed to find crypt" )
    endif()
    message( STATUS "Found crypt: ${CRYPT_LIBRARY}" )
endif()

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(ZLIB REQUIRED)

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
    classes/game_servers_list.cpp
    classes/game_server.cpp
    classes/tasks.cpp
    functions/restapi.cpp
    functions/auth.cpp
    components/server/commands_component.cpp
    components/server/files_component.cpp
)

if (WIN32)
    set(GDAEMON_SOURCES_CPP control_windows.cpp)
elseif(UNIX)
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs)
    set(GDAEMON_SOURCES_CPP control_linux.cpp)
    set(SOURCES_CPP ${SOURCES_CPP} status.cpp)
endif()

add_executable(gameap-daemon ${SOURCES_CPP} ${GDAEMON_SOURCES_CPP})

if (WIN32)
    target_link_libraries (gameap-daemon
        ${BINN_LIBRARY}
        ${Boost_LIBRARIES}
        ${RESTCLIENT_LIBRARY}
        ${CURL_LIBRARIES}
        ${JSONCPP_LIBRARY}
        ${OPENSSL_LIBRARIES}
        ${CMAKE_DL_LIBS}
        ${ZLIB_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        )
elseif(UNIX)

    if(SYSCTL_DAEMON)
        message( STATUS "Compiling sysctl daemon" )
        target_compile_definitions(gameap-daemon PRIVATE SYSCTL_DAEMON)
    elseif(NON_DAEMON)
        message( STATUS "Compiling non daemon" )
        target_compile_definitions(gameap-daemon PRIVATE NON_DAEMON)
    endif()

    target_compile_definitions(gameap-daemon PRIVATE GDADEMON_LIB_PATH="${GDAEMON_LIBPATH}")

    if (BUILD_STATIC)
        set_target_properties(gameap-daemon PROPERTIES LINK_SEARCH_START_STATIC 1)
        set_target_properties(gameap-daemon PROPERTIES LINK_SEARCH_END_STATIC 1)
    endif()
        
    target_link_libraries (gameap-daemon
        ${BINN_LIBRARY}
        ${Boost_LIBRARIES}
        ${RESTCLIENT_LIBRARY}
        ${CURL_LIBRARIES}
        ${JSONCPP_LIBRARY}
        ${OPENSSL_LIBRARIES}
        ${CMAKE_DL_LIBS}
        ${CRYPT_LIBRARY}
        ${ZLIB_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        )

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -g" )
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -g" )
endif()