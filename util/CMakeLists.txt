add_sources(sandbox.cpp)
add_executable(sandbox sandbox.cpp)
target_link_libraries(sandbox yaml-cpp)

add_sources(read.cpp)
add_executable(read read.cpp)
target_link_libraries(read yaml-cpp)
