add_library(3rdparty_zarchive STATIC
    zarchive/src/zarchivereader.cpp
    zarchive/src/zarchivewriter.cpp
    zarchive/src/sha_256.c
)

target_include_directories(3rdparty_zarchive SYSTEM PUBLIC zarchive/include)
target_link_libraries(3rdparty_zarchive PRIVATE 3rdparty::zstd)
target_compile_features(3rdparty_zarchive PUBLIC cxx_std_20)

add_library(3rdparty::zarchive ALIAS 3rdparty_zarchive)
