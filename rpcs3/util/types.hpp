#pragma once // No BOM and only basic ASCII in this header, or a neko will die

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <utility>
#include <chrono>
#include <array>

using std::chrono::steady_clock;

using namespace std::literals;

#ifdef _MSC_VER
#if !defined(__cpp_lib_bitops) && _MSC_VER < 1928
#define __cpp_lib_bitops
#endif
#endif
#include <bit>

#ifndef __has_builtin
	#define __has_builtin(x) 0
#endif

#ifdef _MSC_VER
#define SAFE_BUFFERS __declspec(safebuffers)
#define NEVER_INLINE __declspec(noinline)
#define FORCE_INLINE __forceinline
#else // not _MSC_VER
#define SAFE_BUFFERS __attribute__((no_stack_protector))
#define NEVER_INLINE __attribute__((noinline)) inline
#define FORCE_INLINE __attribute__((always_inline)) inline
#endif // _MSC_VER

#define CHECK_SIZE(type, size) static_assert(sizeof(type) == size, "Invalid " #type " type size")
#define CHECK_ALIGN(type, align) static_assert(alignof(type) == align, "Invalid " #type " type alignment")
#define CHECK_MAX_SIZE(type, size) static_assert(sizeof(type) <= size, #type " type size is too big")
#define CHECK_SIZE_ALIGN(type, size, align) CHECK_SIZE(type, size); CHECK_ALIGN(type, align)

#define DECLARE(...) decltype(__VA_ARGS__) __VA_ARGS__

#define STR_CASE(...) case __VA_ARGS__: return #__VA_ARGS__

#if defined(_DEBUG) || defined(_AUDIT)
#define AUDIT(...) (static_cast<void>(ensure(__VA_ARGS__)))
#else
#define AUDIT(...) (static_cast<void>(0))
#endif

#if __cpp_lib_bit_cast < 201806L
namespace std
{
	template <class To, class From, typename = std::enable_if_t<sizeof(To) == sizeof(From)>>
	constexpr To bit_cast(const From& from) noexcept
	{
		static_assert(sizeof(To) == sizeof(From), "std::bit_cast<>: incompatible type size");

		if constexpr ((std::is_same_v<std::remove_const_t<To>, std::remove_const_t<From>> && std::is_constructible_v<To, From>) || (std::is_integral_v<From> && std::is_integral_v<To>))
		{
			return static_cast<To>(from);
		}

		To result{};
		__builtin_memcpy(&result, &from, sizeof(From));
		return result;
	}
}
#endif

using schar  = signed char;
using uchar  = unsigned char;
using ushort = unsigned short;
using uint   = unsigned int;
using ulong  = unsigned long;
using ullong = unsigned long long;
using llong  = long long;

#if __APPLE__
using uptr = std::uint64_t;
#else
using uptr = std::uintptr_t;
#endif

using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using usz = std::size_t;

using s8  = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;

#if __APPLE__
namespace std
{
	template <typename T, typename = std::enable_if_t<std::is_unsigned_v<T>>>
	constexpr int countr_zero(T x) noexcept
	{
		if (x == 0)
			return sizeof(T) * 8;
		if constexpr (sizeof(T) <= sizeof(uint))
			return __builtin_ctz(x);
		else if constexpr (sizeof(T) <= sizeof(ulong))
			return __builtin_ctzl(x);
		else if constexpr (sizeof(T) <= sizeof(ullong))
			return __builtin_ctzll(x);
		else
			static_assert(sizeof(T) <= sizeof(ullong));
	}

	template <typename T, typename = std::enable_if_t<std::is_unsigned_v<T>>>
	constexpr int countr_one(T x) noexcept
	{
		return countr_zero<T>(~x);
	}

