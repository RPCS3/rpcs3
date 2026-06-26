# OpenXR loader from the OpenXR-SDK submodule.
# We only need the loader, and link it statically like on Windows. The runtime itself is provided by the system (e.g. SteamVR or Monado).

set(BUILD_TESTS OFF CACHE BOOL "Build OpenXR tests" FORCE)
set(BUILD_API_LAYERS OFF CACHE BOOL "Build OpenXR API layers" FORCE)
set(BUILD_CONFORMANCE_TESTS OFF CACHE BOOL "Build OpenXR conformance tests" FORCE)
set(BUILD_SDK_TESTS OFF CACHE BOOL "Build OpenXR SDK samples" FORCE)
set(DYNAMIC_LOADER OFF CACHE BOOL "Build the OpenXR loader as a shared library" FORCE)
set(BUILD_WITH_SYSTEM_JSONCPP OFF CACHE BOOL "Use system jsoncpp for the OpenXR loader" FORCE)

add_subdirectory(OpenXR-SDK EXCLUDE_FROM_ALL)

add_library(3rdparty_openxr INTERFACE)
target_link_libraries(3rdparty_openxr INTERFACE openxr_loader)
target_compile_definitions(3rdparty_openxr INTERFACE HAVE_OPENXR)

# Suppress warnings from the headers, like for the other 3rdparty libraries
target_include_directories(3rdparty_openxr SYSTEM INTERFACE OpenXR-SDK/include)
