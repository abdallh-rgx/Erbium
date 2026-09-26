cmake_minimum_required(VERSION 3.22)
project(erbium-android LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Silence some warnings for the upstream code we compile unmodified later
add_compile_options(-Wno-unused-parameter -Wno-unused-variable -Wno-unused-function)

# Baked per-version offsets (see scripts/analyze_libue4.py). The analyzer in
# CI writes android/Generated/offsets-<version>.h; the build selects via
# ERBIUM_TARGET_VERSION. Default = 21.30.0-CL-21088273 (first baked target).
set(ERBIUM_TARGET_VERSION "21.30" CACHE STRING "Fortnite version whose offsets are baked")
message(STATUS "Erbium: target version ${ERBIUM_TARGET_VERSION}")

add_library(erbium SHARED
    src/entry.cpp
    src/owen.c
    src/platform.cpp
)

target_include_directories(erbium PRIVATE src Generated)
target_compile_definitions(erbium PRIVATE
    ERBIUM_TARGET_VERSION=${ERBIUM_TARGET_VERSION}
    ERBIUM_HAS_OWEN=1
)
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Generated/offsets-${ERBIUM_TARGET_VERSION}.h")
    message(FATAL_ERROR "No baked offsets for ${ERBIUM_TARGET_VERSION} — run scripts/analyze_libue4.py first")
endif()

# logcat
find_library(log-lib log)
target_link_libraries(erbium PRIVATE ${log-lib})

# The upstream Erbium sources are compiled in a later phase (see docs/ANDROID-PORT.md).
# This skeleton proves the toolchain + entry + module discovery first.
