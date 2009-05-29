cmake_minimum_required(VERSION 2.6)

project (YAML_CPP)
set(LIBRARY_OUTPUT_PATH lib${LIB_SUFFIX})
set(LIB_INSTALL_DIR lib${LIB_SUFFIX})
set(INCLUDE_INSTALL_DIR include/yaml-cpp)

file(GLOB public_headers include/*.h)
file(GLOB private_headers src/*.h)
file(GLOB sources src/*.cpp)

include_directories(${YAML_CPP_SOURCE_DIR}/include)
add_library(yaml-cpp
	${public_headers}
	${private_headers}
	${sources}
)

install(
	TARGETS yaml-cpp
	LIBRARY ARCHIVE
	DESTINATION ${LIB_INSTALL_DIR}
)
install(
	FILES ${public_headers}
	DESTINATION ${INCLUDE_INSTALL_DIR}
)

add_subdirectory (yaml-reader)

