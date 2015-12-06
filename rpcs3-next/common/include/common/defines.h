#pragma once
#include <type_traits>
#include "exception.h"

#define CHECK_SIZE(type, size) static_assert(sizeof(type) == size, "Invalid " #type " type size")
#define CHECK_ALIGN(type, align) static_assert(alignof(type) == align, "Invalid " #type " type alignment")
#define CHECK_MAX_SIZE(type, size) static_assert(sizeof(type) <= size, #type " type size is too big")
#define CHECK_SIZE_ALIGN(type, size, align) CHECK_SIZE(type, size); CHECK_ALIGN(type, align)
#define CHECK_ASCENDING(constexpr_array) static_assert(::common::is_ascending(constexpr_array), #constexpr_array " is not sorted in ascending order")

// return 32 bit sizeof() to avoid widening/narrowing conversions with size_t
#define sizeof32(type) static_cast<u32>(sizeof(type))

// return 32 bit alignof() to avoid widening/narrowing conversions with size_t
#define alignof32(type) static_cast<u32>(alignof(type))

#define WRAP_EXPR(expr) [&]{ return expr; }
#define COPY_EXPR(expr) [=]{ return expr; }
#define VM_CAST(value) vm::impl_cast(value, __FILE__, __LINE__, __FUNCTION__)
#define IS_INTEGRAL(t) (std::is_integral<t>::value || std::is_same<std::decay_t<t>, u128>::value)
#define IS_INTEGER(t) (std::is_integral<t>::value || std::is_enum<t>::value || std::is_same<std::decay_t<t>, u128>::value)
#define IS_BINARY_COMPARABLE(t1, t2) (IS_INTEGER(t1) && IS_INTEGER(t2) && sizeof(t1) == sizeof(t2))
#define CHECK_ASSERTION(expr) if (expr) {} else throw EXCEPTION("Assertion failed: " #expr)
#define CHECK_SUCCESS(expr) if (s32 _r = (expr)) throw EXCEPTION(#expr " failed (0x%x)", _r)

namespace common
{
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

	// returns true if all array elements are unique and sorted in ascending order
	template<typename T, std::size_t N> constexpr bool is_ascending(const T(&array)[N], std::size_t from = 0)
	{
		return from >= N - 1 ? true : array[from] < array[from + 1] ? is_ascending(array, from + 1) : false;
	}

	// get (first) array element equal to `value` or nullptr if not found
	template<typename T, std::size_t N, typename T2> constexpr const T* static_search(const T(&array)[N], const T2& value, std::size_t from = 0)
	{
		return from >= N ? nullptr : array[from] == value ? array + from : static_search(array, value, from + 1);
	}

	// bool wrapper for restricting bool result conversions
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

	template<typename T1, typename T2, typename T3 = const char*> struct triplet
	{
		T1 first;
		T2 second;
		T3 third;

		constexpr bool operator ==(const T1& right) const
		{
			return first == right;
		}
	};

	// return 32 bit .size() for container
	template<typename T> inline auto size32(const T& container) -> decltype(static_cast<u32>(container.size()))
	{
		const auto size = container.size();
		return size >= 0 && size <= UINT32_MAX ? static_cast<u32>(size) : throw std::length_error(__FUNCTION__);
	}

	// return 32 bit size for an array
	template<typename T, std::size_t Size> constexpr u32 size32(const T(&)[Size])
	{
		return Size >= 0 && Size <= UINT32_MAX ? static_cast<u32>(Size) : throw std::length_error(__FUNCTION__);
	}
}
