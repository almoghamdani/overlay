cmake_minimum_required(VERSION 3.0)
project(OverlayHelper)

# Debug
add_definitions (-DDEBUG)

# Set C++17 standard
set (CMAKE_CXX_STANDARD 17)

# Get all the cpp and h files for the launcher
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h" "include/*.h")

# Add the overlay launcher as an executable to be compiled
add_definitions(-DOVERLAY_HELPER)
add_library(OverlayHelper SHARED ${SOURCES})