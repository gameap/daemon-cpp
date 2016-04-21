cmake_minimum_required(VERSION 2.8)

add_definitions(-std=c++11)
add_definitions(-ldl)

set(Boost_USE_STATIC_LIBS ON)

# find_package(Boost COMPONENTS property_tree REQUIRED)
# if(NOT Boost_FOUND)
	# message(SEND_ERROR "Failed to find boost::property_tree.")
	# return()
# else()
	include_directories(${Boost_INCLUDE_DIRS})
# endif()

add_executable(daemon config.cpp db.cpp daemon.cpp)
