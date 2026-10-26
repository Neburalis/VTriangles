cmake_minimum_required(VERSION 3.20)

project(triangle)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_SANITIZER "Enable Address and Undefined Behavior sanitizers" OFF)
if(ENABLE_SANITIZER)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
    
    message(STATUS "Sanitizers are ENABLED")
else()
    message(STATUS "Sanitizers are DISABLED")
endif()

find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)
add_subdirectory(external/glad)
set(GLM_BUILD_LIBRARY OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glm)

set(IMGUI_SOURCES
    ${CMAKE_SOURCE_DIR}/external/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/imgui_demo.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/backends/imgui_impl_opengl3.cpp
)

function(add_gl_app Name)
    add_executable(${Name} ${ARGN})
    target_include_directories(
        ${Name} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/external/stb_image
        ${CMAKE_SOURCE_DIR}/external/imgui
        ${CMAKE_SOURCE_DIR}/external/imgui/backends
    )
    target_link_libraries(${Name} PRIVATE
        glad::glad
        glm::glm
        glfw
        OpenGL::GL
    )
endfunction()

add_gl_app(main ${CMAKE_SOURCE_DIR}/src/main.cpp ${IMGUI_SOURCES})