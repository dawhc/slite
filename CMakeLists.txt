cmake_minimum_required(VERSION 2.6)
project (webserver)

set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-g -O2 -Wall -pthread")
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SRC_LIST)
add_executable(server ${SRC_LIST})
