cmake_minimum_required(VERSION 3.21)

project(storm-audio)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

### Set up third-party dependencies
include(cmake/3rd_party.cmake)

include(cmake/target.cmake)

add_subdirectory(src)
add_subdirectory(test)
