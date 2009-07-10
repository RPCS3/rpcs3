cmake_minimum_required(VERSION 2.6)

project (YAML_CPP)

SET(CMAKE_CXX_FLAGS "-O2 -Wall -pedantic -Wextra")

set(YAML_CPP_VERSION_MAJOR "0")
set(YAML_CPP_VERSION_MINOR "1")
set(YAML_CPP_VERSION_PATCH "0")
set(YAML_CPP_VERSION "${YAML_CPP_VERSION_MAJOR}.${YAML_CPP_VERSION_MINOR}.${YAML_CPP_VERSION_PATCH}")

enable_testing()

if(WIN32)
  set(_library_dir bin) # .dll are in PATH, like executables
else(WIN32)
  set(_library_dir lib)
endif(WIN32)
#
set(_INSTALL_DESTINATIONS
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION ${_library_dir}${LIB_SUFFIX} 
	ARCHIVE DESTINATION lib${LIB_SUFFIX}
)
#
set(INCLUDE_INSTALL_DIR include/yaml-cpp)

file(GLOB public_headers include/*.h)
file(GLOB private_headers src/*.h)
file(GLOB sources src/*.cpp)

include_directories(${YAML_CPP_SOURCE_DIR}/include)
add_library(yaml-cpp SHARED
	${public_headers}
	${private_headers}
	${sources}
)
set_target_properties(yaml-cpp PROPERTIES
	VERSION "${YAML_CPP_VERSION}"
	SOVERSION "${YAML_CPP_VERSION_MAJOR}.${YAML_CPP_VERSION_MINOR}"
)

install(TARGETS yaml-cpp ${_INSTALL_DESTINATIONS})
install(
	FILES ${public_headers}
	DESTINATION ${INCLUDE_INSTALL_DIR}
)

add_subdirectory (yaml-reader)
add_subdirectory (util)
