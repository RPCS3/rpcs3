cmake_minimum_required(VERSION 2.6)

project (YAML_CPP)

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
add_library(yaml-cpp
	${public_headers}
	${private_headers}
	${sources}
)

install(TARGETS yaml-cpp ${_INSTALL_DESTINATIONS})
install(
	FILES ${public_headers}
	DESTINATION ${INCLUDE_INSTALL_DIR}
)

add_subdirectory (yaml-reader)

