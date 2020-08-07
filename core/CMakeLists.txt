cmake_minimum_required(VERSION 3.13)
project(OverlayCore)

# Ignore linker warnings
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4098 /ignore:4217")

# Get all the cpp and h files for the library
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

# Find gRPC
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

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

# Compile protos object library
get_target_property(OVERLAY_PROTOS_BINARY_DIR OverlayProtos BINARY_DIR)
include_directories(${OVERLAY_PROTOS_BINARY_DIR}/gens)

# Include shared headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../shared)

# Add the overlay core as an shared library to be compiled
add_library(OverlayCore SHARED ${SOURCES} $<TARGET_OBJECTS:OverlayProtos> ${LOGURU_SOURCES})

# Link the overlay core to the dependencies' libs
target_link_libraries(OverlayCore PRIVATE rpcrt4.lib subhook gRPC::grpc++)