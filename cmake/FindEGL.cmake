# Try to find EGL
# Once done, this will define
#
# EGL_FOUND - system has EGL
# EGL_INCLUDE_DIR - the EGL include directories
# EGL_LIBRARIES - link these to use EGL

if(EGL_INCLUDE_DIR AND EGL_LIBRARIES)
    set(EGL_FIND_QUIETLY TRUE)
endif(EGL_INCLUDE_DIR AND EGL_LIBRARIES)

INCLUDE(CheckCXXSymbolExists)

# include dir
find_path(EGL_INCLUDE_DIR EGL/eglext.h)

CHECK_CXX_SYMBOL_EXISTS(EGL_KHR_create_context "EGL/eglext.h" EGL_GL_CONTEXT_SUPPORT)

# finally the library itself
find_library(libEGL NAMES EGL)
set(EGL_LIBRARIES ${libEGL})

# handle the QUIETLY and REQUIRED arguments and set EGL_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(EGL DEFAULT_MSG EGL_LIBRARIES EGL_INCLUDE_DIR)

mark_as_advanced(EGL_LIBRARIES EGL_INCLUDE_DIR)

