cmake_minimum_required(VERSION 3.10)
project(BitonicSortMPI C)

# Require C11 standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find MPI package (required)
find_package(MPI REQUIRED)

# Source files
file(GLOB SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.c
)

# Add executable targets
add_executable(bitonic_sort ${SOURCES})

# Include directories for targets
target_include_directories(bitonic_sort PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Link MPI and math libraries
target_link_libraries(bitonic_sort PRIVATE MPI::MPI_C m)

# Optional status messages
message(STATUS "MPI include dirs: ${MPI_C_INCLUDE_PATH}")
message(STATUS "MPI libs: ${MPI_C_LIBRARIES}")