	template <typename T, typename = std::enable_if_t<std::is_unsigned_v<T>>>
	constexpr int countl_zero(T x) noexcept
	{
		if (x == 0)
			return sizeof(T) * 8;
		if constexpr (sizeof(T) <= sizeof(uint))
			return __builtin_clz(x) - (sizeof(uint) - sizeof(T)) * 8;
		else if constexpr (sizeof(T) <= sizeof(ulong))
			return __builtin_clzl(x) - (sizeof(ulong) - sizeof(T)) * 8;
		else if constexpr (sizeof(T) <= sizeof(ullong))
			return __builtin_clzll(x) - (sizeof(ullong) - sizeof(T)) * 8;
		else
			static_assert(sizeof(T) <= sizeof(ullong));
	}

	template <typename T, typename = std::enable_if_t<std::is_unsigned_v<T>>>
	constexpr int countl_one(T x) noexcept
	{
		return countl_zero<T>(~x);
	}
}
#endif

// Get integral type from type size
template <usz N>
struct get_int_impl
{
};

template <>
struct get_int_impl<sizeof(u8)>
{
	using utype = u8;
};

template <>
struct get_int_impl<sizeof(u16)>
{
	using utype = u16;
};

template <>
struct get_int_impl<sizeof(u32)>
{
	using utype = u32;
};

template <>
struct get_int_impl<sizeof(u64)>
{
	using utype = u64;
};

template <usz N>
using get_uint_t = typename get_int_impl<N>::utype;

template <typename T>
std::remove_cvref_t<T> as_rvalue(T&& obj)
{
	return std::forward<T>(obj);
}

template <typename T, usz Align>
class atomic_t;

namespace stx
{
	template <typename T, bool Se, usz Align>
	class se_t;
}

using stx::se_t;

// se_t<> with native endianness
template <typename T, usz Align = alignof(T)>
using nse_t = se_t<T, false, Align>;

template <typename T, usz Align = alignof(T)>
using be_t = se_t<T, std::endian::little == std::endian::native, Align>;
template <typename T, usz Align = alignof(T)>
using le_t = se_t<T, std::endian::big == std::endian::native, Align>;

template <typename T, usz Align = alignof(T)>
using atomic_be_t = atomic_t<be_t<T>, Align>;
template <typename T, usz Align = alignof(T)>
using atomic_le_t = atomic_t<le_t<T>, Align>;

// Extract T::simple_type if available, remove cv qualifiers
template <typename T, typename = void>
struct simple_type_helper
{
	using type = typename std::remove_cv<T>::type;
};

template <typename T>
struct simple_type_helper<T, std::void_t<typename T::simple_type>>
{
	using type = typename T::simple_type;
};

template <typename T>
using simple_t = typename simple_type_helper<T>::type;

// Bool type equivalent
class b8
{
	u8 m_value;

public:
	b8() = default;

	constexpr b8(bool value) noexcept
		: m_value(value)
	{
	}

	constexpr operator bool() const noexcept
	{
		return m_value != 0;
	}

	constexpr bool set(bool value) noexcept
	{
		m_value = value;
		return value;
	}
};

#ifndef _MSC_VER

using u128 = __uint128_t;
using s128 = __int128_t;

using __m128i = long long __attribute__((vector_size(16)));
using __m128d = double __attribute__((vector_size(16)));
using __m128 = float __attribute__((vector_size(16)));

#else

extern "C"
{
	union __m128;
	union __m128i;
	struct __m128d;

	uchar _addcarry_u64(uchar, u64, u64, u64*);
	uchar _subborrow_u64(uchar, u64, u64, u64*);
	u64 __shiftleft128(u64, u64, uchar);
	u64 __shiftright128(u64, u64, uchar);
}

// Unsigned 128-bit integer implementation (TODO)
struct alignas(16) u128
{
	u64 lo, hi;

	u128() noexcept = default;

	template <typename T, std::enable_if_t<std::is_unsigned_v<T>, u64> = 0>
	constexpr u128(T arg) noexcept
		: lo(arg)
		, hi(0)
	{
	}

