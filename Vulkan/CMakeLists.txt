set(SKIP_SPIRV_TOOLS_INSTALL ON CACHE BOOL "Skip spirv-tools install" FORCE)
set(SPIRV-Headers_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/spirv-headers" CACHE STRING "spirv-headers path" FORCE)
add_subdirectory(spirv-tools)
