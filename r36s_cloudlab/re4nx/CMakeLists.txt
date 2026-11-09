cmake_minimum_required(VERSION 3.13)
project(re4nx_r36s_phase3 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable(re4nx src/re4nx.cpp)
target_link_libraries(re4nx PRIVATE ${CMAKE_DL_LIBS})
target_compile_options(re4nx PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -O2)
include(CTest)
if(BUILD_TESTING)
  add_test(NAME nso_synthetic_loader COMMAND re4nx --selftest)
  add_test(NAME memory_budget COMMAND re4nx --budget)
endif()
install(TARGETS re4nx RUNTIME DESTINATION .)

# The tiny freestanding AArch64 binary is built separately with clang+LLD.
# On x86_64, compile the same validator with libc for negative regressions.
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
  add_executable(re4nx-native-host src/native_phase2.c)
  target_compile_definitions(re4nx-native-host PRIVATE RE4NX_HOST_TEST=1)
  target_compile_options(re4nx-native-host PRIVATE -O2 -Wall -Wextra -Werror)
  if(BUILD_TESTING)
    add_test(NAME native_nso_synthetic COMMAND re4nx-native-host --synthetic ${CMAKE_SOURCE_DIR}/samples/synthetic-aarch64.nso)
    add_test(NAME native_memory_probe COMMAND re4nx-native-host --memory-probe)
  endif()
endif()

# Phase 3 synthetic IPC/HLE unit tests for non-ARM64 host development.
# AArch64 uses tools/build-native-phase3-aarch64.sh (no libc) instead.
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
  add_executable(re4nx-phase3-host src/native_phase3.c)
  target_compile_options(re4nx-phase3-host PRIVATE -O2 -Wall -Wextra -Werror)
  if(BUILD_TESTING)
    add_test(NAME phase3_synthetic_ipc COMMAND re4nx-phase3-host --selftest)
    add_test(NAME phase3_handle_stress COMMAND re4nx-phase3-host --stress)
  endif()
endif()

# Phase 4: HIPC and CMIF serialized wire header/parser, subset-only fake sm:.
# This host build uses libc for CI; the real target separately builds a
# freestanding, statically linked Linux ARM64 ELF for synthetic execution.
add_executable(re4nx-phase4-host src/native_phase4.c)
target_compile_definitions(re4nx-phase4-host PRIVATE RE4NX_HOST_TEST=1)
target_compile_options(re4nx-phase4-host PRIVATE -O2 -Wall -Wextra -Werror -Wpedantic)
if(BUILD_TESTING)
  add_test(NAME phase4_hipc_cmif_wire COMMAND re4nx-phase4-host --selftest)
  add_test(NAME phase4_hipc_fuzz_50k COMMAND re4nx-phase4-host --fuzz)
endif()

# Phase 5: synthetic guest register/TLS -> HIPC/CMIF SVC boundary (not a real trap).
add_executable(re4nx-phase5-host src/native_phase5.c)
target_compile_definitions(re4nx-phase5-host PRIVATE RE4NX_HOST_TEST=1)
target_compile_options(re4nx-phase5-host PRIVATE -O2 -Wall -Wextra -Werror -Wpedantic)
if(BUILD_TESTING)
  add_test(NAME phase5_svc_tls_bridge COMMAND re4nx-phase5-host --selftest)
  add_test(NAME phase5_svc_fuzz_50k COMMAND re4nx-phase5-host --fuzz)
endif()
