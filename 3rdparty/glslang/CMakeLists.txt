#glslang

set(ENABLE_PCH OFF CACHE BOOL "Enables Precompiled header" FORCE)
set(BUILD_EXTERNAL OFF CACHE BOOL "Build external dependencies in /External" FORCE)
set(SKIP_GLSLANG_INSTALL ON CACHE BOOL "Skip installation" FORCE)
set(ENABLE_SPVREMAPPER OFF CACHE BOOL "Enables building of SPVRemapper" FORCE)
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "Builds glslangValidator and spirv-remap" FORCE)
set(ENABLE_HLSL OFF CACHE BOOL "Enables HLSL input support" FORCE)
set(ENABLE_OPT OFF CACHE BOOL "Enables spirv-opt capability if present" FORCE)
set(ENABLE_CTEST OFF CACHE BOOL "Enables testing" FORCE)
add_subdirectory(glslang)
