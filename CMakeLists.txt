cmake_minimum_required(VERSION 3.10)
project(BitonicSortMPI C)

# Find MPI
find_package(MPI REQUIRED)

# Include directories
include_directories(
    ${MPI_C_INCLUDE_PATH}
    ${CMAKE_SOURCE_DIR}/include
)

# Source files
file(GLOB SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.c
)

# Executable
add_executable(main ${SOURCES})

# Link MPI
target_link_libraries(main ${MPI_C_LIBRARIES} m)

# Optional: Show message
message(STATUS "MPI Include Path: ${MPI_C_INCLUDE_PATH}")
message(STATUS "MPI Libraries: ${MPI_C_LIBRARIES}")
