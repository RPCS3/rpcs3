#pragma once

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif
#include <immintrin.h>
#include <emmintrin.h>

#include <cstdint>
#include <type_traits>
#include <utility>
#include <chrono>
#include <array>

// Assume little-endian
#define IS_LE_MACHINE 1
#define IS_BE_MACHINE 0

#ifdef _MSC_VER
#define ASSUME(cond) __assume(cond)
#define LIKELY
#define UNLIKELY
#define SAFE_BUFFERS __declspec(safebuffers)
#define NEVER_INLINE __declspec(noinline)
#define FORCE_INLINE __forceinline
#else
#define ASSUME(cond) do { if (!(cond)) __builtin_unreachable(); } while (0)
#define LIKELY(cond) __builtin_expect(!!(cond), 1)
#define UNLIKELY(cond) __builtin_expect(!!(cond), 0)
#define SAFE_BUFFERS
#define NEVER_INLINE __attribute__((noinline))
#define FORCE_INLINE __attribute__((always_inline)) inline
#define thread_local __thread
#endif

#define CHECK_SIZE(type, size) static_assert(sizeof(type) == size, "Invalid " #type " type size")
#define CHECK_ALIGN(type, align) static_assert(alignof(type) == align, "Invalid " #type " type alignment")
#define CHECK_MAX_SIZE(type, size) static_assert(sizeof(type) <= size, #type " type size is too big")
#define CHECK_SIZE_ALIGN(type, size, align) CHECK_SIZE(type, size); CHECK_ALIGN(type, align)

// Return 32 bit sizeof() to avoid widening/narrowing conversions with size_t
#define SIZE_32(...) static_cast<u32>(sizeof(__VA_ARGS__))

// Return 32 bit alignof() to avoid widening/narrowing conversions with size_t
#define ALIGN_32(...) static_cast<u32>(alignof(__VA_ARGS__))

#define CONCATENATE_DETAIL(x, y) x ## y
#define CONCATENATE(x, y) CONCATENATE_DETAIL(x, y)

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)

#define HERE "\n(in file " __FILE__ ":" STRINGIZE(__LINE__) ")"

// Ensure that the expression evaluates to true. Obsolete.
//#define EXPECTS(...) do { if (!(__VA_ARGS__)) fmt::raw_error("Precondition failed: " #__VA_ARGS__ HERE); } while (0)
//#define ENSURES(...) do { if (!(__VA_ARGS__)) fmt::raw_error("Postcondition failed: " #__VA_ARGS__ HERE); } while (0)

#define DECLARE(...) decltype(__VA_ARGS__) __VA_ARGS__

#define STR_CASE(...) case __VA_ARGS__: return #__VA_ARGS__

using schar  = signed char;
using uchar  = unsigned char;
using ushort = unsigned short;
using uint   = unsigned int;
using ulong  = unsigned long;
using ullong = unsigned long long;
using llong  = long long;

using uptr = std::uintptr_t;

using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using s8  = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;

using steady_clock = std::conditional<
    std::chrono::high_resolution_clock::is_steady,
    std::chrono::high_resolution_clock, std::chrono::steady_clock>::type;

namespace gsl
{
	enum class byte : u8;
}

// Formatting helper, type-specific preprocessing for improving safety and functionality
template <typename T, typename = void>
struct fmt_unveil;

template <typename Arg>
using fmt_unveil_t = typename fmt_unveil<Arg>::type;

struct fmt_type_info;

namespace fmt
{
	template <typename... Args>
	const fmt_type_info* get_type_info();
}

template <typename T, std::size_t Align = alignof(T), std::size_t Size = sizeof(T)>
struct se_storage;

template <typename T, bool Se = true, std::size_t Align = alignof(T)>
class se_t;

template <typename T, std::size_t Size = sizeof(T)>
struct atomic_storage;

template <typename T1, typename T2, typename = void>
struct atomic_add;

template <typename T1, typename T2, typename = void>
struct atomic_sub;

template <typename T1, typename T2, typename = void>
struct atomic_and;

template <typename T1, typename T2, typename = void>
struct atomic_or;

template <typename T1, typename T2, typename = void>
struct atomic_xor;

template <typename T, typename = void>
struct atomic_pre_inc;

template <typename T, typename = void>
struct atomic_post_inc;

template <typename T, typename = void>
struct atomic_pre_dec;

template <typename T, typename = void>
struct atomic_post_dec;

