cmake_minimum_required(VERSION 3.10)

project("tack")

include_directories("include")

add_executable(tack src/main.cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)