cmake_minimum_required(VERSION 2.8)

project(ImGui)

# Header files
set(IMGUI_HDRS
    imconfig.h
    imgui.h
    imgui_internal.h
    stb_rect_pack.h
    stb_textedit.h
    stb_truetype.h
)

# Source files
set(IMGUI_SRCS
    imgui.cpp
    imgui_draw.cpp
)

add_library(imgui ${IMGUI_HDRS} ${IMGUI_SRCS})
