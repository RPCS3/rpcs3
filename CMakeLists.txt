cmake_minimum_required(VERSION 2.6)

project (YAML_CPP)

set(LIB_TYPE SHARED)

if(IPHONE)
  set(CMAKE_OSX_SYSROOT iphoneos2.2.1)
  set(LIB_TYPE)
endif(IPHONE)

if(CMAKE_COMPILER_IS_GNUCC)
  set(CMAKE_CXX_FLAGS "-O2 -Wall -pedantic -Wextra")
endif(CMAKE_COMPILER_IS_GNUCC)

set(YAML_CPP_VERSION_MAJOR "0")
set(YAML_CPP_VERSION_MINOR "2")
set(YAML_CPP_VERSION_PATCH "0")
set(YAML_CPP_VERSION "${YAML_CPP_VERSION_MAJOR}.${YAML_CPP_VERSION_MINOR}.${YAML_CPP_VERSION_PATCH}")

enable_testing()

if(WIN32)
  set(_library_dir bin) # .dll are in PATH, like executables
else(WIN32)
  set(_library_dir lib)
endif(WIN32)

set(INCLUDE_INSTALL_DIR include/yaml-cpp)
set(LIB_INSTALL_DIR ${_library_dir}${LIB_SUFFIX})

#
set(_INSTALL_DESTINATIONS
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION ${LIB_INSTALL_DIR}
	ARCHIVE DESTINATION lib${LIB_SUFFIX}
)
#
file(GLOB public_headers include/*.h)
file(GLOB private_headers src/*.h)
file(GLOB sources src/*.cpp)

include_directories(${YAML_CPP_SOURCE_DIR}/include)
add_library(yaml-cpp
	${LIB_TYPE}
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

if(UNIX)
	set(PC_FILE ${CMAKE_BINARY_DIR}/yaml-cpp.pc)
	configure_file("yaml-cpp.pc.cmake" ${PC_FILE} @ONLY)
	install(FILES ${PC_FILE} DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)
endif(UNIX)

add_subdirectory (yaml-reader)
add_subdirectory (util)
