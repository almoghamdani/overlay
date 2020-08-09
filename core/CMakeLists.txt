cmake_minimum_required(VERSION 3.13)
project(OverlayCore)

# Ignore linker warnings
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4098 /ignore:4217")

# Get all the cpp and h files for the library
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

# Find gRPC
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

# Find OpenSSL
find_package(OpenSSL REQUIRED)
message(STATUS "Using OpenSSL ${OPENSSL_VERSION}")

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

# Add the overlay core as an shared library to be compiled
add_library(OverlayCore SHARED ${SOURCES} $<TARGET_OBJECTS:OverlayShared> ${LOGURU_SOURCES})

# Link the overlay core to the dependencies' libs
target_link_libraries(OverlayCore PRIVATE rpcrt4.lib subhook gRPC::grpc++ OpenSSL::SSL OpenSSL::Crypto)