cmake_minimum_required(VERSION 3.25)
project(acvd LANGUAGES C CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_definitions(_WIN32_WINNT=0x0A00 WINVER=0x0A00 NOMINMAX UNICODE _UNICODE)

# The port's version (VERSION; the release's changelog names it), logged by the game.
file(STRINGS ${CMAKE_SOURCE_DIR}/VERSION ACVD_VERSION LIMIT_COUNT 1)
set(AC4_VERSION "${ACVD_VERSION}")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/VERSION)
file(CONFIGURE OUTPUT ${CMAKE_BINARY_DIR}/version/acvd_version.h
     CONTENT "// Generated from VERSION by CMakeLists.txt.\n#pragma once\n#define ACVD_VERSION \"@ACVD_VERSION@\"\n#define AC4_VERSION \"@ACVD_VERSION@\"\n" @ONLY)
file(CONFIGURE OUTPUT ${CMAKE_BINARY_DIR}/version/ac4_version.h
     CONTENT "// Generated from VERSION by CMakeLists.txt.\n#pragma once\n#define ACVD_VERSION \"@ACVD_VERSION@\"\n#define AC4_VERSION \"@ACVD_VERSION@\"\n" @ONLY)
include_directories(${CMAKE_BINARY_DIR}/version)

# Developer diagnostics (control-file commands, profiler, memory search and watchpoints, traces,
# frame reports, A/B switches, test autopilot).
option(AC4_DEV "Build with developer diagnostics" OFF)
if(AC4_DEV)
  add_compile_definitions(AC4_DEV=1)
else()
  add_compile_definitions(AC4_DEV=0)
endif()

if(MINGW)
  add_link_options(-static -Wl,--no-insert-timestamp)
endif()
add_compile_options(-ffile-prefix-map=${CMAKE_SOURCE_DIR}=.)
# UTF-8 process code page for every executable (paths with any characters).
set(AC4_UTF8_RC ${CMAKE_SOURCE_DIR}/src/common/utf8.rc)

