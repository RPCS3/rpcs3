add_executable(parse parse.cpp)
target_link_libraries(parse yaml-cpp)

add_executable(value value.cpp)
target_link_libraries(value yaml-cpp)
