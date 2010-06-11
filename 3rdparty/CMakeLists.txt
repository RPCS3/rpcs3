# build project bzip2
if(projectBZip2)
	add_subdirectory(bzip2)
endif(projectBZip2)

# build project zlib
if(projectZLIB)
	add_subdirectory(zlib)
endif(projectZLIB)

# build project SoundTouch
if(projectSoundTouch)
	add_subdirectory(SoundTouch)
endif(projectSoundTouch)