template <typename T1, typename T2, typename = void>
struct atomic_test_and_set;

template <typename T1, typename T2, typename = void>
struct atomic_test_and_reset;

template <typename T1, typename T2, typename = void>
struct atomic_test_and_complement;

template <typename T>
class atomic_t;

#ifdef _MSC_VER
using std::void_t;
#else
namespace void_details
{
	template <typename...>
	struct make_void
	{
		using type = void;
	};
}

template <typename... T>
using void_t = typename void_details::make_void<T...>::type;
#endif

// Extract T::simple_type if available, remove cv qualifiers
template <typename T, typename = void>
struct simple_type_helper
{
	using type = typename std::remove_cv<T>::type;
};

template <typename T>
struct simple_type_helper<T, void_t<typename T::simple_type>>
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

	constexpr b8(bool value)
		: m_value(value)
	{
	}

	constexpr operator bool() const
	{
		return m_value != 0;
	}
};

// Bool wrapper for restricting bool result conversions
struct explicit_bool_t
{
	const bool value;

	constexpr explicit_bool_t(bool value)
		: value(value)
	{
	}

	explicit constexpr operator bool() const
	{
		return value;
	}
};

#ifndef _MSC_VER
using u128 = __uint128_t;
using s128 = __int128_t;
#else

// Unsigned 128-bit integer implementation (TODO)
struct alignas(16) u128
{
	u64 lo, hi;

	u128() = default;

	constexpr u128(u64 l)
		: lo(l)
		, hi(0)
	{
	}

	friend u128 operator+(const u128& l, const u128& r)
	{
		u128 value;
		_addcarry_u64(_addcarry_u64(0, r.lo, l.lo, &value.lo), r.hi, l.hi, &value.hi);
		return value;
	}

	friend u128 operator+(const u128& l, u64 r)
	{
		u128 value;
		_addcarry_u64(_addcarry_u64(0, r, l.lo, &value.lo), l.hi, 0, &value.hi);
		return value;
	}

	friend u128 operator+(u64 l, const u128& r)
	{
		u128 value;
		_addcarry_u64(_addcarry_u64(0, r.lo, l, &value.lo), 0, r.hi, &value.hi);
		return value;
	}

	friend u128 operator-(const u128& l, const u128& r)
	{
		u128 value;
		_subborrow_u64(_subborrow_u64(0, r.lo, l.lo, &value.lo), r.hi, l.hi, &value.hi);
		return value;
	}

	friend u128 operator-(const u128& l, u64 r)
	{
		u128 value;
		_subborrow_u64(_subborrow_u64(0, r, l.lo, &value.lo), 0, l.hi, &value.hi);
		return value;
	}

	friend u128 operator-(u64 l, const u128& r)
	{
		u128 value;
		_subborrow_u64(_subborrow_u64(0, r.lo, l, &value.lo), r.hi, 0, &value.hi);
		return value;
	}

	u128 operator+() const
	{
		return *this;
	}

	u128 operator-() const
	{
		u128 value;
		_subborrow_u64(_subborrow_u64(0, lo, 0, &value.lo), hi, 0, &value.hi);
		return value;
	}

	u128& operator++()
	{
		_addcarry_u64(_addcarry_u64(0, 1, lo, &lo), 0, hi, &hi);
		return *this;
	}

	u128 operator++(int)
	{
		u128 value = *this;
		_addcarry_u64(_addcarry_u64(0, 1, lo, &lo), 0, hi, &hi);
		return value;
	}

	u128& operator--()
	{
		_subborrow_u64(_subborrow_u64(0, 1, lo, &lo), 0, hi, &hi);
		return *this;
	}

	u128 operator--(int)
	{
		u128 value = *this;
		_subborrow_u64(_subborrow_u64(0, 1, lo, &lo), 0, hi, &hi);
		return value;
	}

	u128 operator~() const
	{
		u128 value;
		value.lo = ~lo;
		value.hi = ~hi;
		return value;
	}

	friend u128 operator&(const u128& l, const u128& r)
	{
		u128 value;
		value.lo = l.lo & r.lo;
		value.hi = l.hi & r.hi;
		return value;
	}

	friend u128 operator|(const u128& l, const u128& r)
	{
		u128 value;
		value.lo = l.lo | r.lo;
		value.hi = l.hi | r.hi;
		return value;
	}

