# Cubeb

function(add_sanitizers ...)
endfunction(add_sanitizers)

set(BUILD_SHARED_LIBS FALSE CACHE BOOL "Don't build shared libs")
set(BUILD_TESTS FALSE CACHE BOOL "Don't build tests")
set(BUILD_RUST_LIBS FALSE CACHE BOOL "Don't build rust libs")
set(BUILD_TOOLS FALSE CACHE BOOL "Don't build tools")

add_subdirectory(cubeb EXCLUDE_FROM_ALL)
add_library(3rdparty::cubeb ALIAS cubeb)
