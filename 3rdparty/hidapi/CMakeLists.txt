# hidapi
add_library(3rdparty_hidapi INTERFACE)
target_include_directories(3rdparty_hidapi INTERFACE hidapi/hidapi)

if(APPLE)
	add_subdirectory(hidapi/mac EXCLUDE_FROM_ALL)
	target_include_directories(hidapi-mac PUBLIC hidapi/hidapi)

	target_link_libraries(3rdparty_hidapi INTERFACE hidapi-mac "-framework CoreFoundation" "-framework IOKit")
elseif(CMAKE_SYSTEM MATCHES "Linux")
	add_subdirectory(hidapi/linux EXCLUDE_FROM_ALL)
	target_include_directories(hidapi-hidraw PUBLIC hidapi/hidapi)

	target_link_libraries(3rdparty_hidapi INTERFACE hidapi-hidraw udev)
elseif(WIN32)
	add_subdirectory(hidapi/windows EXCLUDE_FROM_ALL)
	target_include_directories(hidapi-hid PUBLIC hidapi/hidapi)

	target_link_libraries(3rdparty_hidapi INTERFACE hidapi-hid Shlwapi.lib)
else()
	add_subdirectory(hidapi/libusb EXCLUDE_FROM_ALL)
	target_include_directories(hidapi-libusb PUBLIC hidapi/hidapi)

	target_link_libraries(3rdparty_hidapi INTERFACE hidapi-libusb usb)
endif()
