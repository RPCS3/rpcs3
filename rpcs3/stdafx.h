#pragma once

#ifdef MSVC_CRT_MEMLEAK_DETECTION
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif

#define NOMINMAX

#if defined(MSVC_CRT_MEMLEAK_DETECTION) && defined(_DEBUG) && !defined(DBG_NEW)
	#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
	#include "define_new_memleakdetect.h"
#endif

// This header should be frontend-agnostic, so don't assume wx includes everything
#pragma warning( disable : 4800 )

#include <cstdio>
#include <cstring>
#include <cassert>
#include <cstdint>
#include <cmath>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <memory>
#include <vector>
#include <queue>
#include <set>
#include <array>
#include <string>
#include <functional>
#include <algorithm>
#include <random>
#include <unordered_set>
#include <map>
#include <unordered_map>

#include "Utilities/GNU.h"

typedef unsigned int uint;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef float f32;
typedef double f64;

#pragma pack(push, 2)
struct f16
{
	u16 data;

	static f16 make_from_f32(f32 value)
	{
		u8 *tmp = (u8*)&value;
		u32 bits = ((u32)tmp[0] << 24) | ((u32)tmp[1] << 16) | ((u32)tmp[2] << 8) | (u32)tmp[3];

		if (bits == 0)
		{
			return{ 0 };
		}

		s32 e = ((bits & 0x7f800000) >> 23) - 127 + 15;

		if (e < 0)
		{
			return{ 0 };
		}

		if (e > 31)
		{
			e = 31;
		}

		u32 s = bits & 0x80000000;
		u32 m = bits & 0x007fffff;

		return{ ((s >> 16) & 0x8000) | ((e << 10) & 0x7c00) | ((m >> 13) & 0x03ff) };
	}

	static f16 make_from_u16(u16 value)
	{
		return{ value };
	}

	f16& sign(u16 value)
	{
		data &= ~0x8000;
		data |= value << 15;
		return *this;
	}

	bool sign() const
	{
		return data >> 15 ? true : false;
	}

	f16& exp(u16 value)
	{
		data &= ~0x7c00;
		data |= (value << 10) & 0x7c00;
		return *this;
	}

	u16 exp() const
	{
		return (data & 0x7c00) >> 10;
	}

	f16& mant(u16 value)
	{
		data &= ~0x03ff;
		data |= value & 0x03ff;
		return *this;
	}

	u16 mant() const
	{
		return data & 0x03ff;
	}
};

static_assert(sizeof(f16) == 2, "bad f16 implemantation");
#pragma pack(pop)

template<typename T> force_inline T align(const T addr, int align)
{
	return (addr + (align - 1)) & ~(align - 1);
}

#include "Utilities/BEType.h"
#include "Utilities/StrFmt.h"

#include "Emu/Memory/atomic.h"

template<typename T> struct ID_type;

#define REG_ID_TYPE(t, id) template<> struct ID_type<t> { static const u32 type = id; }

#define _PRGNAME_ "RPCS3"
#define _PRGVER_ "0.0.0.5"
