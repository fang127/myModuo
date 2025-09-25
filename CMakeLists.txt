cmake_minimum_required(VERSION 3.10)

project(myMuduo)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_DEBUG_FLAGS "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
set(CMAKE_C_DEBUG_FLAGS "$ENV{CFLAGS} -O0 -Wall -g -ggdb")

include_directories(./include)

aux_source_directory(./src SRC_LIST)

add_library(${PROJECT_NAME} SHARED ${SRC_LIST})

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)