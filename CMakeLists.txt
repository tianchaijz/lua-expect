cmake_minimum_required (VERSION 2.6)
project (lua-expect C)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin")
set(LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}/lib")

add_subdirectory(src)
add_subdirectory(tests)
