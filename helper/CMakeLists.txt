cmake_minimum_required(VERSION 3.13)
project(OverlayHelper)

# Get all the cpp and h files for the helper
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h" "include/*.h")

# Find gRPC
find_package(gRPC CONFIG REQUIRED)

# Add the overlay helper as an shared library to be compiled
add_definitions(-DOVERLAY_HELPER)
add_library(OverlayHelper SHARED ${SOURCES} $<TARGET_OBJECTS:OverlayShared>)

# Link the overlay helper to the dependencies' libs
target_link_libraries(OverlayHelper PRIVATE gRPC::grpc++)