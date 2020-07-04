cmake_minimum_required(VERSION 3.0)
project(Overlay)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(core OverlayCore)
add_subdirectory(helper OverlayHelper)
add_subdirectory(demo OverlayDemo)