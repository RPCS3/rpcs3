# zstd
# Select the version of zstd to use, default is builtin
if(NOT USE_SYSTEM_ZSTD)
  option(ZSTD_BUILD_PROGRAMS "BUILD PROGRAMS" OFF)
  option(ZSTD_BUILD_SHARED "BUILD SHARED LIBRARIES" OFF)
  option(ZSTD_BUILD_STATIC "BUILD STATIC LIBRARIES" ON)
  option(ZSTD_BUILD_TESTS "BUILD TESTS" OFF)
  
  add_subdirectory(zstd/build/cmake EXLUDE_FROM_ALL)
  add_library(3rdparty_zstd INTERFACE)
  target_link_libraries(3rdparty_zstd INTERFACE libzstd_static)
else()
  message(STATUS "RPCS3: using VCPKG zstd")
  find_package(zstd CONFIG REQUIRED)
  add_library(3rdparty_zstd INTERFACE)
  target_link_libraries(3rdparty_zstd INTERFACE zstd::libzstd)
endif()
