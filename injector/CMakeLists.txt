cmake_minimum_required(VERSION 3.0)
project(OverlayInjector)

# Set C++17 standard
set (CMAKE_CXX_STANDARD 17)

# Get all the cpp and h files for the injector
file(GLOB_RECURSE SOURCES "*.cpp" "*.h")

# Add the overlay injector as an executable to be compiled
add_executable(${PROJECT_NAME} ${SOURCES})
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME}64)
endif()