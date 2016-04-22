# SET(CMAKE_SYSTEM_NAME      Windows)
# SET(CMAKE_SYSTEM_PROCESSOR i686)

# SET(CMAKE_C_COMPILER       /usr/bin/i686-w64-mingw32-gcc-win32 )
# SET(CMAKE_CXX_COMPILER     /usr/bin/i686-w64-mingw32-g++-win32 )

cmake_minimum_required(VERSION 2.8)

add_definitions(-std=c++11)

# add_definitions(-ldl)
# target_link_libraries(main dl)

set(Boost_USE_STATIC_LIBS ON)

# find_package(Boost COMPONENTS property_tree REQUIRED)
# if(NOT Boost_FOUND)
	# message(SEND_ERROR "Failed to find boost::property_tree.")
	# return()
# else()
	include_directories(${Boost_INCLUDE_DIRS})
# endif()

add_executable(daemon config.cpp daemon.cpp db/db.cpp)
target_link_libraries (daemon dl)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC" )
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC" )
