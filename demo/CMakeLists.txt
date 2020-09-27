cmake_minimum_required(VERSION 3.0)
project(OverlayDemo)

# Get all the cpp and h files for the demo
file(GLOB_RECURSE SOURCES "*.cpp" "*.h")

# Include overlay helper
include_directories(../helper/include)

# Include cxxopts
include_directories(../vendor/cxxopts/include)

# Add the overlay demo as an executable to be compiled
add_executable(${PROJECT_NAME} ${SOURCES})
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME}64)
endif()

# Link the demo to the helper lib
target_link_libraries(${PROJECT_NAME} OverlayHelper)