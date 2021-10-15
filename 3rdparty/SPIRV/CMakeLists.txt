set(SKIP_SPIRV_TOOLS_INSTALL ON CACHE BOOL "Skip spirv-tools install" FORCE)
set(SPIRV-Headers_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SPIRV-Headers" CACHE STRING "SPIRV-Headers path" FORCE)
add_subdirectory(SPIRV-Tools)
