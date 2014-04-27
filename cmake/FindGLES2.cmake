# Try to find GLESV2
# Once done, this will define
#
# GLESV2_FOUND - system has GLESV2
# GLESV2_INCLUDE_DIR - the GLESV2 include directories
# GLESV2_LIBRARIES - link these to use GLESV2

if(GLESV2_INCLUDE_DIR AND GLESV2_LIBRARIES)
    set(GLESV2_FIND_QUIETLY TRUE)
endif(GLESV2_INCLUDE_DIR AND GLESV2_LIBRARIES)

INCLUDE(CheckCXXSymbolExists)

# include dir
find_path(GLESV2_INCLUDE_DIR GLES3/gl3ext.h)

# finally the library itself
find_library(libGLESV2 NAMES GLESv2)
set(GLESV2_LIBRARIES ${libGLESV2})

# handle the QUIETLY and REQUIRED arguments and set GLESV2_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLESV2 DEFAULT_MSG GLESV2_LIBRARIES GLESV2_INCLUDE_DIR)

mark_as_advanced(GLESV2_LIBRARIES GLESV2_INCLUDE_DIR)

