cmake_minimum_required(VERSION 2.8)
project(PCLViewer)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")

add_subdirectory(src)
include_directories(src)

add_executable(${PROJECT_NAME} src/main.cpp)