	friend u128 operator^(const u128& l, const u128& r)
	{
		u128 value;
		value.lo = l.lo ^ r.lo;
		value.hi = l.hi ^ r.hi;
		return value;
	}

	u128& operator+=(const u128& r)
	{
		_addcarry_u64(_addcarry_u64(0, r.lo, lo, &lo), r.hi, hi, &hi);
		return *this;
	}

	u128& operator+=(uint64_t r)
	{
		_addcarry_u64(_addcarry_u64(0, r, lo, &lo), 0, hi, &hi);
		return *this;
	}

	u128& operator&=(const u128& r)
	{
		lo &= r.lo;
		hi &= r.hi;
		return *this;
	}

	u128& operator|=(const u128& r)
	{
		lo |= r.lo;
		hi |= r.hi;
		return *this;
	}

	u128& operator^=(const u128& r)
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

	constexpr s128(s64 l)
		: hi(l >> 63)
		, lo(l)
	{
	}

	constexpr s128(u64 l)
		: hi(0)
		, lo(l)
	{
	}
};
#endif

CHECK_SIZE_ALIGN(u128, 16, 16);
CHECK_SIZE_ALIGN(s128, 16, 16);

union alignas(2) f16
{
	u16 _u16;
	u8 _u8[2];

	explicit f16(u16 raw)
	{
		_u16 = raw;
	}

	explicit operator float() const
	{
		// See http://stackoverflow.com/a/26779139
		// The conversion doesn't handle NaN/Inf
		u32 raw = ((_u16 & 0x8000) << 16) |             // Sign (just moved)
		          (((_u16 & 0x7c00) + 0x1C000) << 13) | // Exponent ( exp - 15 + 127)
		          ((_u16 & 0x03FF) << 13);              // Mantissa
		return (float&)raw;
	}
};

CHECK_SIZE_ALIGN(f16, 2, 2);

using f32 = float;
using f64 = double;

struct ignore
{
	template <typename T>
	ignore(T)
	{
	}
};

template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
constexpr T align(const T& value, ullong align)
{
	return static_cast<T>((value + (align - 1)) & ~(align - 1));
}

template <typename T, typename T2>
inline u32 offset32(T T2::*const mptr)
{
#ifdef _MSC_VER
	static_assert(sizeof(mptr) == sizeof(u32), "Invalid pointer-to-member size");
	return reinterpret_cast<const u32&>(mptr);
#elif __GNUG__
	static_assert(sizeof(mptr) == sizeof(std::size_t), "Invalid pointer-to-member size");
	return static_cast<u32>(reinterpret_cast<const std::size_t&>(mptr));
#else
	static_assert(sizeof(mptr) == 0, "Invalid pointer-to-member size");
#endif
}

template <typename T>
struct offset32_array
{
	static_assert(std::is_array<T>::value, "Invalid pointer-to-member type (array expected)");

	template <typename Arg>
	static inline u32 index32(const Arg& arg)
	{
		return SIZE_32(std::remove_extent_t<T>) * static_cast<u32>(arg);
	}
};

