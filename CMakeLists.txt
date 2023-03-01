cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 17)

project(tinyc)

include_directories(${CMAKE_SOURCE_DIR}/tinyc)

file(GLOB_RECURSE SRC "tinyc/*.cpp" "tinyc/*.h")
add_executable(tinyc ${SRC})

