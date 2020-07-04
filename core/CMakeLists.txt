cmake_minimum_required(VERSION 3.0)
project(OverlayCore)

# Debug
add_definitions (-DDEBUG)

# Set C++17 standard
set (CMAKE_CXX_STANDARD 17)

# Get all the cpp and h files for the library
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

# Compile the subhook library
set(SUBHOOK_STATIC ON)
set(SUBHOOK_INSTALL OFF)
set(SUBHOOK_TESTS OFF)
add_definitions(-DSUBHOOK_STATIC)
add_subdirectory(../vendor/subhook subhook)
include_directories(../vendor/subhook)

# Compile the loguru library
set(LOGURU_SOURCES "../vendor/loguru/loguru.hpp" "../vendor/loguru/loguru.cpp")
include_directories(../vendor/loguru)

# Ignore deprecations
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

# Add the overlay core as an executable to be compiled
add_library(OverlayCore SHARED ${SOURCES} ${LOGURU_SOURCES})

# Link the overlay core to the dependencies' libs
target_link_libraries(OverlayCore subhook)