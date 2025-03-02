cmake_minimum_required(VERSION 3.30)
project(kdb)

set(CMAKE_CXX_STANDARD 20)

add_executable(tests tests.cpp)
if(WIN32)
target_link_libraries(tests ws2_32)
endif(WIN32)
