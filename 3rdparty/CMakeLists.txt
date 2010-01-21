
# make bzip2
# build local bzip2
if(localBZip2)
	add_subdirectory(bzip2)
endif(localBZip2)

# make zlib
# build local zlib
if(localZLIB)
	add_subdirectory(zlib)
endif(localZLIB)

# make SoundTouch
if(localSoundTouch)
	add_subdirectory(SoundTouch)
endif(localSoundTouch)

