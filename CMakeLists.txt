cmake_minimum_required(VERSION 2.6)

project (YAML_CPP)

set(LIB_TYPE SHARED)

if(IPHONE)
  set(CMAKE_OSX_SYSROOT iphoneos2.2.1)
  set(LIB_TYPE)
endif(IPHONE)

if(CMAKE_COMPILER_IS_GNUCC)
  set(CMAKE_CXX_FLAGS "-O2 -Wall -Wextra -pedantic -Wno-long-long ${CMAKE_CXX_FLAGS}")
endif(CMAKE_COMPILER_IS_GNUCC)

if(MSVC)
  set(LIB_TYPE) # I can't figure out how CMake handles Windows shared libraries
  set(CMAKE_CXX_FLAGS "/W3 /wd4127 /wd4355 /D_SCL_SECURE_NO_WARNINGS ${CMAKE_CXX_FLAGS}")
endif(MSVC)

set(YAML_CPP_VERSION_MAJOR "0")
set(YAML_CPP_VERSION_MINOR "2")
set(YAML_CPP_VERSION_PATCH "5")
set(YAML_CPP_VERSION "${YAML_CPP_VERSION_MAJOR}.${YAML_CPP_VERSION_MINOR}.${YAML_CPP_VERSION_PATCH}")

enable_testing()

option(YAML_CPP_BUILD_TOOLS "Enables or disables testing and parse tools" true)

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
file(GLOB public_headers include/[a-z]*.h)
file(GLOB private_headers src/[a-z]*.h)
file(GLOB sources src/[a-z]*.cpp)

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

if(YAML_CPP_BUILD_TOOLS)
	add_subdirectory (test)
	add_subdirectory (util)
endif(YAML_CPP_BUILD_TOOLS)
