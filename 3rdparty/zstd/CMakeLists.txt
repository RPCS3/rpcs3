
project(3rdparty_zstd)

add_library(3rdparty_zstd INTERFACE)

add_subdirectory(zstd/build/cmake EXLUDE_FROM_ALL)

target_include_directories(3rdparty_zstd INTERFACE zstd zstd/lib)

