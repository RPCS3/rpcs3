# Try to find A52
# Once done, this will define
#
# A52_FOUND - system has A52
# A52_INCLUDE_DIR - the A52 include directories
# A52_LIBRARIES - link these to use A52

if(A52_INCLUDE_DIR AND A52_LIBRARIES)
    set(A52_FIND_QUIETLY TRUE)
endif(A52_INCLUDE_DIR AND A52_LIBRARIES)

# include dir
find_path(A52_INCLUDE_DIR a52dec/a52.h)

# finally the library itself
find_library(libA52 NAMES a52)
set(A52_LIBRARIES ${libA52})

# handle the QUIETLY and REQUIRED arguments and set A52_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(A52 DEFAULT_MSG A52_LIBRARIES A52_INCLUDE_DIR)

mark_as_advanced(A52_LIBRARIES A52_INCLUDE_DIR)