# CoreFoundation_INCLUDE_DIR
# CoreFoundation_LIBRARIES
# CoreFoundation_FOUND
include(LibFindMacros)

find_path(CoreFoundation_INCLUDE_DIR
	CoreFoundation.h
	PATH_SUFFIXES CoreFoundation
)

find_library(CoreFoundation_LIBRARY
	NAMES CoreFoundation
)

set(CoreFoundation_PROCESS_INCLUDES CoreFoundation_INCLUDE_DIR)
set(CoreFoundation_PROCESS_LIBS CoreFoundation_LIBRARY)
libfind_process(CoreFoundation)
