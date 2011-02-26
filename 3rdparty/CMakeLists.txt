# Check that people use the good file
if(NOT TOP_CMAKE_WAS_SOURCED)
    message(FATAL_ERROR "
    You did not 'cmake' the good CMakeLists.txt file. Use the one in the top dir.
    It is advice to delete all wrongly generated cmake stuff => CMakeFiles & CMakeCache.txt")
endif(NOT TOP_CMAKE_WAS_SOURCED)


# build project zlib
if(projectZLIB)
	add_subdirectory(zlib)
endif(projectZLIB)

# build project SoundTouch
if(projectSoundTouch)
	add_subdirectory(SoundTouch)
endif(projectSoundTouch)

# build project sdl
if(projectSDL)
    add_subdirectory(SDL-1.3.0-5387)
endif(projectSDL)
