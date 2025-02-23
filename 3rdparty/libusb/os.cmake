include(FindThreads)

set(PTHREADS_ENABLED FALSE)
if (CMAKE_USE_PTHREADS_INIT)
	set(PTHREADS_ENABLED TRUE)
endif()

if (CYGWIN)
	add_compile_definitions(PLATFORM_WINDOWS=1)
	set(OS_WINDOWS 1)

	message(STATUS "Detected Cygwin")
	set(PTHREADS_ENABLED TRUE)
	set(WITHOUT_POLL_H TRUE)

	list(APPEND PLATFORM_SRC
		events_windows.c
		windows_usbdk.c
		windows_common.c
		windows_winusb.c
	)

	if (PTHREADS_ENABLED AND NOT WITHOUT_PTHREADS)
		list(APPEND PLATFORM_SRC threads_posix)
	endif()
elseif(WIN32)
	add_compile_definitions(PLATFORM_WINDOWS=1)
	set(OS_WINDOWS 1)
	set(PTHREADS_ENABLED FALSE)

	list(APPEND PLATFORM_SRC
		events_windows.c
		windows_usbdk.c
		windows_common.c
		windows_winusb.c
	)

	list(APPEND PLATFORM_SRC threads_windows.c)
elseif (APPLE)
	# Apple != OSX alone
	add_compile_definitions(PLATFORM_POSIX=1 HAVE_CLOCK_GETTIME)
	set(OS_DARWIN 1)

	if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
		set(PLATFORM_SRC
			darwin_usb.c
			threads_posix.c
			events_posix.c
		)

		find_package(IOKit REQUIRED)
		list(APPEND LIBUSB_LIBRARIES ${IOKit_LIBRARIES})
		list(APPEND LIBUSB_LIBRARIES "-framework Security")

		# Currently only objc_registerThreadWithCollector requires linking against it
		# which is only for MAC_OS_X_VERSION_MIN_REQUIRED >= 1060
		include(CheckCSourceCompiles)
		check_c_source_compiles(
"#include <AvailabilityMacros.h>
int main()
{
#if !(MAC_OS_X_VERSION_MIN_REQUIRED >= 1060)
#error \"Don't need objc\"
#endif
}
" NEED_OBJC_REGISTER_THREAD_WITH_COLLECTOR)

		if (NEED_OBJC_REGISTER_THREAD_WITH_COLLECTOR)
			find_library(LIBOBJC objc)
			if (NOT LIBOBJC)
				message(SEND_ERROR "Need objc library but can't find it")
			else()
				list(APPEND LIBUSB_LIBRARIES ${LIBOBJC})
			endif()
		endif()
	endif()
elseif (UNIX)
	# Unix is for all *NIX systems including OSX
	add_compile_definitions(PLATFORM_POSIX=1 HAVE_CLOCK_GETTIME)
	if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" OR ANDROID)
		set(OS_LINUX 1)

		set(PLATFORM_SRC
			linux_usbfs.c
			linux_netlink.c
			threads_posix.c
			events_posix.c
		)

		if(NOT ANDROID)
			list(APPEND LIBUSB_LIBRARIES rt)
		endif()
	endif()
endif()

if (NOT PLATFORM_SRC)
	message(FATAL_ERROR "Unsupported platform ${CMAKE_SYSTEM_NAME}.  Currently only support Windows, OSX, & Linux.")
endif()

# the paths are relative to this directory but used in the parent directory,
# so we have to adjust the paths
foreach(SRC IN LISTS PLATFORM_SRC)
	list(APPEND LIBUSB_PLATFORM ${LIBUSB_SOURCE_DIR}/libusb/os/${SRC})
endforeach()

# export one level up so that the generic
# libusb parts know what the platform bits are supposed to be
set(LIBUSB_PLATFORM ${LIBUSB_PLATFORM} PARENT_SCOPE)
set(LIBUSB_LIBRARIES ${LIBUSB_LIBRARIES} PARENT_SCOPE)

if (WITHOUT_PTHREADS)
	set(PTHREADS_ENABLED FALSE)
endif()
set(THREADS_POSIX ${PTHREADS_ENABLED})
