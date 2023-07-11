include(os.cmake)

include(config.cmake)
include(FindThreads)

include_directories(${LIBUSB_SOURCE_DIR}/libusb)
include_directories(${LIBUSB_SOURCE_DIR}/libusb/os)

if (CMAKE_THREAD_LIBS_INIT)
	list(APPEND LIBUSB_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})
endif()

add_library(usb-1.0-static
	STATIC
	libusb/libusb/core.c
	libusb/libusb/descriptor.c
	libusb/libusb/io.c
	libusb/libusb/sync.c
	libusb/libusb/hotplug.c
	libusb/libusb/strerror.c
	libusb/libusb/libusb-1.0.rc
	libusb/libusb/libusb-1.0.def
	${LIBUSB_PLATFORM}
)

target_include_directories(usb-1.0-static PUBLIC $<BUILD_INTERFACE:${LIBUSB_SOURCE_DIR}/libusb>)

set_target_properties(usb-1.0-static PROPERTIES
	PREFIX "lib"
	OUTPUT_NAME "usb-1.0"
	CLEAN_DIRECT_OUTPUT 1
	PUBLIC_HEADER libusb.h
	VERSION "${LIBUSB_MAJOR}.${LIBUSB_MINOR}.${LIBUSB_MICRO}"
	SOVERSION "${LIBUSB_MAJOR}.${LIBUSB_MINOR}.${LIBUSB_MICRO}"
)

if (DEFINED LIBUSB_LIBRARIES)
	target_link_libraries(usb-1.0-static
		${LIBUSB_LIBRARIES}
	)
endif()

list(APPEND LIBUSB_LIBTARGETS usb-1.0-static)

install(TARGETS ${LIBUSB_LIBTARGETS} EXPORT libusb-1
	PUBLIC_HEADER DESTINATION include/libusb-1.0
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION lib
)
install(EXPORT libusb-1 DESTINATION lib/libusb)

foreach(LIB IN LISTS LIBUSB_LIBRARIES)
	if (LIB MATCHES .framework$)
		get_filename_component(LIB "${LIB}" NAME)
		set(LIB "-Wl,-framework,${LIB}")
	elseif (LIB MATCHES .dylib$)
		get_filename_component(LIBDIR "${LIB}" PATH)
		get_filename_component(LIB "${LIB}" NAME)
		string(REGEX REPLACE "lib(.*).dylib$" "\\1" LIB "${LIB}")
		set(LIB "-L${LIBDIR} -l${LIB}")
	endif()
	set(LIBUSB_LIB_DEPENDS "${LIBUSB_LIB_DEPENDS} ${LIB}")
endforeach()

configure_file(libusb-1.0.pc.cmake "${CMAKE_CURRENT_BINARY_DIR}/libusb-1.0.pc" @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libusb-1.0.pc" DESTINATION lib/pkgconfig)
