# IOKit_INCLUDE_DIR
# IOKit_LIBRARIES
# IOKit_FOUND
include(LibFindMacros)

# IOKit depends on CoreFoundation
find_package(CoreFoundation REQUIRED)

find_path(IOKit_INCLUDE_DIR
	IOKitLib.h
	PATH_SUFFIXES IOKit
)

find_library(IOKit_LIBRARY
	NAMES IOKit
)

set(IOKit_PROCESS_INCLUDES IOKit_INCLUDE_DIR CoreFoundation_INCLUDE_DIR)
set(IOKit_PROCESS_LIBS IOKit_LIBRARY CoreFoundation_LIBRARIES)
libfind_process(IOKit)
