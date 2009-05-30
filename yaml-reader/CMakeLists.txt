set(yaml-reader_sources main.cpp tests.cpp)

add_executable(yaml-reader ${yaml-reader_sources})
target_link_libraries(yaml-reader yaml-cpp)

add_test(yaml-reader-test yaml-reader)
