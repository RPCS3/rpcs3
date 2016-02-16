if (NOT APPLE)
add_subdirectory( glslang )
set(BUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)
set(BUILD_DEMOS OFF CACHE BOOL "Build demos" FORCE)
# TravisCI break build with layers and vkjson
set(BUILD_LAYERS OFF CACHE BOOL "Build demos" FORCE)
set(BUILD_VKJSON OFF CACHE BOOL "Build demos" FORCE)
add_subdirectory( Vulkan-LoaderAndValidationLayers )
endif()