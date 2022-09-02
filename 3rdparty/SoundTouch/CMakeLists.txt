add_library(soundtouch STATIC EXCLUDE_FROM_ALL
	soundtouch/source/SoundTouch/AAFilter.cpp
	soundtouch/source/SoundTouch/FIFOSampleBuffer.cpp
	soundtouch/source/SoundTouch/FIRFilter.cpp
	soundtouch/source/SoundTouch/InterpolateCubic.cpp
	soundtouch/source/SoundTouch/InterpolateLinear.cpp
	soundtouch/source/SoundTouch/InterpolateShannon.cpp
	soundtouch/source/SoundTouch/RateTransposer.cpp
	soundtouch/source/SoundTouch/SoundTouch.cpp
	soundtouch/source/SoundTouch/sse_optimized.cpp
	soundtouch/source/SoundTouch/TDStretch.cpp
)

target_include_directories(soundtouch PRIVATE
	soundtouch/source/SoundTouch
	soundtouch/include)

target_include_directories(soundtouch INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/soundtouch/include>
	$<INSTALL_INTERFACE:/soundtouch/include>)

set_property(TARGET soundtouch PROPERTY FOLDER "3rdparty/")

target_compile_definitions(soundtouch PUBLIC
	ST_NO_EXCEPTION_HANDLING
	USE_MULTICH_ALWAYS
	SOUNDTOUCH_FLOAT_SAMPLES;
)

target_compile_options(soundtouch PRIVATE "-w")

if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86|X86|amd64|AMD64|em64t|EM64T)")
	target_compile_definitions(soundtouch PUBLIC
		SOUNDTOUCH_ALLOW_SSE
	)
endif ()
