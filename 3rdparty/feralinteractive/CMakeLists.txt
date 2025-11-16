# Feral Interactive

add_library(3rdparty_feralinteractive INTERFACE)

if (CMAKE_SYSTEM MATCHES "Linux")
	target_include_directories(3rdparty_feralinteractive INTERFACE feralinteractive/lib)
	target_compile_definitions(3rdparty_feralinteractive INTERFACE -DGAMEMODE_AVAILABLE)
	target_link_libraries(3rdparty_feralinteractive INTERFACE feralinteractive)
endif()
