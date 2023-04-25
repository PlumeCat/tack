# CMakeLists.txt

cmake_minimum_required(VERSION 3.22)
set(PROJECT_NAME tack)
set(LIB_NAME tack_lib)
set(TEST_NAME ${PROJECT_NAME})
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")
endif()

project(${PROJECT_NAME})
message("Project:          ${PROJECT}")
message("CMake build type: ${CMAKE_BUILD_TYPE}")

string(REGEX REPLACE "([\\/\\-]O)3" "\\12" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

include_directories(${TEST_NAME} "${CMAKE_CURRENT_LIST_DIR}/../jlib/")

file(GLOB_RECURSE source_lib src/*.cpp)
file(GLOB_RECURSE source_test test/*.cpp)

add_library(${LIB_NAME} ${source_lib})
set_property(TARGET ${LIB_NAME} PROPERTY OUTPUT_NAME ${PROJECT_NAME})
add_executable(${TEST_NAME} ${source_test})

target_link_libraries(${TEST_NAME} ${LIB_NAME})

# turn warnings up
if(MSVC)
    if (${CMAKE_BUILD_TYPE} STREQUAL Release)
        message("MSVC release")
        target_compile_options(${LIB_NAME} PRIVATE /W4 /02 /0t /0y)
        target_compile_options(${TEST_NAME} PRIVATE /W4 /02 /0t /0y)
    else()
        message("MSVC debug")
        target_compile_options(${LIB_NAME} PRIVATE /W4)
        target_compile_options(${TEST_NAME} PRIVATE /W4)
    endif()
else()
    message("GCC or clang")
    target_compile_options(${LIB_NAME} PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(${TEST_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()