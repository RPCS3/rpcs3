set(yaml-reader_sources main.cpp tests.cpp)
include_directories(${YAML_CPP_SOURCE_DIR}/include)
link_directories(${YAML_CPP_SOURCE_DIR}/lib)

add_executable(yaml-reader ${yaml-reader_sources})
target_link_libraries(yaml-reader yaml-cpp)
