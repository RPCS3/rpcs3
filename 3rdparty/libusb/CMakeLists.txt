project(libusb)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules")
set(LIBUSB_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libusb)


option(WITH_DEBUG_LOG "enable debug logging" OFF)
# if debug logging is enabled, by default enable logging
option(WITH_LOGGING "if false, disable all logging" ON)
option(WITHOUT_PTHREADS "force pthreads to not be used.  if on, then they are used based on detection logic" OFF)

set(LIBUSB_MAJOR 1)
set(LIBUSB_MINOR 0)
set(LIBUSB_MICRO 26)

macro(append_compiler_flags)
	foreach(FLAG IN ITEMS ${ARGN})
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAG}")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}")
	endforeach()
endmacro()

include(libusb.cmake)
