set(RPCS3_LIBCHDR_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libchdr")

set(chdr_sources
    libchdr_bitstream.c
    libchdr_cdrom.c
    libchdr_chd.c
    libchdr_codec_avhuff.c
    libchdr_codec_cdfl.c
    libchdr_codec_cdlz.c
    libchdr_codec_cdzl.c
    libchdr_codec_cdzs.c
    libchdr_codec_flac.c
    libchdr_codec_huff.c
    libchdr_codec_lzma.c
    libchdr_codec_zlib.c
    libchdr_codec_zstd.c
    libchdr_flac.c
    libchdr_huffman.c
)
list(TRANSFORM chdr_sources PREPEND "${RPCS3_LIBCHDR_SOURCE_DIR}/src/")
add_library(rpcs3_chdr STATIC ${chdr_sources}
    "${RPCS3_LIBCHDR_SOURCE_DIR}/deps/lzma-26.02/src/LzmaDec.c")
target_include_directories(rpcs3_chdr PUBLIC "${RPCS3_LIBCHDR_SOURCE_DIR}/include")
target_compile_definitions(rpcs3_chdr PRIVATE
    CHDR_SYSTEM_ZLIB CHDR_SYSTEM_ZSTD
    WANT_RAW_DATA_SECTOR=1 WANT_SUBCODE=1 VERIFY_BLOCK_CRC=1
    LOWRAM_TARGET=0)
target_link_libraries(rpcs3_chdr PRIVATE 3rdparty::zlib 3rdparty::zstd)
if(MSVC)
    target_compile_definitions(rpcs3_chdr PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
add_library(3rdparty::chdr ALIAS rpcs3_chdr)
