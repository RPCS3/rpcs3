cmake_minimum_required(VERSION 2.6)

project (YAML_CPP)
set(LIBRARY_OUTPUT_PATH ${YAML_CPP_SOURCE_DIR}/lib)

file(GLOB public_headers include/*.h)
file(GLOB private_headers src/*.h)
file(GLOB sources src/*.cpp)

include_directories(${YAML_CPP_SOURCE_DIR}/include)
add_library(yaml-cpp
	${public_headers}
	${private_headers}
	${sources}
)

add_subdirectory (yaml-reader)

