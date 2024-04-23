# 7z sdk
if(WIN32 OR APPLE)
	add_library(3rdparty_7z STATIC EXCLUDE_FROM_ALL
		src/7zAlloc.c
		src/7zArcIn.c
		src/7zBuf.c
		src/7zBuf2.c
		src/7zCrc.c
		src/7zCrcOpt.c
		src/7zDec.c
		src/7zFile.c
		src/7zStream.c
		src/Aes.c
		src/AesOpt.c
		src/Alloc.c
		src/Bcj2.c
		src/Bcj2Enc.c
		src/Blake2s.c
		src/Bra.c
		src/Bra86.c
		src/BraIA64.c
		src/BwtSort.c
		src/CpuArch.c
		src/Delta.c
		src/DllSecur.c
		src/HuffEnc.c
		src/LzFind.c
		src/LzFindMt.c
		src/LzFindOpt.c
		src/Lzma2Dec.c
		src/Lzma2DecMt.c
		src/Lzma2Enc.c
		src/Lzma86Dec.c
		src/Lzma86Enc.c
		src/LzmaDec.c
		src/LzmaEnc.c
		src/LzmaLib.c
		src/MtCoder.c
		src/MtDec.c
		src/Ppmd7.c
		src/Ppmd7aDec.c
		src/Ppmd7Dec.c
		src/Ppmd7Enc.c
		src/Ppmd8.c
		src/Ppmd8Dec.c
		src/Ppmd8Enc.c
		src/Sha1.c
		src/Sha1Opt.c
		src/Sha256.c
		src/Sha256Opt.c
		src/Sort.c
		src/SwapBytes.c
		src/Threads.c
		src/Xz.c
		src/XzCrc64.c
		src/XzCrc64Opt.c
		src/XzDec.c
		src/XzEnc.c
		src/XzIn.c)
	target_include_directories(3rdparty_7z INTERFACE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
		$<INSTALL_INTERFACE:/src>)

	target_include_directories(3rdparty_7z INTERFACE 7z)

	set_property(TARGET 3rdparty_7z PROPERTY FOLDER "3rdparty/")

else()
	add_library(3rdparty_7z INTERFACE)
endif()
