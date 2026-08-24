file(GLOB DXBC_SOURCES
	"src/dxbc/*.cpp"
	"src/spirv/*.cpp"
	"src/util/*.cpp"
)

add_library(3rdparty_dxbc STATIC ${DXBC_SOURCES})

set_target_properties(3rdparty_dxbc PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	POSITION_INDEPENDENT_CODE ON)

target_include_directories(3rdparty_dxbc SYSTEM
	PUBLIC include/dxbc
	PUBLIC include/spirv
	PUBLIC include/util
	PUBLIC include/dxvk)

if(NOT MSVC)
	target_compile_options(3rdparty_dxbc PRIVATE -w)
endif()
