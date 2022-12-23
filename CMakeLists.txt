# CMakeLists.txt

cmake_minimum_required (VERSION 3.23)

project ("pl5")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

include_directories(pl5 "..//jlib")
#include_directories(pl5 ".//dep")
#include_directories(pl5 "..//glfw//include")
#include_directories(pl5 "..//stb")
#include_directories(pl5 "..//lua-5.4.2//include")

# Add source to this project's executable.
file(GLOB_RECURSE source_files src/*.cpp src/**/*.cpp)
message("Source files: ${source_files}")
add_executable (pl5 ${source_files})
# target_link_libraries(pl5 "${CMAKE_SOURCE_DIR}//..//glfw//out//build//x64-${CMAKE_BUILD_TYPE}//src//glfw3.lib")
# target_link_libraries(pl5 "${CMAKE_SOURCE_DIR}//..//lua-5.4.2//lua54.lib")
