# 7zip sdk
if(WIN32 OR APPLE)
	add_library(3rdparty_7zip STATIC EXCLUDE_FROM_ALL
		7zip/C/7zAlloc.c
		7zip/C/7zArcIn.c
		7zip/C/7zBuf.c
		7zip/C/7zBuf2.c
		7zip/C/7zCrc.c
		7zip/C/7zCrcOpt.c
		7zip/C/7zDec.c
		7zip/C/7zFile.c
		7zip/C/7zStream.c
		7zip/C/Aes.c
		7zip/C/AesOpt.c
		7zip/C/Alloc.c
		7zip/C/Bcj2.c
		7zip/C/Bcj2Enc.c
		7zip/C/Blake2s.c
		7zip/C/Bra.c
		7zip/C/Bra86.c
		7zip/C/BraIA64.c
		7zip/C/BwtSort.c
		7zip/C/CpuArch.c
		7zip/C/Delta.c
		7zip/C/DllSecur.c
		7zip/C/HuffEnc.c
		7zip/C/LzFind.c
		7zip/C/LzFindMt.c
		7zip/C/LzFindOpt.c
		7zip/C/Lzma2Dec.c
		7zip/C/Lzma2DecMt.c
		7zip/C/Lzma2Enc.c
		7zip/C/Lzma86Dec.c
		7zip/C/Lzma86Enc.c
		7zip/C/LzmaDec.c
		7zip/C/LzmaEnc.c
		7zip/C/LzmaLib.c
		7zip/C/MtCoder.c
		7zip/C/MtDec.c
		7zip/C/Ppmd7.c
		7zip/C/Ppmd7aDec.c
		7zip/C/Ppmd7Dec.c
		7zip/C/Ppmd7Enc.c
		7zip/C/Ppmd8.c
		7zip/C/Ppmd8Dec.c
		7zip/C/Ppmd8Enc.c
		7zip/C/Sha1.c
		7zip/C/Sha1Opt.c
		7zip/C/Sha256.c
		7zip/C/Sha256Opt.c
		7zip/C/Sort.c
		7zip/C/SwapBytes.c
		7zip/C/Threads.c
		7zip/C/Xxh64.c
		7zip/C/Xz.c
		7zip/C/XzCrc64.c
		7zip/C/XzCrc64Opt.c
		7zip/C/XzDec.c
		7zip/C/XzEnc.c
		7zip/C/XzIn.c
		7zip/C/ZstdDec.c)
	target_include_directories(3rdparty_7zip INTERFACE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/7zip/C>
		$<INSTALL_INTERFACE:/7zip/C>)

	target_include_directories(3rdparty_7zip INTERFACE 7zip)

	set_property(TARGET 3rdparty_7zip PROPERTY FOLDER "3rdparty/")

else()
	add_library(3rdparty_7zip INTERFACE)
endif()
