cmake_minimum_required(VERSION 3.20)
project(queens_solver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to an optimised build so benchmark timings are meaningful.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(queens src/queens.cpp)
target_compile_options(queens PRIVATE -Wall -Wextra)
