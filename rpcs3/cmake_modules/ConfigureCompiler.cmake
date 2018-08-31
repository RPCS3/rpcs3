cmake_minimum_required(VERSION 3.8.2)
# Check and configure compiler options for RPCS3

if(CMAKE_COMPILER_IS_GNUCXX)
	# GCC 7.3 or latter is required
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.3)
		message(FATAL_ERROR "RPCS3 requires at least gcc-7.3.")
	endif()

	# Set compiler options here

	# Warnings
	add_compile_options(-Wno-attributes -Wno-enum-compare -Wno-invalid-offsetof)

elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
	# Clang 5.0 or latter is required
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
		message(FATAL_ERROR "RPCS3 requires at least clang-5.0.")
	endif()

	# Set compiler options here

	add_compile_options(-ftemplate-depth=1024)
	if(APPLE)
		add_compile_options(-stdlib=libc++)
	endif()
endif()
