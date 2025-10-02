cmake_minimum_required(VERSION 3.10)

project(testserver)

add_executable(testserver testserver.cc)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -Wall -g -ggdb")

include_directories(${CMAKE_SOURCE_DIR}/include)
link_directories(${CMAKE_SOURCE_DIR}/lib)

target_link_libraries(testserver myMuduo pthread)