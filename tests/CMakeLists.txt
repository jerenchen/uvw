project(uvw_tests)

FetchContent_Declare(Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v2.x
)
FetchContent_MakeAvailable(Catch2)
FetchContent_GetProperties(Catch2
  SOURCE_DIR CATCH2_SOURCE
)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
    ${CATCH2_SOURCE}/Contrib
)

# exclude implementation in headers
add_definitions(
  -DUVW_BUILD_STATIC
)

file(GLOB UVW_TESTS uvw/*.cpp)

add_executable(uvw_tests ${UVW_TESTS} ../include/uvw.cpp)
target_link_libraries(uvw_tests PRIVATE Catch2::Catch2)

include(CTest)
include(Catch)
catch_discover_tests(uvw_tests)
