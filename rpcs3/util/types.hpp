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
#include <string>
#include <source_location>

#if defined(__SSE2__) || defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__) || defined(__amd64__)
#define ARCH_X64 1
#elif defined(__aarch64__) || defined(__arm64__) || defined(_M_ARM64)
#define ARCH_ARM64 1
// v8.4a+ gives us atomic 16 byte ld/st
// See Arm C Language Extensions Documentation
// Currently there is no feature macro for LSE2 specifically so we define it ourself
// Unfortunately the __ARM_ARCH integer macro isn't universally defined so we use this hack instead
#if defined(__ARM_ARCH_8_4__) || defined(__ARM_ARCH_8_5__) || defined(__ARM_ARCH_8_6__) || defined(__ARM_ARCH_9__)
#define ARM_FEATURE_LSE2 1
#endif
#endif

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
#define AUDIT(...) (static_cast<std::void_t<decltype((__VA_ARGS__))>>(0))
#endif

namespace utils
{
	template <typename F>
	struct fn_helper
	{
		F f;

		fn_helper(F&& f)
			: f(std::forward<F>(f))
		{
		}

		template <typename... Args>
		auto operator()(Args&&... args) const
		{
			if constexpr (sizeof...(Args) == 0)
				return f(0, 0, 0, 0);
			else if constexpr (sizeof...(Args) == 1)
				return f(std::forward<Args>(args)..., 0, 0, 0);
			else if constexpr (sizeof...(Args) == 2)
				return f(std::forward<Args>(args)..., 0, 0);
			else if constexpr (sizeof...(Args) == 3)
				return f(std::forward<Args>(args)..., 0);
			else if constexpr (sizeof...(Args) == 4)
				return f(std::forward<Args>(args)...);
			else
				static_assert(sizeof...(Args) <= 4);
		}
	};

	template <typename F>
	fn_helper(F&& f) -> fn_helper<F>;
}

// Shorter lambda.
#define FN(...) \
	::utils::fn_helper([&]( \
		[[maybe_unused]] auto&& x, \
		[[maybe_unused]] auto&& y, \
		[[maybe_unused]] auto&& z, \
		[[maybe_unused]] auto&& w){ return (__VA_ARGS__); })

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

#if defined(__INTELLISENSE__) || (defined (__clang__) && (__clang_major__ <= 16))
#define consteval constexpr
#define constinit
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
using ssz = std::make_signed_t<std::size_t>;

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

	template <typename T>
	struct lazy;

	template <typename T>
	struct generator;
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

// Removes be_t<> wrapper from type be_<T> with nop fallback for unwrapped T
template<typename T>
struct remove_be { using type = T; };
template<typename T>
struct remove_be<be_t<T>> { using type = T; };
template<typename T>
using remove_be_t = typename remove_be<T>::type;

// Bool type equivalent
class b8
{
	u8 m_value;

public:
	b8() = default;

	using enable_bitcopy = std::true_type;

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

#if defined(ARCH_X64) && !defined(_MSC_VER)
using __m128i = long long __attribute__((vector_size(16)));
using __m128d = double __attribute__((vector_size(16)));
using __m128 = float __attribute__((vector_size(16)));
#endif

#ifndef _MSC_VER
using u128 = __uint128_t;
using s128 = __int128_t;
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

	template <typename T>
		requires std::is_unsigned_v<T>
	constexpr u128(T arg) noexcept
		: lo(arg)
		, hi(0)
	{
	}

	template <typename T>
		requires std::is_signed_v<T>
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

// Optimization for u64*u64=u128
constexpr u128 u128_from_mul(u64 a, u64 b)
{
#ifdef _MSC_VER
	if (!std::is_constant_evaluated())
	{
		u64 hi;
		u128 result = _umul128(a, b, &hi);
		result.hi = hi;
		return result;
	}
#endif

	return u128{a} * b;
}

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
concept Integral = std::is_integral_v<std::common_type_t<T>> || std::is_same_v<std::common_type_t<T>, u128> || std::is_same_v<std::common_type_t<T>, s128>;

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
	static_assert(std::is_array_v<T>, "Invalid pointer-to-member type (array expected)");

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

template <usz Size = umax>
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
struct const_str_t<umax>
{
	const usz size;

