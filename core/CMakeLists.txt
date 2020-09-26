cmake_minimum_required(VERSION 3.13)
project(OverlayCore)

# Ignore linker warnings
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4098 /ignore:4217")

# Get all the cpp and h files for the library
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")
include_directories(src)

# Find gRPC
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

# Find OpenSSL
find_package(OpenSSL REQUIRED)
message(STATUS "Using OpenSSL ${OPENSSL_VERSION}")

# Compile the MinHook library
find_package(minhook CONFIG REQUIRED)
message(STATUS "Using MinHook ${minhook_VERSION}")

# Compile the loguru library
set(LOGURU_SOURCES "../vendor/loguru/loguru.hpp" "../vendor/loguru/loguru.cpp")
include_directories(../vendor/loguru)

# Include DirectX SDK headers
include_directories(../vendor/dxsdk/Include)

# Add the overlay core as an shared library to be compiled
add_library(${PROJECT_NAME} SHARED ${SOURCES} $<TARGET_OBJECTS:OverlayShared> ${LOGURU_SOURCES})
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME}64)
endif()

# Link the overlay core to the dependencies' libs
target_link_libraries(${PROJECT_NAME} PRIVATE rpcrt4.lib gRPC::grpc++ OpenSSL::SSL OpenSSL::Crypto minhook::minhook)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_link_libraries(${PROJECT_NAME} PRIVATE msvcrtd.lib)
else ()
	target_link_libraries(${PROJECT_NAME} PRIVATE msvcrt.lib)
endif ()