	template <typename T, std::enable_if_t<std::is_signed_v<T>, s64> = 0>
	constexpr u128(T arg) noexcept
		: lo(s64{arg})
		, hi(s64{arg} >> 63)
	{
	}

	constexpr explicit operator bool() const noexcept
	{
		return !!(lo | hi);
	}

	constexpr explicit operator u64() const noexcept
	{
		return lo;
	}

	constexpr friend u128 operator+(const u128& l, const u128& r)
	{
		u128 value = l;
		value += r;
		return value;
	}

	constexpr friend u128 operator-(const u128& l, const u128& r)
	{
		u128 value = l;
		value -= r;
		return value;
	}

	constexpr u128 operator+() const
	{
		return *this;
	}

	constexpr u128 operator-() const
	{
		u128 value{};
		value -= *this;
		return value;
	}

	constexpr u128& operator++()
	{
		*this += 1;
		return *this;
	}

	constexpr u128 operator++(int)
	{
		u128 value = *this;
		*this += 1;
		return value;
	}

	constexpr u128& operator--()
	{
		*this -= 1;
		return *this;
	}

	constexpr u128 operator--(int)
	{
		u128 value = *this;
		*this -= 1;
		return value;
	}

	constexpr u128 operator<<(u128 shift_value) const
	{
		u128 value = *this;
		value <<= shift_value;
		return value;
	}

	constexpr u128 operator>>(u128 shift_value) const
	{
		u128 value = *this;
		value >>= shift_value;
		return value;
	}

	constexpr u128 operator~() const
	{
		u128 value{};
		value.lo = ~lo;
		value.hi = ~hi;
		return value;
	}

	constexpr friend u128 operator&(const u128& l, const u128& r)
	{
		u128 value{};
		value.lo = l.lo & r.lo;
		value.hi = l.hi & r.hi;
		return value;
	}

	constexpr friend u128 operator|(const u128& l, const u128& r)
	{
		u128 value{};
		value.lo = l.lo | r.lo;
		value.hi = l.hi | r.hi;
		return value;
	}

	constexpr friend u128 operator^(const u128& l, const u128& r)
	{
		u128 value{};
		value.lo = l.lo ^ r.lo;
		value.hi = l.hi ^ r.hi;
		return value;
	}

	constexpr u128& operator+=(const u128& r)
	{
		if (std::is_constant_evaluated())
		{
			lo += r.lo;
			hi += r.hi + (lo < r.lo);
		}
		else
		{
			_addcarry_u64(_addcarry_u64(0, r.lo, lo, &lo), r.hi, hi, &hi);
		}

		return *this;
	}

	constexpr u128& operator-=(const u128& r)
	{
		if (std::is_constant_evaluated())
		{
			hi -= r.hi + (lo < r.lo);
			lo -= r.lo;
		}
		else
		{
			_subborrow_u64(_subborrow_u64(0, lo, r.lo, &lo), hi, r.hi, &hi);
		}

		return *this;
	}

	constexpr u128& operator<<=(const u128& r)
	{
		if (std::is_constant_evaluated())
		{
			if (r.hi == 0 && r.lo < 64)
			{
				hi = (hi << r.lo) | (lo >> (64 - r.lo));
				lo = (lo << r.lo);
				return *this;
			}
			else if (r.hi == 0 && r.lo < 128)
			{
				hi = (lo << (r.lo - 64));
				lo = 0;
				return *this;
			}
		}

		const u64 v0 = lo << (r.lo & 63);
		const u64 v1 = __shiftleft128(lo, hi, static_cast<uchar>(r.lo));
		lo = (r.lo & 64) ? 0 : v0;
		hi = (r.lo & 64) ? v0 : v1;
		return *this;
	}

