# OpenAL
if (MSVC)
	find_path(OPENAL_INCLUDE_DIR al.h PATHS OpenAL/include)
	find_library(OPENAL_LIBRARY OpenAL32 PATHS OpenAL/libs/Win64/)
else()
	find_package(OpenAL REQUIRED)
endif()

add_library(3rdparty_openal INTERFACE)
target_include_directories(3rdparty_openal INTERFACE ${OPENAL_INCLUDE_DIR})
target_link_libraries(3rdparty_openal INTERFACE ${OPENAL_LIBRARY})
