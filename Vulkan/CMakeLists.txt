set(SKIP_GLSLANG_INSTALL ON CACHE BOOL "Skip installation" FORCE)
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "Builds glslangValidator and spirv-remap" FORCE)
set(ENABLE_HLSL OFF CACHE BOOL "Enables HLSL input support" FORCE)
add_subdirectory(glslang)
