cmake_minimum_required(VERSION 3.13)
project(OverlayCore)

# Ignore linker warnings
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4098 /ignore:4217")

# Get all the cpp and h files for the library
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h" "resources/*.rc" "resources/*.h")
include_directories(src resources)

# Find gRPC
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC v${gRPC_VERSION}")

# Find OpenSSL
find_package(OpenSSL REQUIRED)
message(STATUS "Using OpenSSL v${OPENSSL_VERSION}")

# Find minhook
find_package(minhook CONFIG REQUIRED)
message(STATUS "Using MinHook v${minhook_VERSION}")

# Find magic_enum
find_package(magic_enum CONFIG REQUIRED)
message(STATUS "Using magic_enum v${magic_enum_VERSION}")

# Find loguru
find_path(LOGURU_INCLUDE_DIRS "loguru/loguru.cpp")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_definitions(-DLOGURU_DEBUG_LOGGING)
endif()

# Include DirectX SDK headers
include_directories(../vendor/dxsdk/Include)

# Add the overlay core as an shared library to be compiled
add_library(${PROJECT_NAME} SHARED ${SOURCES} $<TARGET_OBJECTS:OverlayShared> ${LOGURU_SOURCES})
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME}64)
endif()

# Link the overlay core to the dependencies' libs
target_link_libraries(${PROJECT_NAME} PRIVATE rpcrt4.lib Comctl32.lib gRPC::grpc++ OpenSSL::SSL OpenSSL::Crypto minhook::minhook magic_enum::magic_enum)
target_include_directories(${PROJECT_NAME} PRIVATE ${LOGURU_INCLUDE_DIRS})
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_link_libraries(${PROJECT_NAME} PRIVATE msvcrtd.lib)
else ()
	target_link_libraries(${PROJECT_NAME} PRIVATE msvcrt.lib)
endif ()