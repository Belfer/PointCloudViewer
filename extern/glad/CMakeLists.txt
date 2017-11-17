cmake_minimum_required(VERSION 2.8)
project(GLAD)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(glad
  ${CMAKE_CURRENT_SOURCE_DIR}/src/glad.c
  ${CMAKE_CURRENT_SOURCE_DIR}/include/glad/glad.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/KHR/khrplatform.h
)