	union
	{
		const char8_t* chars;
		const char* chars2;
	};

	constexpr const_str_t()
		: size(0)
		, chars(nullptr)
	{
	}

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

	constexpr operator const char*() const
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

namespace fmt
{
	[[noreturn]] void raw_verify_error(std::source_location loc, const char8_t* msg, usz object);
	[[noreturn]] void raw_range_error(std::source_location loc, std::string_view index, usz container_size);
	[[noreturn]] void raw_range_error(std::source_location loc, usz index, usz container_size);
}

// No full implementation to ease on header weight
template <typename T>
std::conditional_t<std::is_integral_v<std::remove_cvref_t<T>>, usz, std::string_view> format_object_simplified(const T& obj)
{
	using type = std::remove_cvref_t<T>;

	if constexpr (std::is_integral_v<type> || std::is_same_v<std::string, type> || std::is_same_v<std::string_view, type>)
	{
		return obj;
	}
	else if constexpr (std::is_array_v<type> && std::is_constructible_v<std::string_view, type>)
	{
		return { obj, std::size(obj) - 1 };
	}
	else
	{
		return std::string_view{};
	}
}

template <typename T>
constexpr decltype(auto) ensure(T&& arg, const_str msg = const_str(), std::source_location src_loc = std::source_location::current()) noexcept
{
	if (std::forward<T>(arg)) [[likely]]
	{
		return std::forward<T>(arg);
	}

	fmt::raw_verify_error(src_loc, msg, 0);
}

template <typename T, typename F> requires (std::is_invocable_v<F, T&&>)
constexpr decltype(auto) ensure(T&& arg, F&& pred, const_str msg = const_str(), std::source_location src_loc = std::source_location::current()) noexcept
{
	if (std::forward<F>(pred)(std::forward<T>(arg))) [[likely]]
	{
		return std::forward<T>(arg);
	}

	fmt::raw_verify_error(src_loc, msg, 0);
}

template <typename To, typename From> requires (std::is_integral_v<decltype(std::declval<To>() + std::declval<From>())>)
[[nodiscard]] constexpr To narrow(const From& value, std::source_location src_loc = std::source_location::current())
{
	// Narrow check
	using CommonFrom = std::common_type_t<From>;
	using CommonTo = std::common_type_t<To>;

	using UnFrom = std::make_unsigned_t<CommonFrom>;
	using UnTo = std::make_unsigned_t<CommonTo>;

	constexpr bool is_from_signed = std::is_signed_v<CommonFrom>;
	constexpr bool is_to_signed = std::is_signed_v<CommonTo>;

	// For unsigned/signed mismatch, create an "unsigned" compatible mask
	constexpr auto from_mask = (is_from_signed && !is_to_signed && sizeof(CommonFrom) <= sizeof(CommonTo)) ? UnFrom{umax} >> 1 : UnFrom{umax};
	constexpr auto to_mask = (is_to_signed && !is_from_signed) ? UnTo{umax} >> 1 : UnTo{umax};

	constexpr auto mask = static_cast<UnFrom>(~(from_mask & to_mask));

	// If destination ("unsigned" compatible) mask is smaller than source ("unsigned" compatible) mask
	// It requires narrowing.
	if constexpr (!!mask)
	{
		// Try to optimize test if both are of the same signedness
		if (is_from_signed != is_to_signed ? !!(value & mask) : static_cast<CommonFrom>(static_cast<CommonTo>(value)) != value) [[unlikely]]
		{
			fmt::raw_verify_error(src_loc, u8"Narrowing error", +value);
		}
	}

	return static_cast<To>(value);
}

// Returns u32 size() for container
template <typename CT> requires requires (const CT& x) { std::size(x); }
[[nodiscard]] constexpr u32 size32(const CT& container, std::source_location src_loc = std::source_location::current())
{
	// TODO: Support std::array
	constexpr bool is_const = std::is_array_v<std::remove_cvref_t<CT>>;

	if constexpr (is_const)
	{
		constexpr usz Size = sizeof(container) / sizeof(container[0]);
		return std::conditional_t<is_const, u32, usz>{Size};
	}
	else
	{
		return narrow<u32>(container.size(), src_loc);
	}
}

template <typename CT, typename T> requires requires (CT&& x) { std::size(x); std::data(x); } || requires (CT&& x) { std::size(x); x.front(); }
[[nodiscard]] constexpr auto& at32(CT&& container, T&& index, std::source_location src_loc = std::source_location::current())
{
	// Make sure the index is within u32 range
	const std::make_unsigned_t<std::common_type_t<T>> idx = index;
	const u32 csz = ::size32(container, src_loc);
	if (csz <= idx) [[unlikely]]
		fmt::raw_range_error(src_loc, format_object_simplified(index), csz);
	auto it = std::begin(std::forward<CT>(container));
	std::advance(it, idx);
	return *it;
}

template <typename CT, typename T> requires requires (CT&& x, T&& y) { x.count(y); x.find(y); }
[[nodiscard]] constexpr auto& at32(CT&& container, T&& index, std::source_location src_loc = std::source_location::current())
{
	// Associative container
	const auto found = container.find(std::forward<T>(index));
	usz csv = umax;
	if constexpr ((requires () { container.size(); }))
		csv = container.size();
	if (found == container.end()) [[unlikely]]
		fmt::raw_range_error(src_loc, format_object_simplified(index), csv);
	return found->second;
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
consteval bool is_same_ptr()
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
}

template <typename X, typename Y> requires PtrCastable<X, Y>
constexpr bool is_same_ptr(const volatile Y* ptr)
{
	return static_cast<const volatile X*>(ptr) == static_cast<const volatile void*>(ptr);
}

template <typename X, typename Y>
concept PtrSame = (is_same_ptr<X, Y>());

namespace stx
{
	template <typename T>
	struct exact_t
	{
		static_assert(std::is_reference_v<T> || std::is_convertible_v<T, const T&>);

