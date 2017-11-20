# Find packages.
find_package(OpenGL REQUIRED) 

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

add_subdirectory(glad)
add_subdirectory(imgui)

add_library(TOL tol/tiny_obj_loader.cc)

set(DEPENDENCIES_INCLUDE
  ${CMAKE_CURRENT_SOURCE_DIR}/glad/include
  ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include
  ${CMAKE_CURRENT_SOURCE_DIR}/glm
  ${CMAKE_CURRENT_SOURCE_DIR}/tol
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui
  PARENT_SCOPE
)

set(DEPENDENCIES_LIBS
  ${OPENGL_LIBRARY}
  glad
  glfw
  TOL
  imgui
  PARENT_SCOPE
)
