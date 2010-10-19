#
# Try to find GLEW library and include path.
# Once done this will define
#
# GLEW_FOUND - system has GLEW
# GLEW_INCLUDE_DIR - the GLEW include directories
# GLEW_LIBRARY - link these to use GLEW
# 

if(GLEW_INCLUDE_DIR AND GLEW_LIBRARY)
    set(GLEW_FIND_QUIETLY TRUE)
endif(GLEW_INCLUDE_DIR AND GLEW_LIBRARY)

IF (WIN32)
    FIND_PATH( GLEW_INCLUDE_DIR GL/glew.h
		$ENV{PROGRAMFILES}/GLEW/include
		${PROJECT_SOURCE_DIR}/src/nvgl/glew/include
		DOC "The directory where GL/glew.h resides")
	FIND_LIBRARY( GLEW_LIBRARY
		NAMES glew GLEW glew32 glew32s
		PATHS
		$ENV{PROGRAMFILES}/GLEW/lib
		${PROJECT_SOURCE_DIR}/src/nvgl/glew/bin
		${PROJECT_SOURCE_DIR}/src/nvgl/glew/lib
		DOC "The GLEW library")
ELSE (WIN32)
    FIND_PATH( GLEW_INCLUDE_DIR GL/glew.h
		/usr/include
		/usr/local/include
		/sw/include
		/opt/local/include
		DOC "The directory where GL/glew.h resides")
	FIND_LIBRARY( GLEW_LIBRARY
		NAMES GLEW glew
		PATHS
		/usr/lib32
		/usr/lib
		/usr/local/lib32
		/usr/local/lib
		/sw/lib
		/opt/local/lib
		DOC "The GLEW library")
ENDIF (WIN32)

# handle the QUIETLY and REQUIRED arguments and set GLEW_FOUND to TRUE if 
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLEW DEFAULT_MSG GLEW_LIBRARY GLEW_INCLUDE_DIR)

mark_as_advanced(GLEW_LIBRARY GLEW_INCLUDE_DIR)