# ---- third party ----
file(GLOB ZLIB_SRC ${CMAKE_SOURCE_DIR}/third_party/zlib/*.c)
add_library(ac4_zlib STATIC ${ZLIB_SRC})
target_include_directories(ac4_zlib PUBLIC ${CMAKE_SOURCE_DIR}/third_party/zlib)
target_compile_options(ac4_zlib PRIVATE -w)
add_library(acvd_zlib ALIAS ac4_zlib)

# ---- common ----
add_library(ac4_common STATIC
  src/common/bytes.cpp
  src/common/crypto.cpp
  src/common/known_inputs.cpp
  src/common/file.cpp
  src/common/ini.cpp
  src/common/privacy.cpp
  src/common/settings_file.cpp
  src/common/sfo.cpp)
target_include_directories(ac4_common PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(ac4_common PUBLIC ac4_zlib bcrypt)
target_compile_options(ac4_common PRIVATE -Wall -Wextra)
add_library(acvd_common ALIAS ac4_common)

# ---- PPU decoding (shared by recompiler and runtime) ----
add_library(ac4_ppu STATIC src/ppu/ppu_decode.cpp)
target_link_libraries(ac4_ppu PUBLIC ac4_common)
target_compile_options(ac4_ppu PRIVATE -Wall -Wextra)
add_library(acvd_ppu ALIAS ac4_ppu)

# ---- SPU ISA, chunk discovery (shared by the SPU recompiler and the runtime) ----
add_library(ac4_spu STATIC src/spu/spu_isa.cpp)
target_link_libraries(ac4_spu PUBLIC ac4_common)
target_compile_options(ac4_spu PRIVATE -Wall -Wextra)
add_library(acvd_spu ALIAS ac4_spu)

add_executable(acvd-spurecomp src/spurecomp/main.cpp ${AC4_UTF8_RC})
target_link_libraries(acvd-spurecomp PRIVATE ac4_spu)
target_compile_options(acvd-spurecomp PRIVATE -Wall -Wextra)
add_executable(ac4-spurecomp ALIAS acvd-spurecomp)

# ---- recompiler ----
add_executable(acvd-recomp
  src/recomp/main.cpp
  src/recomp/image.cpp
  src/recomp/analysis.cpp
  src/recomp/emit.cpp
  ${AC4_UTF8_RC})
target_link_libraries(acvd-recomp PRIVATE ac4_ppu)
target_compile_options(acvd-recomp PRIVATE -Wall -Wextra)
add_executable(ac4-recomp ALIAS acvd-recomp)

# ---- generated PPU code (game-derived, from local/gen/ppu; never distributed) ----
set(ACVD_GEN_DIR "${CMAKE_SOURCE_DIR}/local/gen/ppu" CACHE PATH "Directory with acvd-recomp output")
file(GLOB ACVD_GEN_SRC CONFIGURE_DEPENDS "${ACVD_GEN_DIR}/*.cpp")
set(AC4_CPU_FLAGS -march=haswell)  # AVX2 + FMA + BMI2 (x86-64-v3)

if(ACVD_GEN_SRC)
  add_library(acvd_ppu_gen STATIC ${ACVD_GEN_SRC})
  target_include_directories(acvd_ppu_gen PRIVATE ${CMAKE_SOURCE_DIR}/src)
  target_compile_options(acvd_ppu_gen PRIVATE -O2 ${AC4_CPU_FLAGS} -fno-strict-aliasing -w)
  add_library(ac4_ppu_gen ALIAS acvd_ppu_gen)

  file(GLOB ACVD_RT_SRC CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/runtime/*.cpp
    ${CMAKE_SOURCE_DIR}/src/runtime/hle/*.cpp
    ${CMAKE_SOURCE_DIR}/src/runtime/lv2/*.cpp
    ${CMAKE_SOURCE_DIR}/src/runtime/rsx/*.cpp
    ${CMAKE_SOURCE_DIR}/src/runtime/gpu/*.cpp
    ${CMAKE_SOURCE_DIR}/src/runtime/audio/*.cpp
    ${CMAKE_SOURCE_DIR}/src/runtime/media/*.cpp
    ${CMAKE_SOURCE_DIR}/src/runtime/spu/*.cpp
    ${CMAKE_SOURCE_DIR}/src/runtime/ui/*.cpp)
  # Recompiled SPU code (game-derived, from local/gen/spu; never distributed).
  set(ACVD_SPU_GEN_DIR "${CMAKE_SOURCE_DIR}/local/gen/spu" CACHE PATH "Directory with acvd-spurecomp output")
  file(GLOB ACVD_SPU_GEN_SRC CONFIGURE_DEPENDS "${ACVD_SPU_GEN_DIR}/*.cpp")
  if(NOT ACVD_SPU_GEN_SRC)
    set(ACVD_SPU_GEN_SRC ${CMAKE_SOURCE_DIR}/src/spu/spu_gen_empty.cpp)
  endif()
  add_library(acvd_spu_gen STATIC ${ACVD_SPU_GEN_SRC})
  target_include_directories(acvd_spu_gen PRIVATE ${CMAKE_SOURCE_DIR}/src)
  target_compile_options(acvd_spu_gen PRIVATE -O2 ${AC4_CPU_FLAGS} -fno-strict-aliasing -w)
  add_library(ac4_spu_gen ALIAS acvd_spu_gen)

  add_executable(acvd ${ACVD_RT_SRC} ${AC4_UTF8_RC} src/runtime/game.rc)
  if(NOT AC4_DEV)  # players' builds open no console window (the run log holds every message)
    set_target_properties(acvd PROPERTIES WIN32_EXECUTABLE ON)
  endif()
  target_include_directories(acvd PRIVATE ${CMAKE_SOURCE_DIR}/src)
  target_compile_options(acvd PRIVATE ${AC4_CPU_FLAGS} -fno-strict-aliasing -Wall -Wextra -Wno-unused-parameter -g)
  # Whole-archive: the function table references everything anyway; keeps HLE registrars alive.
  target_link_libraries(acvd PRIVATE acvd_ppu_gen acvd_spu_gen ac4_spu ac4_ppu ac4_common d3d11 dxgi d3dcompiler d2d1 dwrite ole32 psapi
                        synchronization hid setupapi)
endif()
