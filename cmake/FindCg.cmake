# Find a NVidia Cg Toolkit installation
# This module finds a NVidia Cg Toolkit installation.

# CG_FOUND			found Cg
# CG_INCLUDE_DIRS	include path to cg.h
# CG_LIBRARIES		path to Cg libs
# CG_COMPILER		path to Cg compiler

# find Cg on Windows
if(WIN32)
	# find Cg compiler
	find_program(CG_COMPILER cgc PATHS
				 "C:/Program Files/NVIDIA Corporation/cg/bin"
				 DOC "Path to the Cg compiler.")
			 
	# find Cg include
	find_path(CG_INCLUDE_DIRS NAMES Cg/cg.h GL/glext.h PATHS
			  "C:/Program Files/NVIDIA Corporation/cg/include"
			  DOC "Path to the Cg/GL includes.")
	
	# find Cg libraries
	# Cg library
	find_library(CG_LIBRARY NAMES Cg PATHS
				 "C:/Program Files/NVIDIA Corporation/cg/lib"
				 DOC "Path to the Cg library.")

	# Cg GL library
	find_library(CG_GL_LIBRARY NAMES CgGL PATHS
				 "C:/Program Files/NVIDIA Corporation/cg/lib"
				 DOC "Path to the CgGL library.")

	set(CG_LIBRARIES ${CG_LIBRARY} ${CG_GL_LIBRARY})

else(WIN32) # Unix based OS
	# find Cg compiler
	find_program(CG_COMPILER cgc PATHS
				 /usr/bin
				 /usr/local/bin
				/opt/nvidia-cg-toolkit/bin
				 DOC "Path to the Cg compiler.")
	
	# find Cg include
	find_path(CG_INCLUDE_DIRS NAMES Cg/cg.h GL/glext.h PATHS
			  /usr/include
			  /usr/local/include
			  /opt/nvidia-cg-toolkit/include
			  DOC "Path to the Cg/GL includes.")
	
	# find Cg libraries
	# Cg library
	find_library(CG_LIBRARY NAMES Cg PATHS
				 /usr/include
				 /usr/local/lib
				 /opt/nvidia-cg-toolkit/lib	
				 DOC "Path to the Cg library.")

	# Cg GL library
	find_library(CG_GL_LIBRARY NAMES CgGL PATHS
				 /usr/include
				 /usr/local/lib
				 /opt/nvidia-cg-toolkit/lib
				 DOC "Path to the CgGL library.")

	set(CG_LIBRARIES ${CG_LIBRARY} ${CG_GL_LIBRARY})
endif(WIN32)

# handle the QUIETLY and REQUIRED arguments and set CG_FOUND to TRUE if 
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Cg DEFAULT_MSG CG_LIBRARIES CG_INCLUDE_DIRS)

mark_as_advanced(CG_FOUND CG_INCLUDE_DIRS CG_LIBRARIES CG_COMPILER)

