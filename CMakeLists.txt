cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 17)

project(zvm)
include_directories(src)
add_subdirectory(src)
add_subdirectory(test)
