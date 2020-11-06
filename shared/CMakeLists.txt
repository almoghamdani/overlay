cmake_minimum_required(VERSION 3.13)
project(OverlayShared)

# Get all the src and proto files for the project
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")
file(GLOB_RECURSE PROTOS "protos/*.proto")
include_directories(src)

# Find Protobuf
set(Protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")

# Find gRPC
find_package(gRPC CONFIG REQUIRED)

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
	list(APPEND PROTO_SRCS ${PROTO_SRC} ${PROTO_HDR})
	list(APPEND GRPC_SRCS ${GRPC_SRC} ${GRPC_HDR})
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
set_source_files_properties(${GRPC_SRCS} PROPERTIES COMPILE_FLAGS "/W0") # Ignore all warnings for gRPC protobuf generated files

# Add the overlay shared as an object library
add_library(${PROJECT_NAME} OBJECT ${SOURCES} ${PROTO_SRCS} ${GRPC_SRCS})

# Link it to gRPC
target_link_libraries(${PROJECT_NAME} PRIVATE gRPC::grpc++)