template <typename T, std::size_t N>
struct offset32_array<std::array<T, N>>
{
	template <typename Arg>
	static inline u32 index32(const Arg& arg)
	{
		return SIZE_32(T) * static_cast<u32>(arg);
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

inline u32 cntlz32(u32 arg, bool nonzero = false)
{
#ifdef _MSC_VER
	ulong res;
	return _BitScanReverse(&res, arg) || nonzero ? res ^ 31 : 32;
#else
	return arg || nonzero ? __builtin_clzll(arg) - 32 : 32;
#endif
}

inline u64 cntlz64(u64 arg, bool nonzero = false)
{
#ifdef _MSC_VER
	ulong res;
	return _BitScanReverse64(&res, arg) || nonzero ? res ^ 63 : 64;
#else
	return arg || nonzero ? __builtin_clzll(arg) : 64;
#endif
}

// Helper function, used by ""_u16, ""_u32, ""_u64
constexpr u8 to_u8(char c)
{
	return static_cast<u8>(c);
}

// Convert 2-byte string to u16 value like reinterpret_cast does
constexpr u16 operator""_u16(const char* s, std::size_t length)
{
	return length != 2 ? throw s :
#if IS_LE_MACHINE == 1
		to_u8(s[1]) << 8 | to_u8(s[0]);
#endif
}

// Convert 4-byte string to u32 value like reinterpret_cast does
constexpr u32 operator""_u32(const char* s, std::size_t length)
{
	return length != 4 ? throw s :
#if IS_LE_MACHINE == 1
		to_u8(s[3]) << 24 | to_u8(s[2]) << 16 | to_u8(s[1]) << 8 | to_u8(s[0]);
#endif
}

// Convert 8-byte string to u64 value like reinterpret_cast does
constexpr u64 operator""_u64(const char* s, std::size_t length)
{
	return length != 8 ? throw s :
#if IS_LE_MACHINE == 1
		static_cast<u64>(to_u8(s[7]) << 24 | to_u8(s[6]) << 16 | to_u8(s[5]) << 8 | to_u8(s[4])) << 32 | to_u8(s[3]) << 24 | to_u8(s[2]) << 16 | to_u8(s[1]) << 8 | to_u8(s[0]);
#endif
}

namespace fmt
{
	[[noreturn]] void raw_error(const char* msg);
	[[noreturn]] void raw_verify_error(const char* msg, const fmt_type_info* sup, u64 arg);
	[[noreturn]] void raw_narrow_error(const char* msg, const fmt_type_info* sup, u64 arg);
}

struct verify_func
{
	template <typename T>
	bool operator()(T&& value) const
	{
		if (std::forward<T>(value))
		{
			return true;
		}

		return false;
	}
};

template <uint N>
struct verify_impl
{
	const char* cause;

	template <typename T>
	auto operator,(T&& value) const
	{
		// Verification (can be safely disabled)
		if (!verify_func()(std::forward<T>(value)))
		{
			fmt::raw_verify_error(cause, nullptr, N);
		}

		return verify_impl<N + 1>{cause};
	}
};

// Verification helper, checks several conditions delimited with comma operator
inline auto verify(const char* cause)
{
	return verify_impl<0>{cause};
}

// Verification helper (returns value or lvalue reference, may require to use verify_move instead)
template <typename F = verify_func, typename T>
inline T verify(const char* cause, T&& value, F&& pred = F())
{
	if (!pred(std::forward<T>(value)))
	{
		using unref = std::remove_const_t<std::remove_reference_t<T>>;
		fmt::raw_verify_error(cause, fmt::get_type_info<fmt_unveil_t<unref>>(), fmt_unveil<unref>::get(value));
	}

	return std::forward<T>(value);
}

// Verification helper (must be used in return expression or in place of std::move)
template <typename F = verify_func, typename T>
inline std::remove_reference_t<T>&& verify_move(const char* cause, T&& value, F&& pred = F())
{
	if (!pred(std::forward<T>(value)))
	{
		using unref = std::remove_const_t<std::remove_reference_t<T>>;
		fmt::raw_verify_error(cause, fmt::get_type_info<fmt_unveil_t<unref>>(), fmt_unveil<unref>::get(value));
	}

	return std::move(value);
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
struct narrow_impl<From, To, void_t<typename From::simple_type>>
	: narrow_impl<simple_t<From>, To>
{
};

template <typename To = void, typename From, typename = decltype(static_cast<To>(std::declval<From>()))>
inline To narrow(const From& value, const char* msg = nullptr)
{
	// Narrow check
	if (narrow_impl<From, To>::test(value))
	{
		// Pack value as formatting argument
		fmt::raw_narrow_error(msg, fmt::get_type_info<fmt_unveil_t<From>>(), fmt_unveil<From>::get(value));
	}

	return static_cast<To>(value);
}

// Returns u32 size() for container
template <typename CT, typename = decltype(static_cast<u32>(std::declval<CT>().size()))>
inline u32 size32(const CT& container, const char* msg = nullptr)
{
	return narrow<u32>(container.size(), msg);
}

// Returns u32 size for an array
template <typename T, std::size_t Size>
constexpr u32 size32(const T (&)[Size], const char* msg = nullptr)
{
	return static_cast<u32>(Size);
}

template <typename T1, typename = std::enable_if_t<std::is_integral<T1>::value>>
constexpr bool test(const T1& value)
{
	return value != 0;
}

template <typename T1, typename T2, typename = std::enable_if_t<std::is_integral<T1>::value && std::is_integral<T2>::value>>
constexpr bool test(const T1& lhs, const T2& rhs)
{
	return (lhs & rhs) != 0;
}

template <typename T, typename T2, typename = std::enable_if_t<std::is_integral<T>::value && std::is_integral<T2>::value>>
inline bool test_and_set(T& lhs, const T2& rhs)
{
	const bool result = (lhs & rhs) != 0;
	lhs |= rhs;
	return result;
}

template <typename T, typename T2, typename = std::enable_if_t<std::is_integral<T>::value && std::is_integral<T2>::value>>
inline bool test_and_reset(T& lhs, const T2& rhs)
{
	const bool result = (lhs & rhs) != 0;
	lhs &= ~rhs;
	return result;
}

template <typename T, typename T2, typename = std::enable_if_t<std::is_integral<T>::value && std::is_integral<T2>::value>>
inline bool test_and_complement(T& lhs, const T2& rhs)
{
	const bool result = (lhs & rhs) != 0;
	lhs ^= rhs;
	return result;
}

// Simplified hash algorithm for pointers. May be used in std::unordered_(map|set).
template <typename T, std::size_t Align = alignof(T)>
struct pointer_hash
{
	std::size_t operator()(T* ptr) const
	{
		return reinterpret_cast<std::uintptr_t>(ptr) / Align;
	}
};

template <typename T, std::size_t Shift = 0>
struct value_hash
{
	std::size_t operator()(T value) const
	{
		return static_cast<std::size_t>(value) >> Shift;
	}
};

// Contains value of any POD type with fixed size and alignment. TT<> is the type converter applied.
// For example, `simple_t` may be used to remove endianness.
template <template <typename> class TT, std::size_t S, std::size_t A = S>
struct alignas(A) any_pod
{
	std::aligned_storage_t<S, A> data;

	any_pod() = default;

	template <typename T, typename T2 = TT<T>, typename = std::enable_if_t<std::is_pod<T2>::value && sizeof(T2) == S && alignof(T2) <= A>>
	any_pod(const T& value)
	{
		reinterpret_cast<T2&>(data) = value;
	}

	template <typename T, typename T2 = TT<T>, typename = std::enable_if_t<std::is_pod<T2>::value && sizeof(T2) == S && alignof(T2) <= A>>
	T2& as()
	{
		return reinterpret_cast<T2&>(data);
	}

	template <typename T, typename T2 = TT<T>, typename = std::enable_if_t<std::is_pod<T2>::value && sizeof(T2) == S && alignof(T2) <= A>>
	const T2& as() const
	{
		return reinterpret_cast<const T2&>(data);
	}
};

using any16 = any_pod<simple_t, sizeof(u16)>;
using any32 = any_pod<simple_t, sizeof(u32)>;
using any64 = any_pod<simple_t, sizeof(u64)>;

struct cmd64 : any64
{
	struct pair_t
	{
		any32 arg1;
		any32 arg2;
	};

	cmd64() = default;

	template <typename T>
	cmd64(const T& value)
		: any64(value)
	{
	}

	template <typename T1, typename T2>
	cmd64(const T1& arg1, const T2& arg2)
		: any64(pair_t{arg1, arg2})
	{
	}

	explicit operator bool() const
	{
		return as<u64>() != 0;
	}

	// TODO: compatibility with std::pair/std::tuple?

	template <typename T>
	decltype(auto) arg1()
	{
		return as<pair_t>().arg1.as<T>();
	}

	template <typename T>
	decltype(auto) arg1() const
	{
		return as<const pair_t>().arg1.as<const T>();
	}

	template <typename T>
	decltype(auto) arg2()
	{
		return as<pair_t>().arg2.as<T>();
	}

	template <typename T>
	decltype(auto) arg2() const
	{
		return as<const pair_t>().arg2.as<const T>();
	}
};

static_assert(sizeof(cmd64) == 8 && std::is_pod<cmd64>::value, "Incorrect cmd64 type");

// Allows to define integer convertible to multiple types
template <typename T, T Value, typename T1 = void, typename... Ts>
struct multicast : multicast<T, Value, Ts...>
{
	constexpr multicast()
		: multicast<T, Value, Ts...>()
	{
	}

	// Implicit conversion to desired type
	constexpr operator T1() const
	{
		return static_cast<T1>(Value);
	}
};

// Recursion terminator
template <typename T, T Value>
struct multicast<T, Value, void>
{
	constexpr multicast() = default;

	// Explicit conversion to base type
	explicit constexpr operator T() const
	{
		return Value;
	}
};

// Error code type (return type), implements error reporting. Could be a template.
struct error_code
{
	// Use fixed s32 type for now
	s32 value;

	error_code() = default;

	// Implementation must be provided specially
	static s32 error_report(const fmt_type_info* sup, u64 arg);

	// Helper type
	enum class not_an_error : s32
	{
		__not_an_error // SFINAE marker
	};

	// __not_an_error tester
	template<typename ET, typename = void>
	struct is_error : std::integral_constant<bool, std::is_enum<ET>::value || std::is_integral<ET>::value>
	{
	};

	template<typename ET>
	struct is_error<ET, void_t<decltype(ET::__not_an_error)>> : std::false_type
	{
	};

	// Not an error constructor
	template<typename ET, typename = decltype(ET::__not_an_error)>
	error_code(const ET& value, int = 0)
		: value(static_cast<s32>(value))
	{
	}

	// Error constructor
	template<typename ET, typename = std::enable_if_t<is_error<ET>::value>>
	error_code(const ET& value)
		: value(error_report(fmt::get_type_info<fmt_unveil_t<ET>>(), fmt_unveil<ET>::get(value)))
	{
	}

	operator s32() const
	{
		return value;
	}
};

// Helper function for error_code
template <typename T>
constexpr FORCE_INLINE error_code::not_an_error not_an_error(const T& value)
{
	return static_cast<error_code::not_an_error>(static_cast<s32>(value));
}

// Synchronization helper (cache-friendly busy waiting)
inline void busy_wait(std::size_t count = 100)
{
	while (count--) _mm_pause();
}

// Rotate helpers
#if defined(__GNUG__)

inline u8 rol8(u8 x, u8 n)
{
	u8 result = x;
	__asm__("rolb %[n], %[result]" : [result] "+g" (result) : [n] "c" (n));
	return result;
}

inline u8 ror8(u8 x, u8 n)
{
	u8 result = x;
	__asm__("rorb %[n], %[result]" : [result] "+g" (result) : [n] "c" (n));
	return result;
}

inline u16 rol16(u16 x, u16 n)
{
	u16 result = x;
	__asm__("rolw %b[n], %[result]" : [result] "+g" (result) : [n] "c" (n));
	return result;
}

inline u16 ror16(u16 x, u16 n)
{
	u16 result = x;
	__asm__("rorw %b[n], %[result]" : [result] "+g" (result) : [n] "c" (n));
	return result;
}

inline u32 rol32(u32 x, u32 n)
{
	u32 result = x;
	__asm__("roll %b[n], %[result]" : [result] "+g" (result) : [n] "c" (n));
	return result;
}

inline u32 ror32(u32 x, u32 n)
{
	u32 result = x;
	__asm__("rorl %b[n], %[result]" : [result] "+g" (result) : [n] "c" (n));
	return result;
}

inline u64 rol64(u64 x, u64 n)
{
	u64 result = x;
	__asm__("rolq %b[n], %[result]" : [result] "+g" (result) : [n] "c" (n));
	return result;
}

inline u64 ror64(u64 x, u64 n)
{
	u64 result = x;
	__asm__("rorq %b[n], %[result]" : [result] "+g" (result) : [n] "c" (n));
	return result;
}

inline u64 umulh64(u64 a, u64 b)
{
	u64 result;
	__asm__("mulq %[b]" : "=d" (result) : [a] "a" (a), [b] "rm" (b));
	return result;
}

inline s64 mulh64(s64 a, s64 b)
{
	s64 result;
	__asm__("imulq %[b]" : "=d" (result) : [a] "a" (a), [b] "rm" (b));
	return result;
}

#elif defined(_MSC_VER)
inline u8 rol8(u8 x, u8 n) { return _rotl8(x, n); }
inline u8 ror8(u8 x, u8 n) { return _rotr8(x, n); }
inline u16 rol16(u16 x, u16 n) { return _rotl16(x, (u8)n); }
inline u16 ror16(u16 x, u16 n) { return _rotr16(x, (u8)n); }
inline u32 rol32(u32 x, u32 n) { return _rotl(x, (int)n); }
inline u32 ror32(u32 x, u32 n) { return _rotr(x, (int)n); }
inline u64 rol64(u64 x, u64 n) { return _rotl64(x, (int)n); }
inline u64 ror64(u64 x, u64 n) { return _rotr64(x, (int)n); }
inline u64 umulh64(u64 x, u64 y) { return __umulh(x, y); }
inline s64 mulh64(s64 x, s64 y) { return __mulh(x, y); }
#endif
