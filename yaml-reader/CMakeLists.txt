set(FILES main.cpp tests.cpp)
include_directories(${YAML_CPP_SOURCE_DIR}/include)
link_directories(${YAML_CPP_BINARY_DIR}/bin)

add_executable(yaml-reader ${FILES})
target_link_libraries(yaml-reader yaml-cpp)
