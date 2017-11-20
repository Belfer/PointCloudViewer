cmake_minimum_required(VERSION 2.8)

set(CLIENT_HDRS
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui_impl.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui_style.h"
    PARENT_SCOPE
)

set(CLIENT_SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui_impl.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    PARENT_SCOPE
)
