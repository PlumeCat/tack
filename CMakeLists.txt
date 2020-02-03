cmake_minimum_required(VERSION 3.10)

project("lang")

include_directories("include")

add_executable(lang src/main.cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)