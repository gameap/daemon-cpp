cmake_minimum_required(VERSION 2.8)
project(GDaemon)

#--------------------------------------------------------------------------------------------------------
# Tests
#--------------------------------------------------------------------------------------------------------

include_directories(${CMAKE_HOME_DIRECTORY}/src)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(SOURCES_CPP
    ${CMAKE_HOME_DIRECTORY}/src/config.cpp
    ${CMAKE_HOME_DIRECTORY}/src/daemon.cpp
    ${CMAKE_HOME_DIRECTORY}/src/daemon_server.cpp
    ${CMAKE_HOME_DIRECTORY}/src/functions/gstring.cpp
    ${CMAKE_HOME_DIRECTORY}/src/functions/gsystem.cpp
    ${CMAKE_HOME_DIRECTORY}/src/classes/dedicated_server.cpp
    ${CMAKE_HOME_DIRECTORY}/src/classes/game_server.cpp
    ${CMAKE_HOME_DIRECTORY}/src/classes/tasks.cpp
    ${CMAKE_HOME_DIRECTORY}/src/functions/restapi.cpp
    ${CMAKE_HOME_DIRECTORY}/src/functions/auth.cpp
    ${CMAKE_HOME_DIRECTORY}/src/status.cpp
    ${CMAKE_HOME_DIRECTORY}/src/components/server/commands_component.cpp
    ${CMAKE_HOME_DIRECTORY}/src/components/server/files_component.cpp
    )

add_executable(run_tests ${SOURCES_CPP} run_tests.cpp)

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