# Try to find SparseHash
# Once done, this will define
#
# SPARSEHASE_NEW_FOUND - system has SparseHash
# SPARSEHASE_NEW_INCLUDE_DIR - the SparseHash include directories

if(SPARSEHASE_NEW_INCLUDE_DIR)
    set(SPARSEHASE_NEW_FIND_QUIETLY TRUE)
endif(SPARSEHASE_NEW_INCLUDE_DIR)

find_path(SPARSEHASE_NEW_INCLUDE_DIR sparsehash/internal/densehashtable.h)

# handle the QUIETLY and REQUIRED arguments and set SPARSEHASE_NEW_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SparseHash_new DEFAULT_MSG SPARSEHASE_NEW_INCLUDE_DIR)

mark_as_advanced(SPARSEHASE_NEW_INCLUDE_DIR)
