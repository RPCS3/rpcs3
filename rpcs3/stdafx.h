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

template<typename T> __forceinline T align(const T addr, int align)
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
