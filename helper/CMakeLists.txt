cmake_minimum_required(VERSION 3.13)
project(OverlayHelper)

# Get all the cpp and h files for the helper
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h" "include/*.h")

# Find gRPC
find_package(gRPC CONFIG REQUIRED)

# Compile protos object library
get_target_property(OVERLAY_PROTOS_BINARY_DIR OverlayProtos BINARY_DIR)
include_directories(${OVERLAY_PROTOS_BINARY_DIR}/gens)

# Include shared headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../shared)

# Add the overlay helper as an shared library to be compiled
add_definitions(-DOVERLAY_HELPER)
add_library(OverlayHelper SHARED ${SOURCES} $<TARGET_OBJECTS:OverlayProtos>)

# Link the overlay helper to the dependencies' libs
target_link_libraries(OverlayHelper PRIVATE gRPC::grpc++)