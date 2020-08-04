cmake_minimum_required(VERSION 3.0)
project(OverlayCore)

# Debug
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_definitions(-DDEBUG)
endif()

# Ignore linker warnings
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4098 /ignore:4217")

# Set C++17 standard
set (CMAKE_CXX_STANDARD 17)

# Get all the cpp and h files for the library
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")
file(GLOB_RECURSE PROTOS "src/*.proto")

# Set runtime library if compiling against gRPC and protobuf
if(PROTOS)
	set(CMAKE_CXX_FLAGS_RELEASE "/MT")
	set(CMAKE_CXX_FLAGS_DEBUG "/MTd")
endif()

# Find Protobuf
set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${protobuf_VERSION}")

# Find gRPC
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

# Compile protos
set(PROTO_SRCS "")
set(GRPC_SRCS "")
foreach(PROTO_FILE ${PROTOS})
	get_filename_component(PROTO_FILE_RAW_NAME ${PROTO_FILE} NAME_WE)
	get_filename_component(PROTO_FILE_DIR ${PROTO_FILE} DIRECTORY)
	set(PROTO_SRC "${CMAKE_CURRENT_BINARY_DIR}/gens/${PROTO_FILE_RAW_NAME}.pb.cc")
	set(PROTO_HDR "${CMAKE_CURRENT_BINARY_DIR}/gens/${PROTO_FILE_RAW_NAME}.pb.h")
	set(GRPC_SRC "${CMAKE_CURRENT_BINARY_DIR}/gens/${PROTO_FILE_RAW_NAME}.grpc.pb.cc")
	set(GRPC_HDR "${CMAKE_CURRENT_BINARY_DIR}/gens/${PROTO_FILE_RAW_NAME}.grpc.pb.h")
	list(APPEND PROTO_SRCS ${PROTO_SRC})
	list(APPEND GRPC_SRCS ${GRPC_SRC})
	add_custom_command(
      OUTPUT "${PROTO_SRC}" "${PROTO_HDR}" "${GRPC_SRC}" "${GRPC_HDR}"
      COMMAND $<TARGET_FILE:protobuf::protoc>
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}/gens"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}/gens"
        -I "${PROTO_FILE_DIR}"
        --plugin=protoc-gen-grpc="$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
        "${PROTO_FILE}"
	  DEPENDS "${PROTO_FILE}")
endforeach()
set_source_files_properties(${PROTO_SRCS} ${GRPC_SRCS} PROPERTIES COMPILE_FLAGS "/W0") # Ignore all warnings for protobuf generated files
include_directories(${CMAKE_CURRENT_BINARY_DIR}/gens)

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
add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)

# Set windows version to fix gRPC issue
add_definitions(-D_WIN32_WINNT=0xA00)

# Add the overlay core as an executable to be compiled
add_library(OverlayCore SHARED ${SOURCES} ${PROTO_SRCS} ${GRPC_SRCS} ${LOGURU_SOURCES})

# Link the overlay core to the dependencies' libs
target_link_libraries(OverlayCore PRIVATE subhook gRPC::grpc++)