cmake_minimum_required(VERSION 3.1)

project(uvw)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(
  DOWNLOAD
    https://github.com/nlohmann/json/releases/download/v3.9.1/json.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/include/json.hpp
  TIMEOUT 60
)

include_directories(
    include
    ${CMAKE_CURRENT_BINARY_DIR}/include
)

set(UVW_USE_STATIC_LIBRARY OFF)

add_subdirectory(tests)