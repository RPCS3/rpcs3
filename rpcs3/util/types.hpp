#pragma once // No BOM and only basic ASCII in this header, or a neko will die

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <utility>
#include <chrono>
#include <array>
#include <tuple>
#include <compare>
#include <memory>
#include <bit>

using std::chrono::steady_clock;

using namespace std::literals;

#ifndef __has_builtin
	#define __has_builtin(x) 0
#endif

#ifdef _MSC_VER
#define SAFE_BUFFERS(...) __declspec(safebuffers) __VA_ARGS__
#define NEVER_INLINE __declspec(noinline)
#define FORCE_INLINE __forceinline
#else // not _MSC_VER
#ifdef __clang__
#define SAFE_BUFFERS(...) __attribute__((no_stack_protector)) __VA_ARGS__
#else
#define SAFE_BUFFERS(...) __VA_ARGS__ __attribute__((__optimize__("no-stack-protector")))
#endif
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
	template <typename To, typename From>
	[[nodiscard]] constexpr To bit_cast(const From& from) noexcept
	{
		return __builtin_bit_cast(To, from);
	}
}
#endif

#if defined(__INTELLISENSE__)
#define consteval constexpr
#endif

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
using usz = std::size_t;

using s8  = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;

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
	u64 _umul128(u64, u64, u64*);
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

	constexpr explicit operator s64() const noexcept
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

	constexpr friend u128 operator*(const u128& l, const u128& r)
	{
		u128 value = l;
		value *= r;
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

	constexpr u128& operator*=(const u128& r)
	{
		const u64 _hi = r.hi * lo + r.lo * hi;

		if (std::is_constant_evaluated())
		{
			hi = (lo >> 32) * (r.lo >> 32) + (((lo >> 32) * (r.lo & 0xffffffff)) >> 32) + (((r.lo >> 32) * (lo & 0xffffffff)) >> 32);
			lo = lo * r.lo;
		}
		else
		{
			lo = _umul128(lo, r.lo, &hi);
		}

		hi += _hi;
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

// Signed 128-bit integer implementation
struct s128 : u128
{
	using u128::u128;

	constexpr s128 operator>>(u128 shift_value) const
	{
		s128 value = *this;
		value >>= shift_value;
		return value;
	}

	constexpr s128& operator>>=(const u128& r)
	{
		if (std::is_constant_evaluated())
		{
			if (r.hi == 0 && r.lo < 64)
			{
				lo = (lo >> r.lo) | (hi << (64 - r.lo));
				hi = (static_cast<s64>(hi) >> r.lo);
				return *this;
			}
			else if (r.hi == 0 && r.lo < 128)
			{
				s64 _lo = static_cast<s64>(hi) >> (r.lo - 64);
				lo = _lo;
				hi = _lo >> 63;
				return *this;
			}
		}

		const u64 v0 = static_cast<s64>(hi) >> (r.lo & 63);
		const u64 v1 = __shiftright128(lo, hi, static_cast<uchar>(r.lo));
		lo = (r.lo & 64) ? v0 : v1;
		hi = (r.lo & 64) ? static_cast<s64>(hi) >> 63 : v0;
		return *this;
	}
};
#endif

template <>
struct get_int_impl<16>
{
	using utype = u128;
	using stype = s128;
};

enum class f16 : u16{};

using f32 = float;
using f64 = double;

template <typename T>
concept UnsignedInt = std::is_unsigned_v<std::common_type_t<T>> || std::is_same_v<std::common_type_t<T>, u128>;

template <typename T>
concept SignedInt = (std::is_signed_v<std::common_type_t<T>> && std::is_integral_v<std::common_type_t<T>>) || std::is_same_v<std::common_type_t<T>, s128>;

template <typename T>
concept FPInt = std::is_floating_point_v<std::common_type_t<T>> || std::is_same_v<std::common_type_t<T>, f16>;

template <typename T>
constexpr T min_v;

template <UnsignedInt T>
constexpr std::common_type_t<T> min_v<T> = 0;

template <SignedInt T>
constexpr std::common_type_t<T> min_v<T> = static_cast<std::common_type_t<T>>(-1) << (sizeof(std::common_type_t<T>) * 8 - 1);

template <>
constexpr inline f16 min_v<f16>{0xfbffu};

template <>
constexpr inline f32 min_v<f32> = std::bit_cast<f32, u32>(0xff'7fffffu);

template <>
constexpr inline f64 min_v<f64> = std::bit_cast<f64, u64>(0xffe'7ffff'ffffffffu);

template <FPInt T>
constexpr std::common_type_t<T> min_v<T> = min_v<std::common_type_t<T>>;

template <typename T>
constexpr T max_v;

template <UnsignedInt T>
constexpr std::common_type_t<T> max_v<T> = -1;

template <SignedInt T>
constexpr std::common_type_t<T> max_v<T> = static_cast<std::common_type_t<T>>(~min_v<T>);

template <>
constexpr inline f16 max_v<f16>{0x7bffu};

template <>
constexpr inline f32 max_v<f32> = std::bit_cast<f32, u32>(0x7f'7fffffu);

template <>
constexpr inline f64 max_v<f64> = std::bit_cast<f64, u64>(0x7fe'fffff'ffffffffu);

template <FPInt T>
constexpr std::common_type_t<T> max_v<T> = max_v<std::common_type_t<T>>;

// Return magic value for any unsigned type
constexpr struct umax_impl_t
{
	template <UnsignedInt T>
	constexpr bool operator==(const T& rhs) const
	{
		return rhs == max_v<T>;
	}

	template <UnsignedInt T>
	constexpr std::strong_ordering operator<=>(const T& rhs) const
	{
		return rhs == max_v<T> ? std::strong_ordering::equal : std::strong_ordering::greater;
	}

	template <UnsignedInt T>
	constexpr operator T() const
	{
		return max_v<T>;
	}
} umax;

constexpr struct smin_impl_t
{
	template <SignedInt T>
	constexpr bool operator==(const T& rhs) const
	{
		return rhs == min_v<T>;
	}

	template <SignedInt T>
	constexpr std::strong_ordering operator<=>(const T& rhs) const
	{
		return rhs == min_v<T> ? std::strong_ordering::equal : std::strong_ordering::less;
	}

	template <SignedInt T>
	constexpr operator T() const
	{
		return min_v<T>;
	}
} smin;

constexpr struct smax_impl_t
{
	template <SignedInt T>
	constexpr bool operator==(const T& rhs) const
	{
		return rhs == max_v<T>;
	}

	template <SignedInt T>
	constexpr std::strong_ordering operator<=>(const T& rhs) const
	{
		return rhs == max_v<T> ? std::strong_ordering::equal : std::strong_ordering::greater;
	}

	template <SignedInt T>
	constexpr operator T() const
	{
		return max_v<T>;
	}
} smax;

// Compare signed or unsigned type with its max value
constexpr struct amax_impl_t
{
	template <typename T> requires SignedInt<T> || UnsignedInt<T>
	constexpr bool operator ==(const T& rhs) const
	{
		return rhs == max_v<T>;
	}

	template <typename T> requires SignedInt<T> || UnsignedInt<T>
	constexpr std::strong_ordering operator <=>(const T& rhs) const
	{
		return max_v<T> <=> rhs;
	}

	template <typename T> requires SignedInt<T> || UnsignedInt<T>
	constexpr operator T() const
	{
		return max_v<T>;
	}
} amax;

// Compare signed or unsigned type with its minimal value (like zero or INT_MIN)
constexpr struct amin_impl_t
{
	template <typename T> requires SignedInt<T> || UnsignedInt<T>
	constexpr bool operator ==(const T& rhs) const
	{
		return rhs == min_v<T>;
	}

	template <typename T> requires SignedInt<T> || UnsignedInt<T>
	constexpr std::strong_ordering operator <=>(const T& rhs) const
	{
		return min_v<T> <=> rhs;
	}

	template <typename T> requires SignedInt<T> || UnsignedInt<T>
	constexpr operator T() const
	{
		return min_v<T>;
	}
} amin;

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

// Convert 0-2-byte string to u16 value like reinterpret_cast does
constexpr u16 operator""_u16(const char* s, usz /*length*/)
{
	char buf[2]{s[0], s[1]};
	return std::bit_cast<u16>(buf);
}

// Convert 3-4-byte string to u32 value like reinterpret_cast does
constexpr u32 operator""_u32(const char* s, usz /*length*/)
{
	char buf[4]{s[0], s[1], s[2], s[3]};
	return std::bit_cast<u32>(buf);
}

// Convert 5-8-byte string to u64 value like reinterpret_cast does
constexpr u64 operator""_u64(const char* s, usz len)
{
	char buf[8]{s[0], s[1], s[2], s[3], s[4], (len < 6 ? '\0' : s[5]), (len < 7 ? '\0' : s[6]), (len < 8 ? '\0' : s[7])};
	return std::bit_cast<u64>(buf);
}

#if !defined(__INTELLISENSE__) && !__has_builtin(__builtin_COLUMN) && !defined(_MSC_VER)
constexpr unsigned __builtin_COLUMN()
{
	return -1;
}
#endif

template <usz Size = usz(-1)>
struct const_str_t
{
	static constexpr usz size = Size;

	char8_t chars[Size + 1]{};

	constexpr const_str_t(const char(&a)[Size + 1])
	{
		for (usz i = 0; i <= Size; i++)
			chars[i] = a[i];
	}

	constexpr const_str_t(const char8_t(&a)[Size + 1])
	{
		for (usz i = 0; i <= Size; i++)
			chars[i] = a[i];
	}

	operator const char*() const
	{
		return reinterpret_cast<const char*>(chars);
	}

	constexpr operator const char8_t*() const
	{
		return chars;
	}
};

template <>
struct const_str_t<usz(-1)>
{
	const usz size;

	const union
	{
		const char8_t* chars;
		const char* chars2;
	};

	template <usz N>
	constexpr const_str_t(const char8_t(&a)[N])
		: size(N - 1)
		, chars(+a)
	{
	}

	template <usz N>
	constexpr const_str_t(const char(&a)[N])
		: size(N - 1)
		, chars2(+a)
	{
	}

	operator const char*() const
	{
		return std::launder(chars2);
	}

	constexpr operator const char8_t*() const
	{
		return chars;
	}
};

template <usz Size>
const_str_t(const char(&a)[Size]) -> const_str_t<Size - 1>;

template <usz Size>
const_str_t(const char8_t(&a)[Size]) -> const_str_t<Size - 1>;

using const_str = const_str_t<>;

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
	static constexpr bool test(const From&)
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
struct narrow_impl<From, To, std::enable_if_t<!std::is_same_v<std::common_type_t<From>, From>>>
	: narrow_impl<std::common_type_t<From>, To>
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
	static_assert(Size < u32{umax}, "Array is too big for 32-bit");
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

template <typename... T>
struct fill_array_t
{
	std::tuple<T...> args;

	template <typename V, usz Num>
	constexpr std::unwrap_reference_t<V> get() const
	{
		return std::get<Num>(args);
	}

	template <typename U, usz N, usz... M, usz... Idx>
	constexpr std::array<U, N> fill(std::index_sequence<M...>, std::index_sequence<Idx...>) const
	{
		return{(static_cast<void>(Idx), U(get<T, M>()...))...};
	}

	template <typename U, usz N>
	constexpr operator std::array<U, N>() const
	{
		return fill<U, N>(std::make_index_sequence<sizeof...(T)>(), std::make_index_sequence<N>());
	}
};

template <typename... T>
constexpr auto fill_array(const T&... args)
{
	return fill_array_t<T...>{{args...}};
}

template <typename X, typename Y>
concept PtrCastable = requires(const volatile X* x, const volatile Y* y)
{
	static_cast<const volatile Y*>(x);
	static_cast<const volatile X*>(y);
};

template <typename X, typename Y> requires PtrCastable<X, Y>
constexpr bool is_same_ptr()
{
	if constexpr (std::is_void_v<X> || std::is_void_v<Y> || std::is_same_v<std::remove_cv_t<X>, std::remove_cv_t<Y>>)
	{
		return true;
	}
	else if constexpr (sizeof(X) == sizeof(Y))
	{
		return true;
	}
	else
	{
		if (std::is_constant_evaluated())
		{
			bool result = false;

			if constexpr (sizeof(X) < sizeof(Y))
			{
				std::allocator<Y> a{};
				Y* ptr = a.allocate(1);
				result = static_cast<X*>(ptr) == static_cast<void*>(ptr);
				a.deallocate(ptr, 1);
			}
			else
			{
				std::allocator<X> a{};
				X* ptr = a.allocate(1);
				result = static_cast<Y*>(ptr) == static_cast<void*>(ptr);
				a.deallocate(ptr, 1);
			}

			return result;
		}
		else
		{
			std::aligned_union_t<0, X, Y> s;
			Y* ptr = reinterpret_cast<Y*>(&s);
			return static_cast<X*>(ptr) == static_cast<void*>(ptr);
		}
	}
}

template <typename X, typename Y> requires PtrCastable<X, Y>
constexpr bool is_same_ptr(const volatile Y* ptr)
{
	return static_cast<const volatile X*>(ptr) == static_cast<const volatile void*>(ptr);
}

template <typename X, typename Y>
concept PtrSame = (is_same_ptr<X, Y>());

namespace utils
{
	struct serial;
}

template <typename T>
extern bool serialize(utils::serial& ar, T& obj);