	constexpr u128& operator>>=(const u128& r)
	{
		if (std::is_constant_evaluated())
		{
			if (r.hi == 0 && r.lo < 64)
			{
				lo = (lo >> r.lo) | (hi << (64 - r.lo));
				hi = (hi >> r.lo);
				return *this;
			}
			else if (r.hi == 0 && r.lo < 128)
			{
				lo = (hi >> (r.lo - 64));
				hi = 0;
				return *this;
			}
		}

		const u64 v0 = hi >> (r.lo & 63);
		const u64 v1 = __shiftright128(lo, hi, static_cast<uchar>(r.lo));
		lo = (r.lo & 64) ? v0 : v1;
		hi = (r.lo & 64) ? 0 : v0;
		return *this;
	}

	constexpr u128& operator&=(const u128& r)
	{
		lo &= r.lo;
		hi &= r.hi;
		return *this;
	}

	constexpr u128& operator|=(const u128& r)
	{
		lo |= r.lo;
		hi |= r.hi;
		return *this;
	}

	constexpr u128& operator^=(const u128& r)
	{
		lo ^= r.lo;
		hi ^= r.hi;
		return *this;
	}
};

// Signed 128-bit integer implementation (TODO)
struct alignas(16) s128
{
	u64 lo;
	s64 hi;

	s128() = default;
};
#endif

template <>
struct get_int_impl<16>
{
	using utype = u128;
};

// Return magic value for any unsigned type
constexpr inline struct umax_helper
{
	constexpr umax_helper() noexcept = default;

	template <typename T, typename S = simple_t<T>, typename = std::enable_if_t<std::is_unsigned_v<S>>>
	constexpr bool operator==(const T& rhs) const
	{
		return rhs == static_cast<S>(-1);
	}

#if __cpp_impl_three_way_comparison >= 201711 && !__INTELLISENSE__
#else
	template <typename T>
	friend constexpr std::enable_if_t<std::is_unsigned_v<simple_t<T>>, bool> operator==(const T& lhs, const umax_helper& rhs)
	{
		return lhs == static_cast<simple_t<T>>(-1);
	}
#endif

#if __cpp_impl_three_way_comparison >= 201711
#else
	template <typename T, typename S = simple_t<T>, typename = std::enable_if_t<std::is_unsigned_v<S>>>
	constexpr bool operator!=(const T& rhs) const
	{
		return rhs != static_cast<S>(-1);
	}

	template <typename T>
	friend constexpr std::enable_if_t<std::is_unsigned_v<simple_t<T>>, bool> operator!=(const T& lhs, const umax_helper& rhs)
	{
		return lhs != static_cast<simple_t<T>>(-1);
	}
#endif
} umax;

enum class f16 : u16{};

using f32 = float;
using f64 = double;

template <typename T, typename T2>
inline u32 offset32(T T2::*const mptr)
{
#ifdef _MSC_VER
	return std::bit_cast<u32>(mptr);
#elif __GNUG__
	return std::bit_cast<usz>(mptr);
#else
	static_assert(sizeof(mptr) == 0, "Unsupported pointer-to-member size");
#endif
}

template <typename T>
struct offset32_array
{
	static_assert(std::is_array<T>::value, "Invalid pointer-to-member type (array expected)");

	template <typename Arg>
	static inline u32 index32(const Arg& arg)
	{
		return u32{sizeof(std::remove_extent_t<T>)} * static_cast<u32>(arg);
	}
};

template <typename T, usz N>
struct offset32_array<std::array<T, N>>
{
	template <typename Arg>
	static inline u32 index32(const Arg& arg)
	{
		return u32{sizeof(T)} * static_cast<u32>(arg);
	}
};

template <typename Arg>
struct offset32_detail;

template <typename T, typename T2, typename Arg, typename... Args>
inline u32 offset32(T T2::*const mptr, const Arg& arg, const Args&... args)
{
	return offset32_detail<Arg>::offset32(mptr, arg, args...);
}

template <typename Arg>
struct offset32_detail
{
	template <typename T, typename T2, typename... Args>
	static inline u32 offset32(T T2::*const mptr, const Arg& arg, const Args&... args)
	{
		return ::offset32(mptr, args...) + offset32_array<T>::index32(arg);
	}
};

