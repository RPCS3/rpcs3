/*
* Copyright (c) 2011-2013 by naehrwert
* This file is released under the GPLv2.
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

namespace scetool{
	typedef char s8;
	typedef unsigned char u8;
	typedef short s16;
	typedef unsigned short u16;
	typedef int s32;
	typedef unsigned int u32;
	#if defined(_WIN32) && defined(_MSC_VER)
	typedef __int64 s64;
	typedef unsigned __int64 u64;
	#else
	typedef long long int s64;
	typedef unsigned long long int u64;
	#endif
}

#define BOOL int
#define TRUE 1
#define FALSE 0

//Align.
#define ALIGN(x, a) (((x) + (a) - 1) & ~((a) - 1))

//Bits <-> bytes conversion.
#define BITS2BYTES(x) ((x) / 8)
#define BYTES2BITS(x) ((x) * 8)

//Endian swap for scetool::u16.
#define _Es16(val) \
	((scetool::u16)(((((scetool::u16)val) & 0xff00) >> 8) | \
	       ((((scetool::u16)val) & 0x00ff) << 8)))

//Endian swap for scetool::u32.
#define _Es32(val) \
	((scetool::u32)(((((scetool::u32)val) & 0xff000000) >> 24) | \
	       ((((scetool::u32)val) & 0x00ff0000) >> 8 ) | \
	       ((((scetool::u32)val) & 0x0000ff00) << 8 ) | \
	       ((((scetool::u32)val) & 0x000000ff) << 24)))

//Endian swap for scetool::u64.
#define _Es64(val) \
	((scetool::u64)(((((scetool::u64)val) & 0xff00000000000000ull) >> 56) | \
	       ((((scetool::u64)val) & 0x00ff000000000000ull) >> 40) | \
	       ((((scetool::u64)val) & 0x0000ff0000000000ull) >> 24) | \
	       ((((scetool::u64)val) & 0x000000ff00000000ull) >> 8 ) | \
	       ((((scetool::u64)val) & 0x00000000ff000000ull) << 8 ) | \
	       ((((scetool::u64)val) & 0x0000000000ff0000ull) << 24) | \
	       ((((scetool::u64)val) & 0x000000000000ff00ull) << 40) | \
	       ((((scetool::u64)val) & 0x00000000000000ffull) << 56)))

#ifdef __cplusplus
}
#endif

