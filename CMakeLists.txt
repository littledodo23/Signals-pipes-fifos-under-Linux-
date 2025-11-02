cmake_minimum_required(VERSION 3.10)
project(RealTimeProject1 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -fopenmp")

# Add executable with all source files
add_executable(matrix_ops
        main.c
        matrix.c
        file_io.c
        worker_pool.c)

# Link math library and OpenMP
target_link_libraries(matrix_ops m)

# Find and link OpenMP
find_package(OpenMP)
if(OpenMP_C_FOUND)
    target_link_libraries(matrix_ops OpenMP::OpenMP_C)
endif()