		T obj;

		explicit exact_t(T&& _obj) : obj(std::forward<T>(_obj)) {}
		exact_t& operator=(const exact_t&) = delete;

		template <typename U> requires (std::is_same_v<U&, T>)
		operator U&() const noexcept { return obj; };

		template <typename U> requires (std::is_same_v<const U&, T>)
		operator const U&() const noexcept { return obj; };

		template <typename U> requires (std::is_same_v<U, T> && std::is_copy_constructible_v<T>)
		operator U() const noexcept { return obj; };
	};

	template <typename T>
	stx::exact_t<T&> make_exact(T&& obj) noexcept
	{
		return stx::exact_t<T&>(static_cast<T&>(obj));
	}
}

// Read object of type T from raw pointer, array, string, vector, or any contiguous container
template <typename T, typename U>
constexpr T read_from_ptr(U&& array, usz pos = 0)
{
	// TODO: ensure array element types are trivial
	static_assert(sizeof(T) % sizeof(array[0]) == 0);
	std::decay_t<decltype(array[0])> buf[sizeof(T) / sizeof(array[0])];
	if (!std::is_constant_evaluated())
		std::memcpy(+buf, &array[pos], sizeof(buf));
	else
		for (usz i = 0; i < pos; buf[i] = array[pos + i], i++);
	return std::bit_cast<T>(buf);
}

template <typename T, typename U>
constexpr void write_to_ptr(U&& array, usz pos, const T& value)
{
	static_assert(sizeof(T) % sizeof(array[0]) == 0);
	if (!std::is_constant_evaluated())
		std::memcpy(static_cast<void*>(&array[pos]), &value, sizeof(value));
	else
		ensure(!"Unimplemented");
}

template <typename T, typename U>
constexpr void write_to_ptr(U&& array, const T& value)
{
	static_assert(sizeof(T) % sizeof(array[0]) == 0);
	if (!std::is_constant_evaluated())
		std::memcpy(static_cast<void*>(&array[0]), &value, sizeof(value));
	else
		ensure(!"Unimplemented");
}

constexpr struct aref_tag_t{} aref_tag{};

template <typename T, typename U>
class aref final
{
	U* m_ptr;

