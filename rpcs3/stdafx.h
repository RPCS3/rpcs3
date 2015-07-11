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

#pragma warning( disable : 4351 )

#include <cstdio>
#include <cstring>
#include <cassert>
#include <cstdint>
#include <climits>
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

using uint = unsigned int;

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using s8 = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;

using b8 = std::uint8_t;

using f32 = float;
using f64 = double;

template<typename T, typename = std::enable_if_t<std::is_integral<T>::value>> inline T align(const T& value, u64 align)
{
	return static_cast<T>((value + (align - 1)) & ~(align - 1));
}

// copy null-terminated string from std::string to char array with truncation
template<std::size_t N> inline void strcpy_trunc(char(&dst)[N], const std::string& src)
{
	const std::size_t count = src.size() >= N ? N - 1 : src.size();
	std::memcpy(dst, src.c_str(), count);
	dst[count] = '\0';
}

// copy null-terminated string from char array to char array with truncation
template<std::size_t N, std::size_t N2> inline void strcpy_trunc(char(&dst)[N], const char(&src)[N2])
{
	const std::size_t count = N2 >= N ? N - 1 : N2;
	std::memcpy(dst, src, count);
	dst[count] = '\0';
}

// bool wrapper for restricting bool result conversions
struct explicit_bool_t
{
	const bool value;

	explicit_bool_t(bool value)
		: value(value)
	{
	}

	explicit operator bool() const
	{
		return value;
	}
};

// return 32 bit sizeof() to avoid widening/narrowing conversions with size_t
#define sizeof32(type) static_cast<u32>(sizeof32_t<sizeof(type)>::value)

// return 32 bit alignof() to avoid widening/narrowing conversions with size_t
#define alignof32(type) static_cast<u32>(alignof32_t<__alignof(type)>::value)

template<std::size_t Size> struct sizeof32_t
{
	static_assert(Size <= UINT32_MAX, "sizeof32() error: size is too big");

	enum : u32 { value = static_cast<u32>(Size) };
};

template<std::size_t Align> struct alignof32_t
{
	static_assert(Align <= UINT32_MAX, "alignof32() error: alignment is too big");

	enum : u32 { value = static_cast<u32>(Align) };
};

template<typename T> using func_def = T; // workaround for MSVC bug: `using X = func_def<void()>;` instead of `using X = void();`

template<typename T> struct ID_type;

#define REG_ID_TYPE(t, id) template<> struct ID_type<t> { enum : u32 { type = id }; }

#define CHECK_SIZE(type, size) static_assert(sizeof(type) == size, "Invalid " #type " type size")
#define CHECK_ALIGN(type, align) static_assert(__alignof(type) == align, "Invalid " #type " type alignment")
#define CHECK_MAX_SIZE(type, size) static_assert(sizeof(type) <= size, #type " type size is too big")
#define CHECK_SIZE_ALIGN(type, size, align) CHECK_SIZE(type, size); CHECK_ALIGN(type, align)

#define WRAP_EXPR(expr) [&]{ return expr; }
#define COPY_EXPR(expr) [=]{ return expr; }
#define EXCEPTION(text, ...) fmt::exception(__FILE__, __LINE__, __FUNCTION__, text, ##__VA_ARGS__)
#define VM_CAST(value) vm::impl_cast(value, __FILE__, __LINE__, __FUNCTION__)

#define _PRGNAME_ "RPCS3"
#define _PRGVER_ "0.0.0.5"

#include "Utilities/BEType.h"
#include "Utilities/StrFmt.h"
#include "Emu/Memory/atomic.h"
