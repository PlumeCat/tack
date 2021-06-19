cmake_minimum_required(VERSION 3.10)
project(lang VERSION 0.1.0)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

# include(CTest)
# enable_testing()

file(GLOB_RECURSE sources "src/*.cpp")

message(STATUS "Source files:")
message(STATUS ${sources})

add_executable(lang ${sources})

# set(CPACK_PROJECT_NAME ${PROJECT_NAME})
# set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
# include(CPack)

include_directories("include")