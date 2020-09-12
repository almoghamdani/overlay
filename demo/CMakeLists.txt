cmake_minimum_required(VERSION 3.0)
project(OverlayDemo)

# Debug
add_definitions (-DDEBUG)

# Set C++17 standard
set (CMAKE_CXX_STANDARD 17)

# Get all the cpp and h files for the demo
file(GLOB_RECURSE SOURCES "*.cpp" "*.h")

# Include overlay helper
include_directories(../helper/include)

# Include cxxopts
include_directories(../vendor/cxxopts/include)

# Add the overlay demo as an executable to be compiled
add_executable(OverlayDemo ${SOURCES})

# Link the demo to the helper lib
target_link_libraries(OverlayDemo OverlayHelper)