template <typename T3, typename T4>
struct offset32_detail<T3 T4::*>
{
	template <typename T, typename T2, typename... Args>
	static inline u32 offset32(T T2::*const mptr, T3 T4::*const mptr2, const Args&... args)
	{
		return ::offset32(mptr) + ::offset32(mptr2, args...);
	}
};

// Helper function, used by ""_u16, ""_u32, ""_u64
constexpr u32 to_u8(char c)
{
	return static_cast<u8>(c);
}

// Convert 1-2-byte string to u16 value like reinterpret_cast does
constexpr u16 operator""_u16(const char* s, usz /*length*/)
{
	if constexpr (std::endian::little == std::endian::native)
	{
		return static_cast<u16>(to_u8(s[1]) << 8 | to_u8(s[0]));
	}
	else
	{
		return static_cast<u16>(to_u8(s[0]) << 8 | to_u8(s[1]));
	}
}

// Convert 3-4-byte string to u32 value like reinterpret_cast does
constexpr u32 operator""_u32(const char* s, usz /*length*/)
{
	if constexpr (std::endian::little == std::endian::native)
	{
		return to_u8(s[3]) << 24 | to_u8(s[2]) << 16 | to_u8(s[1]) << 8 | to_u8(s[0]);
	}
	else
	{
		return to_u8(s[0]) << 24 | to_u8(s[1]) << 16 | to_u8(s[2]) << 8 | to_u8(s[3]);
	}
}

// Convert 5-6-byte string to u64 value like reinterpret_cast does
constexpr u64 operator""_u48(const char* s, usz /*length*/)
{
	if constexpr (std::endian::little == std::endian::native)
	{
		return static_cast<u64>(to_u8(s[5]) << 8 | to_u8(s[4])) << 32 | to_u8(s[3]) << 24 | to_u8(s[2]) << 16 | to_u8(s[1]) << 8 | to_u8(s[0]);
	}
	else
	{
		return static_cast<u64>(to_u8(s[0]) << 8 | to_u8(s[1])) << 32 | to_u8(s[2]) << 24 | to_u8(s[3]) << 16 | to_u8(s[4]) << 8 | to_u8(s[5]);
	}
}

// Convert 7-8-byte string to u64 value like reinterpret_cast does
constexpr u64 operator""_u64(const char* s, usz /*length*/)
{
	if constexpr (std::endian::little == std::endian::native)
	{
		return static_cast<u64>(to_u8(s[7]) << 24 | to_u8(s[6]) << 16 | to_u8(s[5]) << 8 | to_u8(s[4])) << 32 | to_u8(s[3]) << 24 | to_u8(s[2]) << 16 | to_u8(s[1]) << 8 | to_u8(s[0]);
	}
	else
	{
		return static_cast<u64>(to_u8(s[0]) << 24 | to_u8(s[1]) << 16 | to_u8(s[2]) << 8 | to_u8(s[3])) << 32 | to_u8(s[4]) << 24 | to_u8(s[5]) << 16 | to_u8(s[6]) << 8 | to_u8(s[7]);
	}
}

#if !defined(__INTELLISENSE__) && !__has_builtin(__builtin_COLUMN) && !defined(_MSC_VER)
constexpr unsigned __builtin_COLUMN()
{
	return -1;
}
#endif

struct src_loc
{
	u32 line;
	u32 col;
	const char* file;
	const char* func;
};

namespace fmt
{
	[[noreturn]] void raw_verify_error(const src_loc& loc);
	[[noreturn]] void raw_narrow_error(const src_loc& loc);
}

template <typename T>
constexpr decltype(auto) ensure(T&& arg,
	u32 line = __builtin_LINE(),
	u32 col = __builtin_COLUMN(),
	const char* file = __builtin_FILE(),
	const char* func = __builtin_FUNCTION()) noexcept
{
	if (std::forward<T>(arg)) [[likely]]
	{
		return std::forward<T>(arg);
	}

	fmt::raw_verify_error({line, col, file, func});
}

