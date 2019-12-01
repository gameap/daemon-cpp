cmake_minimum_required(VERSION 2.8)
project(GDaemonTests)

#--------------------------------------------------------------------------------------------------------
# Including Google Tests
#--------------------------------------------------------------------------------------------------------

configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)

execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
    message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
        ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
        EXCLUDE_FROM_ALL)

include_directories("${gtest_SOURCE_DIR}/include")

#--------------------------------------------------------------------------------------------------------
# Including Projects deps
#--------------------------------------------------------------------------------------------------------

include_directories(${CMAKE_HOME_DIRECTORY}/src)

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

if ("${Boost_LIBRARIES}" STREQUAL "")
    set(BOOST_LIBS
            Boost::thread
            Boost::system
            Boost::filesystem
            Boost::iostreams
            )
else()
    set(BOOST_LIBS
            ${Boost_LIBRARIES}
            )
endif()
message( STATUS "Found boost: ${BOOST_LIBS}" )

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
include_directories(../plog/include)
include_directories(../gmock-global/include)

#--------------------------------------------------------------------------------------------------------
# Tests
#--------------------------------------------------------------------------------------------------------

if (CMAKE_COMPILER_IS_GNUCXX)
    set (CMAKE_CXX_FLAGS "--coverage")
endif()

set(SOURCES_CPP
        ${CMAKE_HOME_DIRECTORY}/src/config.cpp
        ${CMAKE_HOME_DIRECTORY}/src/state.cpp
        ${CMAKE_HOME_DIRECTORY}/src/daemon.cpp
        ${CMAKE_HOME_DIRECTORY}/src/daemon_server.cpp
        ${CMAKE_HOME_DIRECTORY}/src/functions/gstring.cpp
        ${CMAKE_HOME_DIRECTORY}/src/functions/gsystem.cpp
        ${CMAKE_HOME_DIRECTORY}/src/classes/dedicated_server.cpp
        ${CMAKE_HOME_DIRECTORY}/src/classes/game_servers_list.cpp
        ${CMAKE_HOME_DIRECTORY}/src/classes/game_server.cpp
        ${CMAKE_HOME_DIRECTORY}/src/classes/game_server_installer.cpp
        ${CMAKE_HOME_DIRECTORY}/src/classes/cmd_output.cpp
        ${CMAKE_HOME_DIRECTORY}/src/classes/task_list.cpp
        ${CMAKE_HOME_DIRECTORY}/src/classes/tasks.cpp
        ${CMAKE_HOME_DIRECTORY}/src/functions/restapi.cpp
        ${CMAKE_HOME_DIRECTORY}/src/functions/auth.cpp
        ${CMAKE_HOME_DIRECTORY}/src/components/server/commands_component.cpp
        ${CMAKE_HOME_DIRECTORY}/src/components/server/files_component.cpp
        ${CMAKE_HOME_DIRECTORY}/src/components/server/status_component.cpp
        )

if(UNIX)
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs)
    set(SOURCES_CPP ${SOURCES_CPP} ${CMAKE_HOME_DIRECTORY}/src/status.cpp)
endif()

add_executable(run_tests
        ${SOURCES_CPP} ${GDAEMON_SOURCES_CPP}
        unit/state_test.cpp
        unit/classes/game_server_installer_test.cpp
        unit/classes/cmd_output_test.cpp
)

target_link_libraries(run_tests
        ${BINN_LIBRARY}
        ${BOOST_LIBS}
        ${RESTCLIENT_LIBRARY}
        ${CURL_LIBRARIES}
        ${JSONCPP_LIBRARY}
        ${OPENSSL_LIBRARIES}
        ${CMAKE_DL_LIBS}
        ${CRYPT_LIBRARY}
        ${ZLIB_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        gtest_main
        gmock_main
)

add_test(NAME control_linux_test COMMAND run_tests)