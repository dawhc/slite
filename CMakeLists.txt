cmake_minimum_required(VERSION 2.6)
project (webserver)

# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O2 -Wall -pthread -std=c11 -D_POSIX_C_SOURCE=199309L")
# set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SRC_LIST)
add_executable(server ${SRC_LIST})
