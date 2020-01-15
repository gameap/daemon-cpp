cmake_minimum_required(VERSION 2.8)
project(GDaemon)

if (BUILD_TESTING)
	enable_testing()
endif()

set (CMAKE_CXX_STANDARD 11)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_definitions(-std=c++11)
endif()

add_subdirectory(src)

if (BUILD_TESTING)
	add_subdirectory(tests)
endif()
