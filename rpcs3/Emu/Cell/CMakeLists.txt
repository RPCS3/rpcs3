file(GLOB_RECURSE SRC_FILES "*.cpp")

if(NOT MSVC)
	set_source_files_properties(PPUTranslator.cpp PROPERTIES COMPILE_FLAGS -fno-rtti)
endif()

add_library(rpcs3_cell ${SRC_FILES})

target_link_libraries(rpcs3_cell
	PRIVATE
		rpcs3_utilities rpcs3_rsx
		rpcs3_crypto rpcs3_loader
		3rdparty::yaml-cpp
		3rdparty::stblib 3rdparty::libpng
	PUBLIC
		rpcs3_utilities_jit)

# For some reason GCC 7 requires manually linking with -latomic
if(CMAKE_COMPILER_IS_GNUCXX AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 7 OR CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 7))
	target_link_libraries(rpcs3_cell PUBLIC -latomic)
endif()
