# libPNG
# Select the version of libpng to use, default is builtin
if (NOT USE_SYSTEM_LIBPNG)
	# We use libpng's static library and don't need to build the shared library and run the tests
	set(PNG_SHARED OFF CACHE BOOL "Build shared lib" FORCE)
	set(PNG_TESTS OFF CACHE BOOL "Build libpng tests" FORCE)
	set(PNG_BUILD_ZLIB ON CACHE BOOL "ZLIB is already build or package is found" FORCE)
	set(SKIP_INSTALL_ALL ON)
	add_subdirectory(libpng EXCLUDE_FROM_ALL)
	target_include_directories(png_static INTERFACE "${libpng_BINARY_DIR}" "${libpng_SOURCE_DIR}")

	set(LIBPNG_TARGET png_static PARENT_SCOPE)	
else()
	find_package(PNG REQUIRED)

	add_library(3rdparty_system_libpng INTERFACE)
	target_include_directories(3rdparty_system_libpng INTERFACE ${PNG_INCLUDE_DIR})
	target_link_libraries(3rdparty_system_libpng INTERFACE ${PNG_LIBRARY})
	target_compile_definitions(3rdparty_system_libpng INTERFACE ${PNG_DEFINITIONS})

	set(LIBPNG_TARGET 3rdparty_system_libpng PARENT_SCOPE)
endif()
