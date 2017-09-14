# Find Vulkan

find_package(PkgConfig)
pkg_check_modules(PC_VULKAN vulkan)

# RPCS3 ships with the SDK headers
# find_path(VULKAN_INCLUDE_DIR
#   NAMES vulkan/vulkan.h
#   HINTS ${PC_VULKAN_INCLUDE_DIR} ${PC_VULKAN_INCLUDE_DIRS})

find_library(VULKAN_LIBRARY
  NAMES vulkan
  HINTS ${PC_VULKAN_LIBRARY} ${PC_VULKAN_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Vulkan DEFAULT_MSG VULKAN_LIBRARY)

mark_as_advanced(VULKAN_LIBRARY VULKAN_INCLUDE_DIR)
