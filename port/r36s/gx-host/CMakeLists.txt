cmake_minimum_required(VERSION 3.20)
project(cubeevil4_r36s_gx_host LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
get_filename_component(RE4_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(RE4_ANDROID_CPP "${RE4_ROOT}/port/android-decomp/app/src/main/cpp")
set(RE4_WHOLE_RUNTIME "${RE4_ROOT}/port/whole/runtime")
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
find_library(GLESV2_LIB GLESv2 REQUIRED)
add_executable(r36s_gx_host
  r36s_gx_host.cpp
  "${RE4_ANDROID_CPP}/gx_gl.cpp"
  "${RE4_ANDROID_CPP}/gx_gl_shader.cpp"
  "${RE4_ANDROID_CPP}/gx_texture.cpp"
)
target_include_directories(r36s_gx_host PRIVATE "${RE4_ANDROID_CPP}" "${RE4_WHOLE_RUNTIME}")
target_compile_options(r36s_gx_host PRIVATE -O2 -Wall -Wextra -ffp-contract=off)
target_link_libraries(r36s_gx_host PRIVATE PkgConfig::SDL2 "${GLESV2_LIB}" dl)

# Real-time test: GX commands from a separate Linux ARM64 core process.
add_executable(r36s_live_ipc
  r36s_live_ipc.cpp
  "${RE4_ANDROID_CPP}/gx_gl.cpp"
  "${RE4_ANDROID_CPP}/gx_gl_shader.cpp"
  "${RE4_ANDROID_CPP}/gx_texture.cpp"
  "${RE4_WHOLE_RUNTIME}/core_ipc.cpp"
  "${RE4_WHOLE_RUNTIME}/core_ipc_client.cpp"
)
target_include_directories(r36s_live_ipc PRIVATE "${RE4_ANDROID_CPP}" "${RE4_WHOLE_RUNTIME}")
target_compile_options(r36s_live_ipc PRIVATE -O2 -Wall -Wextra -ffp-contract=off)
target_link_libraries(r36s_live_ipc PRIVATE PkgConfig::SDL2 "${GLESV2_LIB}" dl pthread)
