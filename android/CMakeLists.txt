cmake_minimum_required(VERSION 3.22)
project(erbium-android LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Silence some warnings for the upstream code we compile unmodified later
add_compile_options(-Wno-unused-parameter -Wno-unused-variable -Wno-unused-function)

add_library(erbium SHARED
    src/entry.cpp
    src/platform.cpp
)

target_include_directories(erbium PRIVATE src)

# logcat
find_library(log-lib log)
target_link_libraries(erbium PRIVATE ${log-lib})

# The upstream Erbium sources are compiled in a later phase (see docs/ANDROID-PORT.md).
# This skeleton proves the toolchain + entry + module discovery first.
