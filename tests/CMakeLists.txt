project(uvw_test)

if(UVW_USE_STATIC_LIBRARY)
    add_definitions(-DUVW_BUILD_STATIC)
endif()

add_executable(uvw_test main.cpp)
target_link_libraries(uvw_test)
install(TARGETS uvw_test DESTINATION .)