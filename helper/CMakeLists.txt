cmake_minimum_required(VERSION 3.13)
project(OverlayHelper)

# Get all the cpp and h files for the helper
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h" "include/*.h")
include_directories(src include)

# Find gRPC
find_package(gRPC CONFIG REQUIRED)

# Find magic_enum
find_package(magic_enum CONFIG REQUIRED)

# Add the overlay helper as an shared library to be compiled
add_definitions(-DOVERLAY_HELPER_MAKEDLL)
add_library(OverlayHelper SHARED ${SOURCES} $<TARGET_OBJECTS:OverlayShared>)

# Link the overlay helper to the dependencies' libs
target_link_libraries(${PROJECT_NAME} PRIVATE rpcrt4.lib gRPC::grpc++ magic_enum::magic_enum)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME}64)
endif()

# Output the lib in the bin folder
set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})