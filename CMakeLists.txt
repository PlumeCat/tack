# CMakeLists.txt

cmake_minimum_required(VERSION 3.22)
set(PROJECT_NAME pl5)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)
# set(CMAKE_BUILD_TYPE Debug) # TODO: debug and release build

project(${PROJECT_NAME})
message("Project:          ${PROJECT}")
message("CMake build type: ${CMAKE_BUILD_TYPE}")

include_directories(${PROJECT_NAME} "${CMAKE_CURRENT_LIST_DIR}/../jlib/")

file(GLOB_RECURSE source_files src/*.cpp)
add_executable(${PROJECT_NAME} ${source_files})