	static_assert(sizeof(std::decay_t<T>) % sizeof(U) == 0);

public:
	aref() = delete;

	constexpr aref(const aref&) = default;

	explicit constexpr aref(aref_tag_t, U* ptr)
		: m_ptr(ptr)
	{
	}

	constexpr T value() const
	{
		return read_from_ptr<T>(m_ptr);
	}

	constexpr operator T() const
	{
		return read_from_ptr<T>(m_ptr);
	}

	aref& operator=(const aref&) = delete;

	constexpr aref& operator=(const T& value) const
	{
		write_to_ptr<T>(m_ptr, value);
		return *this;
	}

	template <typename MT, typename T2> requires (std::is_convertible_v<const volatile T*, const volatile T2*>) && PtrSame<T, T2>
	aref<MT, U> ref(MT T2::*const mptr) const
	{
		return aref<MT, U>(aref_tag, m_ptr + offset32(mptr) / sizeof(U));
	}

	template <typename MT, typename T2, typename ET = std::remove_extent_t<MT>> requires (std::is_convertible_v<const volatile T*, const volatile T2*>) && PtrSame<T, T2>
	aref<ET, U> ref(MT T2::*const mptr, usz index) const
	{
		return aref<ET, U>(aref_tag, m_ptr + offset32(mptr) / sizeof(U) + sizeof(ET) / sizeof(U) * index);
	}
};

template <typename T, typename U>
class aref<T[], U>
{
	U* m_ptr;

	static_assert(sizeof(std::decay_t<T>) % sizeof(U) == 0);

public:
	aref() = delete;

	constexpr aref(const aref&) = default;

	explicit constexpr aref(aref_tag_t, U* ptr)
		: m_ptr(ptr)
	{
	}

	aref& operator=(const aref&) = delete;

	constexpr aref<T, U> operator[](usz index) const
	{
		return aref<T, U>(aref_tag, m_ptr + index * (sizeof(T) / sizeof(U)));
	}
};

template <typename T, typename U, std::size_t N>
class aref<T[N], U>
{
	U* m_ptr;

	static_assert(sizeof(std::decay_t<T>) % sizeof(U) == 0);

public:
	aref() = delete;

	constexpr aref(const aref&) = default;

	explicit constexpr aref(aref_tag_t, U* ptr)
		: m_ptr(ptr)
	{
	}

	aref& operator=(const aref&) = delete;

	constexpr aref<T, U> operator[](usz index) const
	{
		return aref<T, U>(aref_tag, m_ptr + index * (sizeof(T) / sizeof(U)));
	}
};

// Reference object of type T, see read_from_ptr
template <typename T, typename U>
constexpr auto ref_ptr(U&& array, usz pos = 0) -> aref<T, std::decay_t<decltype(array[0])>>
{
	return aref<T, std::decay_t<decltype(array[0])>>(aref_tag, &array[pos]);
}

namespace utils
{
	struct serial;
}

template <typename T>
extern bool serialize(utils::serial& ar, T& obj);

#define USING_SERIALIZATION_VERSION(name) []()\
{\
	extern void using_##name##_serialization();\
	using_##name##_serialization();\
}()

#define GET_OR_USE_SERIALIZATION_VERSION(cond, name) [&]()\
{\
	extern void using_##name##_serialization();\
	extern s32 get_##name##_serialization_version();\
	return (static_cast<bool>(cond) ? (using_##name##_serialization(), 0) : get_##name##_serialization_version());\
}()

#define GET_SERIALIZATION_VERSION(name) []()\
{\
	extern s32 get_##name##_serialization_version();\
	return get_##name##_serialization_version();\
}()

#define ENABLE_BITWISE_SERIALIZATION using enable_bitcopy = std::true_type;
#define SAVESTATE_INIT_POS(...) static constexpr double savestate_init_pos = (__VA_ARGS__)

#define UNUSED(expr) do { (void)(expr); } while (0)
