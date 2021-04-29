cmake_minimum_required(VERSION 3.11)

project(uvw)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(picojson
  GIT_REPOSITORY https://github.com/kazuho/picojson.git
)
FetchContent_MakeAvailable(picojson)
FetchContent_GetProperties(picojson
  SOURCE_DIR PICOJSON_SRC
)
add_definitions(-DPICOJSON_USE_INT64)

include_directories(
    include
    ${PICOJSON_SRC}
)

set(UVW_USE_STATIC_LIBRARY OFF)

# tests
enable_testing()
add_subdirectory(tests)
