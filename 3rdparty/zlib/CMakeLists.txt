# ZLib
if (USE_SYSTEM_ZLIB)
	find_package(ZLIB QUIET)
endif()

if (NOT ZLIB_FOUND)
	message(STATUS "Using builtin ZLIB")
	set(SKIP_INSTALL_ALL ON)
	add_subdirectory(zlib EXCLUDE_FROM_ALL)
	set(ZLIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/zlib" "${CMAKE_CURRENT_BINARY_DIR}/zlib")
	set(ZLIB_LIBRARY zlibstatic)
endif()

add_library(3rdparty_zlib INTERFACE)
target_link_libraries(3rdparty_zlib INTERFACE ${ZLIB_LIBRARY})
target_include_directories(3rdparty_zlib INTERFACE ${ZLIB_INCLUDE_DIR})