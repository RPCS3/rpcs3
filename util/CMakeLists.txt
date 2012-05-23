add_executable(parse parse.cpp)
target_link_libraries(parse yaml-cpp)

add_executable(sandbox sandbox.cpp)
target_link_libraries(sandbox yaml-cpp)
