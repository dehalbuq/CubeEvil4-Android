cmake_minimum_required(VERSION 3.20)
project(cubeevil4_r36s LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(SDL2 CONFIG REQUIRED)
find_package(Threads REQUIRED)
find_library(RE4_GLES_LIBRARY NAMES GLESv2 REQUIRED)
find_library(RE4_EGL_LIBRARY NAMES EGL REQUIRED)

get_filename_component(RE4_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(GX "${RE4_ROOT}/port/android-decomp/app/src/main/cpp")
set(RUNTIME "${RE4_ROOT}/port/whole/runtime")
add_executable(re4_r36s
    linux_frontend.cpp
    "${GX}/gx_gl.cpp"
    "${GX}/gx_core.cpp"
    "${GX}/gx_renderer.cpp"
    "${GX}/gx_gl_shader.cpp"
    "${GX}/gx_texture.cpp")
target_include_directories(re4_r36s PRIVATE "${GX}" "${RUNTIME}")
target_link_libraries(re4_r36s PRIVATE SDL2::SDL2 Threads::Threads
    "${RE4_GLES_LIBRARY}" "${RE4_EGL_LIBRARY}" dl)
target_compile_options(re4_r36s PRIVATE -Wall -Wextra -ffp-contract=off -fno-fast-math)
install(TARGETS re4_r36s RUNTIME DESTINATION .)
