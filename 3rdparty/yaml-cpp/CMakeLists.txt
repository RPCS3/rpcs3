
# We don't want to install yaml-cpp but its cmake file doesn't have option
# to disable it...
# So we just install it to a different directory
set(YAML_CPP_INSTALL OFF CACHE BOOL "Don't install YAML")
set(YAML_CPP_DISABLE_UNINSTALL ON CACHE BOOL "Disable yaml-cpp uninstall")

set(CMAKE_INSTALL_PREFIX_OLD ${CMAKE_INSTALL_PREFIX})
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/yaml-cpp_install)

set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "Enable testing" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "Enable parse tools" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "Enable contrib stuff in library" FORCE)
add_subdirectory(yaml-cpp EXCLUDE_FROM_ALL)

set(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX_OLD})
