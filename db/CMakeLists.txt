if (WIN32)
    SET(CMAKE_SYSTEM_NAME      Windows)
    # SET(CMAKE_SYSTEM_PROCESSOR i686)

    SET(CMAKE_C_COMPILER       gcc-4.9)
    SET(CMAKE_CXX_COMPILER     g++-4.9)
endif()

cmake_minimum_required(VERSION 2.8)

add_definitions(-std=c++11)
# LINK_LIBRARIES(mysql)
# LINK_LIBRARIES(ws2_32 mswsock netapi32)
# add_definitions(-ldl)
# target_link_libraries(main dl)

#set(CMAKE_CXX_FLAGS ${CMAKE_CSS_FLAGS} "-lwsock32 -add-stdcall-alias")
#set(CMAKE_CXX_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "--add-stdcall-alias")
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_CXX_LINK_FLAGS} -asdfasdfa --add-stdcall-alias")

if (WIN32)
	#set(CMAKE_CXX_FLAGS ${CMAKE_CSS_FLAGS} "-lwsock32 --add-stdcall-alias")
endif()

if (LINUX)
    #set(CMAKE_CXX_FLAGS ${CMAKE_CSS_FLAGS} "-lwsock32")
endif()

# set(Boost_USE_STATIC_LIBS ON)

# CMAKE_IMPORT_LIBRARY_PREFIX

add_library(mysql SHARED mysql.cpp ../functions/gstring.cpp )
set(LIBRARY_OUTPUT_PATH ${CMAKE_HOME_DIRECTORY}/build/lib/db)

if (WIN32)
	target_link_libraries(mysql gcc stdc++ mysql_dll)
	
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static" )
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static" )
endif()

if (UNIX)
	target_link_libraries (mysql mysqlclient)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fPIC" )
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fPIC" )
endif()

set_target_properties(mysql PROPERTIES PREFIX "")
