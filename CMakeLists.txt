cmake_minimum_required(VERSION 3.16)
project(openloop_loadgen CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

add_compile_options(-Wall -Wextra -Wshadow)

add_executable(echo_server src/echo_server.cpp)
add_executable(loadgen     src/loadgen.cpp)