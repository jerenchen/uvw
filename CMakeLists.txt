cmake_minimum_required(VERSION 3.1)

project(uvw)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(include)

set(UVW_USE_STATIC_LIBRARY OFF)

add_subdirectory(tests)