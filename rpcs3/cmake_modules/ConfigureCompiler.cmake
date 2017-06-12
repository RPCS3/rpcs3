cmake_minimum_required(VERSION 2.8.12)
# Check and configure compiler options for RPCS3

if(CMAKE_COMPILER_IS_GNUCXX)

	# GCC 4.9 and lower are too old
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9.0)
		message(FATAL_ERROR
			"GCC ${CMAKE_CXX_COMPILER_VERSION} is too old.")
	endif()
	if (CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 6.1)
		message( FATAL_ERROR "RPCS3 can't be compiled with gcc-6.1, see #1691." )
	endif()
	if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.1)
		message( FATAL_ERROR "RPCS3 requires at least gcc-5.1." )
	endif()
	# FIXME: do we really need this?
	# GCC 6.1 is insufficient to compile, because of a regression bug
	#if(GCC_MAJOR EQUAL "6" AND GCC_MINOR EQUAL "1")
	#	message(FATAL_ERROR
	#		"GCC ${CMAKE_C_COMPILER_VERSION} is insufficient to build this project! "
	#		"If you need to compile with GCC, use GCC 5.4 or lower or GCC 6.2 or higher "
	#		"to build the project, or use CLANG instead. "
	#		"See this regression bug: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70869")
	#endif()

	# Set compiler options here

	# Warnings
	add_compile_options(-Wno-attributes -Wno-enum-compare -Wno-invalid-offsetof)

elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
	# Clang 3.4 and lower are too old
	#if(CLANG_VERSION_MAJOR LESS "3" AND CLANG_VERSION_MINOR LESS "4")
	if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.4)
		message(FATAL_ERROR
			"Clang ${CMAKE_CXX_COMPILER_VERSION} is too old.")
	endif()
	# Set compiler options here

	add_compile_options(-ftemplate-depth=1024)
	if(APPLE)
		add_compile_options(-stdlib=libc++)
	endif()
	if(WIN32)
		add_compile_options(-pthread)
	endif()
endif()
