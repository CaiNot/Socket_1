cmake_minimum_required(VERSION 3.12)
project(Socket_1)

set(CMAKE_CXX_STANDARD 14)

link_libraries(ws2_32)

add_executable(Socket_1 main.cpp WinsockEnv.h WinsockEnv.cpp)