# Check and configure compiler options for RPCS3

if(CMAKE_COMPILER_IS_GNUCXX)
	# Get GCC version
	execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
					OUTPUT_VARIABLE GCC_VERSION)
	string(REGEX MATCHALL "[0-9]+" GCC_VERSION_COMPONENTS ${GCC_VERSION})
	list(GET GCC_VERSION_COMPONENTS 0 GCC_MAJOR)
	list(GET GCC_VERSION_COMPONENTS 1 GCC_MINOR)

	# GCC 4.9 and lower are too old
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9.0)
		message(FATAL_ERROR
			"GCC ${CMAKE_CXX_COMPILER_VERSION} is too old.")
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
	if(CLANG_VERSION_MAJOR LESS "3" AND CLANG_VERSION_MINOR LESS "4")
		message(FATAL_ERROR
			"Clang ${CLANG_VERSION_MAJOR}.${CLANG_VERSION_MINOR} is too old.")
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
