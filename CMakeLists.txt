cmake_minimum_required(VERSION 2.8)
project(GDaemon)

if (WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)
endif()

if(MSVC)
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
endif()

if (BUILD_TESTING)
    enable_testing()
endif()

set (CMAKE_CXX_STANDARD 17)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_definitions(-std=c++17)
endif()

add_subdirectory(src)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif()
