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

# Optional first real-disc-data boot (kept separate from the validated synthetic probe).
add_executable(r36s_game_boot
  r36s_game_boot.cpp
  "${RE4_ANDROID_CPP}/gx_gl.cpp"
  "${RE4_ANDROID_CPP}/gx_gl_shader.cpp"
  "${RE4_ANDROID_CPP}/gx_texture.cpp"
  "${RE4_WHOLE_RUNTIME}/core_ipc.cpp"
  "${RE4_WHOLE_RUNTIME}/core_ipc_client.cpp"
)
target_include_directories(r36s_game_boot PRIVATE "${RE4_ANDROID_CPP}" "${RE4_WHOLE_RUNTIME}")
target_compile_options(r36s_game_boot PRIVATE -O2 -Wall -Wextra -ffp-contract=off)
target_link_libraries(r36s_game_boot PRIVATE PkgConfig::SDL2 "${GLESV2_LIB}" dl pthread)
# Preserve native frame pointers and ELF DWARF data for R36S SIGSEGV address resolution.
target_compile_options(r36s_game_boot PRIVATE -g -fno-omit-frame-pointer -fno-optimize-sibling-calls)
target_link_options(r36s_game_boot PRIVATE -rdynamic -Wl,--build-id)

# Experimental disk shader cache; build separate to keep validated game_boot unchanged.
# The cache lives on the user's microSD, not in shared Mali/CPU RAM.
add_executable(r36s_game_shadercache
  r36s_game_boot.cpp
  "${RE4_ANDROID_CPP}/gx_gl.cpp"
  "${RE4_ANDROID_CPP}/gx_gl_shader.cpp"
  "${RE4_ANDROID_CPP}/gx_texture.cpp"
  "${RE4_WHOLE_RUNTIME}/core_ipc.cpp"
  "${RE4_WHOLE_RUNTIME}/core_ipc_client.cpp"
)
target_include_directories(r36s_game_shadercache PRIVATE "${RE4_ANDROID_CPP}" "${RE4_WHOLE_RUNTIME}")
target_compile_definitions(r36s_game_shadercache PRIVATE RE4_R36S_SHADER_CACHE=1)
target_compile_options(r36s_game_shadercache PRIVATE -O2 -Wall -Wextra -ffp-contract=off -g -fno-omit-frame-pointer)
target_link_libraries(r36s_game_shadercache PRIVATE PkgConfig::SDL2 "${GLESV2_LIB}" dl pthread)
target_link_options(r36s_game_shadercache PRIVATE -rdynamic -Wl,--build-id)

# Resident Evil 4 PS2-inspired graphics downgrade. Isolated from validated
# stable frontend, shader-cache tests and the original GameCube renderer.
add_executable(r36s_game_ps2
  r36s_game_boot.cpp
  "${RE4_ANDROID_CPP}/gx_gl.cpp"
  "${RE4_ANDROID_CPP}/gx_gl_shader.cpp"
  "${RE4_ANDROID_CPP}/gx_texture.cpp"
  "${RE4_WHOLE_RUNTIME}/core_ipc.cpp"
  "${RE4_WHOLE_RUNTIME}/core_ipc_client.cpp"
)
target_include_directories(r36s_game_ps2 PRIVATE "${RE4_ANDROID_CPP}" "${RE4_WHOLE_RUNTIME}")
target_compile_definitions(r36s_game_ps2 PRIVATE RE4_R36S_LOW_SPEC=1 RE4_R36S_SHADER_CACHE=1)
target_compile_options(r36s_game_ps2 PRIVATE -O2 -Wall -Wextra -ffp-contract=off -g -fno-omit-frame-pointer)
target_link_libraries(r36s_game_ps2 PRIVATE PkgConfig::SDL2 "${GLESV2_LIB}" dl pthread)
target_link_options(r36s_game_ps2 PRIVATE -rdynamic -Wl,--build-id)

# Device-specific builds. GCC on GitHub's native AArch64 runner uses
# Cortex-A35 scheduling/instructions and cross-TU link-time optimization.
# No fast-math: RE4 GX depends on stable floating-point and shader state.
# The existing r36s_game_boot and r36s_game_ps2 are left intact.
set(RE4_R36S_A35_SOURCES
  r36s_game_boot.cpp
  "${RE4_ANDROID_CPP}/gx_gl.cpp"
  "${RE4_ANDROID_CPP}/gx_gl_shader.cpp"
  "${RE4_ANDROID_CPP}/gx_texture.cpp"
  "${RE4_WHOLE_RUNTIME}/core_ipc.cpp"
  "${RE4_WHOLE_RUNTIME}/core_ipc_client.cpp"
)
function(re4_a35_target target half_efb)
  add_executable(${target} ${RE4_R36S_A35_SOURCES})
  target_include_directories(${target} PRIVATE "${RE4_ANDROID_CPP}" "${RE4_WHOLE_RUNTIME}")
  target_compile_definitions(${target} PRIVATE RE4_R36S_LOW_SPEC=1 RE4_R36S_SHADER_CACHE=1 RE4_R36S_A35_TARGET=1)
  if(half_efb)
    target_compile_definitions(${target} PRIVATE RE4_R36S_HALF_EFB=1)
  endif()
  target_compile_options(${target} PRIVATE
    -O3 -mcpu=cortex-a35 -mtune=cortex-a35 -flto=auto
    -ffp-contract=off -fno-omit-frame-pointer
    -ffunction-sections -fdata-sections -Wall -Wextra
  )
  target_link_libraries(${target} PRIVATE PkgConfig::SDL2 "${GLESV2_LIB}" dl pthread)
  target_link_options(${target} PRIVATE
    -flto=auto -Wl,--gc-sections -rdynamic -Wl,--build-id
  )
endfunction()
re4_a35_target(r36s_game_a35_full FALSE)
re4_a35_target(r36s_game_a35_half TRUE)
# A new standalone binary with fixed runtime-selected 75%, 50%, 37.5%
# scales; existing A35 half/full binaries are NOT touched.
re4_a35_target(r36s_game_a35_scalable TRUE)
target_compile_definitions(r36s_game_a35_scalable PRIVATE RE4_R36S_SCALABLE_EFB=1)

# R36S Mali-G31 A/B: glBufferData every GX chunk versus persistent-size
# geometry VBO/IBO reused with glBufferSubData. Both variants run at 480x396,
# same Cortex-A35 -O3/LTO and GL driver timing counters. Unlike the uniform
# ring this relies on the already-proven mode-5 glFinish at every presentation.
# Neither binary replaces the stable renderer or previous A35 profiles.
re4_a35_target(r36s_game_a35_geom_base TRUE)
target_compile_definitions(r36s_game_a35_geom_base PRIVATE
  RE4_R36S_SCALABLE_EFB=1 RE4_R36S_UPLOAD_PROFILE=1
)
re4_a35_target(r36s_game_a35_geom_reuse TRUE)
target_compile_definitions(r36s_game_a35_geom_reuse PRIVATE
  RE4_R36S_SCALABLE_EFB=1 RE4_R36S_UPLOAD_PROFILE=1 RE4_R36S_BUFFER_REUSE=1
)
