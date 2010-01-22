
# make bzip2
# build project bzip2
if(projectBZip2)
	add_subdirectory(bzip2)
endif(projectBZip2)

# make zlib
# build project zlib
if(projectZLIB)
	add_subdirectory(zlib)
endif(projectZLIB)

# make SoundTouch
if(projectSoundTouch)
	add_subdirectory(SoundTouch)
endif(projectSoundTouch)

