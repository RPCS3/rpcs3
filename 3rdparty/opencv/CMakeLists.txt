# OpenCV

set(OPENCV_TARGET 3rdparty_dummy_lib PARENT_SCOPE)

if (USE_SYSTEM_OPENCV)
	message(STATUS "RPCS3: using system OpenCV")
	find_package(OpenCV COMPONENTS core photo)
	
	if(OPENCV_FOUND)
		message(STATUS "RPCS3: found system OpenCV")
		include_directories(${OpenCV_INCLUDE_DIRS})
		add_library(3rdparty_opencv INTERFACE)
		target_link_libraries(3rdparty_opencv INTERFACE ${OpenCV_LIBS})
		target_compile_definitions(3rdparty_opencv INTERFACE -DHAVE_OPENCV)
		set(OPENCV_TARGET 3rdparty_opencv PARENT_SCOPE)
	else()
		message(WARNING "RPCS3: OpenCV not found. Building without OpenCV support")
	endif()
else()
	message(WARNING "RPCS3: Building without OpenCV support")
endif()
