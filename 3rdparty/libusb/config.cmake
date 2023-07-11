include(CheckCXXCompilerFlag)
include(CheckIncludeFiles)
include(CheckTypeSize)

if (${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
	if (NOT OS_WINDOWS)
		# mingw appears to print a bunch of warnings about this
		check_cxx_compiler_flag("-fvisibility=hidden" HAVE_VISIBILITY)
	endif()
	check_cxx_compiler_flag("-Wno-pointer-sign" HAVE_WARN_NO_POINTER_SIGN)

	set(_GNU_SOURCE 1 CACHE INTERNAL "" FORCE)

	unset(ADDITIONAL_CC_FLAGS)

	if (HAVE_VISIBILITY)
		list(APPEND ADDITIONAL_CC_FLAGS -fvisibility=hidden)
	endif()

	if (HAVE_WARN_NO_POINTER_SIGN)
		list(APPEND ADDITIONAL_CC_FLAGS -Wno-pointer-sign)
	endif()

	append_compiler_flags(
		-std=gnu99
		-Wall
		-Wundef
		-Wunused
		-Wstrict-prototypes
		-Werror-implicit-function-declaration
		-Wshadow
		${ADDITIONAL_CC_FLAGS}
	)
elseif(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

check_include_files(sys/timerfd.h USBI_TIMERFD_AVAILABLE)
#check_type_size(struct timespec STRUCT_TIMESPEC)

if (HAVE_VISIBILITY)
	set(DEFAULT_VISIBILITY "__attribute__((visibility(\"default\")))" CACHE INTERNAL "visibility attribute to function decl" FORCE)
else()
	set(DEFAULT_VISIBILITY "" CACHE INTERNAL "visibility attribute to function decl" FORCE)
endif()

if (NOT WITHOUT_POLL_H)
	check_include_files(poll.h HAVE_POLL_H)
else()
	set(HAVE_POLL_H FALSE CACHE INTERNAL "poll.h explicitely disabled" FORCE)
endif()

if (HAVE_POLL_H)
	list(APPEND CMAKE_EXTRA_INCLUDE_FILES "poll.h")
	check_type_size(nfds_t NFDS_T)
	unset(CMAKE_EXTRA_INCLUDE_FILES)
else()
	set(HAVE_NFDS_T FALSE CACHE INTERNAL "poll.h not found - assuming no nfds_t (windows)" FORCE)
	set(NFDS_T "" CACHE INTERNAL "" FORCE)
endif()

if (HAVE_NFDS_T)
	set(POLL_NFDS_TYPE nfds_t CACHE INTERNAL "the poll nfds types for this platform" FORCE)
else()
	set(POLL_NFDS_TYPE "unsigned int" CACHE INTERNAL "the poll nfds for this platform" FORCE)
endif()

if (OS_WINDOWS)
	macro(copy_header_if_missing HEADER VARIABLE ALTERNATIVE_DIR)
		check_include_files(${HEADER} ${VARIABLE})
		if (NOT ${VARIABLE})
			message(STATUS "Missing ${HEADER} - grabbing from ${ALTERNATIVE_DIR}")
			file(COPY "${ALTERNATIVE_DIR}/${HEADER}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/")
		endif()
	endmacro()

	# Only VS 2010 has stdint.h
	copy_header_if_missing(stdint.h HAVE_STDINT_H ../msvc)
	copy_header_if_missing(inttypes.h HAVE_INTTYPES_H ../msvc)
endif()

set(ENABLE_DEBUG_LOGGING ${WITH_DEBUG_LOG} CACHE INTERNAL "enable debug logging (WITH_DEBUG_LOGGING)" FORCE)
set(ENABLE_LOGGING ${WITH_LOGGING} CACHE INTERNAL "enable logging (WITH_LOGGING)" FORCE)
set(PACKAGE "libusb" CACHE INTERNAL "The package name" FORCE)
set(PACKAGE_BUGREPORT "libusb-devel@lists.sourceforge.net" CACHE INTERNAL "Where to send bug reports" FORCE)
set(PACKAGE_VERSION "${LIBUSB_MAJOR}.${LIBUSB_MINOR}.${LIBUSB_MICRO}" CACHE INTERNAL "package version" FORCE)
set(PACKAGE_STRING "${PACKAGE} ${PACKAGE_VERSION}" CACHE INTERNAL "package string" FORCE)
set(PACKAGE_URL "http://www.libusb.org" CACHE INTERNAL "package url" FORCE)
set(PACKAGE_TARNAME "libusb" CACHE INTERNAL "tarball name" FORCE)
set(VERSION "${PACKAGE_VERSION}" CACHE INTERNAL "version" FORCE)

if(MSVC)
	file(COPY libusb/msvc/config.h DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
else()
	configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h @ONLY)
endif()
message(STATUS "Generated configuration file in ${CMAKE_CURRENT_BINARY_DIR}/config.h")

# for generated config.h
include_directories(${CMAKE_CURRENT_BINARY_DIR})
