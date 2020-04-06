set(SOURCES_CPP
        ../mocks/config_mock.cpp
        ${CMAKE_HOME_DIRECTORY}/src/state.cpp
        ${CMAKE_HOME_DIRECTORY}/src/daemon.cpp
        ${CMAKE_HOME_DIRECTORY}/src/daemon_server.cpp
        ${CMAKE_HOME_DIRECTORY}/src/functions/gstring.cpp
        ${CMAKE_HOME_DIRECTORY}/src/functions/gsystem.cpp
        ${CMAKE_HOME_DIRECTORY}/src/classes/dedicated_server.cpp
        ${CMAKE_HOME_DIRECTORY}/src/classes/game_servers_list.cpp
        ${CMAKE_HOME_DIRECTORY}/src/commands/game_server_installer.cpp
        ${CMAKE_HOME_DIRECTORY}/src/commands/game_server_cmd.cpp
        ${CMAKE_HOME_DIRECTORY}/src/commands/dedicated_server_cmd.cpp
        ${CMAKE_HOME_DIRECTORY}/src/classes/cmd_output.cpp
        ${CMAKE_HOME_DIRECTORY}/src/classes/servers_tasks.cpp
        ${CMAKE_HOME_DIRECTORY}/src/classes/gdaemon_tasks.cpp
        ../mocks/restapi_mock.cpp
        ${CMAKE_HOME_DIRECTORY}/src/functions/auth.cpp
        ${CMAKE_HOME_DIRECTORY}/src/components/server/commands_component.cpp
        ${CMAKE_HOME_DIRECTORY}/src/components/server/files_component.cpp
        ${CMAKE_HOME_DIRECTORY}/src/components/server/status_component.cpp
)

if(UNIX)
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs)
    set(SOURCES_CPP ${SOURCES_CPP} ${CMAKE_HOME_DIRECTORY}/src/status.cpp)
else()
    set(SOURCES_CPP ${SOURCES_CPP} ../status_vars.cpp)
endif()

add_executable(run_functional_tests
        ${SOURCES_CPP} ${GDAEMON_SOURCES_CPP}
        common.cpp
)

target_link_libraries(run_functional_tests
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

add_test(NAME functional_tests COMMAND run_functional_tests)