// narrow() function details
template <typename From, typename To = void, typename = void>
struct narrow_impl
{
	// Temporarily (diagnostic)
	static_assert(std::is_void<To>::value, "narrow_impl<> specialization not found");

	// Returns true if value cannot be represented in type To
	static constexpr bool test(const From& value)
	{
		// Unspecialized cases (including cast to void) always considered narrowing
		return true;
	}
};

// Unsigned to unsigned narrowing
template <typename From, typename To>
struct narrow_impl<From, To, std::enable_if_t<std::is_unsigned<From>::value && std::is_unsigned<To>::value>>
{
	static constexpr bool test(const From& value)
	{
		return sizeof(To) < sizeof(From) && static_cast<To>(value) != value;
	}
};

// Signed to signed narrowing
template <typename From, typename To>
struct narrow_impl<From, To, std::enable_if_t<std::is_signed<From>::value && std::is_signed<To>::value>>
{
	static constexpr bool test(const From& value)
	{
		return sizeof(To) < sizeof(From) && static_cast<To>(value) != value;
	}
};

// Unsigned to signed narrowing
template <typename From, typename To>
struct narrow_impl<From, To, std::enable_if_t<std::is_unsigned<From>::value && std::is_signed<To>::value>>
{
	static constexpr bool test(const From& value)
	{
		return sizeof(To) <= sizeof(From) && value > (static_cast<std::make_unsigned_t<To>>(-1) >> 1);
	}
};

// Signed to unsigned narrowing (I)
template <typename From, typename To>
struct narrow_impl<From, To, std::enable_if_t<std::is_signed<From>::value && std::is_unsigned<To>::value && sizeof(To) >= sizeof(From)>>
{
	static constexpr bool test(const From& value)
	{
		return value < static_cast<From>(0);
	}
};

// Signed to unsigned narrowing (II)
template <typename From, typename To>
struct narrow_impl<From, To, std::enable_if_t<std::is_signed<From>::value && std::is_unsigned<To>::value && sizeof(To) < sizeof(From)>>
{
	static constexpr bool test(const From& value)
	{
		return static_cast<std::make_unsigned_t<From>>(value) > static_cast<To>(-1);
	}
};

// Simple type enabled (TODO: allow for To as well)
template <typename From, typename To>
struct narrow_impl<From, To, std::void_t<typename From::simple_type>>
	: narrow_impl<simple_t<From>, To>
{
};

template <typename To = void, typename From, typename = decltype(static_cast<To>(std::declval<From>()))>
[[nodiscard]] constexpr To narrow(const From& value,
	u32 line = __builtin_LINE(),
	u32 col = __builtin_COLUMN(),
	const char* file = __builtin_FILE(),
	const char* func = __builtin_FUNCTION())
{
	// Narrow check
	if (narrow_impl<From, To>::test(value)) [[unlikely]]
	{
		// Pack value as formatting argument
		fmt::raw_narrow_error({line, col, file, func});
	}

	return static_cast<To>(value);
}

// Returns u32 size() for container
template <typename CT, typename = decltype(static_cast<u32>(std::declval<CT>().size()))>
[[nodiscard]] constexpr u32 size32(const CT& container,
	u32 line = __builtin_LINE(),
	u32 col = __builtin_COLUMN(),
	const char* file = __builtin_FILE(),
	const char* func = __builtin_FUNCTION())
{
	return narrow<u32>(container.size(), line, col, file, func);
}

// Returns u32 size for an array
template <typename T, usz Size>
[[nodiscard]] constexpr u32 size32(const T (&)[Size])
{
	static_assert(Size < UINT32_MAX, "Array is too big for 32-bit");
	return static_cast<u32>(Size);
}

// Simplified hash algorithm. May be used in std::unordered_(map|set).
template <typename T, usz Shift = 0>
struct value_hash
{
	usz operator()(T value) const
	{
		return static_cast<usz>(value) >> Shift;
	}
};
