project(uvw_test)

add_executable(uvw_test main.cpp)
target_link_libraries(uvw_test)
install(TARGETS uvw_test DESTINATION .)