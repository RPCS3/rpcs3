# - Locate glext header
# This module defines
#  GLEXT_FOUND, if false, do not try to link to GLEXT
#  GLEXT_INCLUDE_DIRS, where to find headers.
#
# $GLEXT_DIR is an environment variable that points to the directory in which glext.h resides.

FIND_PATH(GLEXT_INCLUDE_DIR glext.h 
  HINTS
  $ENV{GLEXT_DIR}
  PATH_SUFFIXES include src GL
  PATHS
  /usr/include
  /usr/local/include
  /sw/include
  /opt/local/include
  /usr/freeware/include
)

# set the user variables
IF(GLEXT_INCLUDE_DIR)
  SET(GLEXT_INCLUDE_DIRS "${GLEXT_INCLUDE_DIR}")
ENDIF()
SET(GLEXT_LIBRARIES "${GLEXT_LIBRARY}")

# handle the QUIETLY and REQUIRED arguments and set GLEXT_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE("FindPackageHandleStandardArgs")
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GLEXT  DEFAULT_MSG  GLEXT_INCLUDE_DIR)

MARK_AS_ADVANCED(GLEXT_INCLUDE_DIR)