#pragma once

#include <cstdint>
#include <exception>

template<typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
constexpr T align(const T& value, std::uint64_t align)
{
	return static_cast<T>((value + (align - 1)) & ~(align - 1));
}

template<typename To, typename From>
constexpr To narrow_impl(const To& result, const From& value, const char* message)
{
	return static_cast<From>(result) != value ? throw std::runtime_error(message) : result;
}

// Narrow cast (similar to gsl::narrow) with fixed message
template<typename To, typename From>
constexpr auto narrow(const From& value, const char* fixed_msg = "::narrow() failed") -> decltype(static_cast<To>(static_cast<From>(std::declval<To>())))
{
	return narrow_impl(static_cast<To>(value), value, fixed_msg);
}

// Return 32 bit .size() for container
template<typename CT>
constexpr auto size32(const CT& container, const char* fixed_msg = "::size32() failed") -> decltype(static_cast<std::uint32_t>(container.size()))
{
	return narrow<std::uint32_t>(container.size(), fixed_msg);
}

// Return 32 bit size for an array
template<typename T, std::size_t Size>
constexpr std::uint32_t size32(const T(&)[Size])
{
	static_assert(Size <= UINT32_MAX, "size32() error: too big");
	return static_cast<std::uint32_t>(Size);
}

#define CHECK_SIZE(type, size) static_assert(sizeof(type) == size, "Invalid " #type " type size")
#define CHECK_ALIGN(type, align) static_assert(alignof(type) == align, "Invalid " #type " type alignment")
#define CHECK_MAX_SIZE(type, size) static_assert(sizeof(type) <= size, #type " type size is too big")
#define CHECK_SIZE_ALIGN(type, size, align) CHECK_SIZE(type, size); CHECK_ALIGN(type, align)
#define CHECK_STORAGE(type, storage) static_assert(sizeof(type) <= sizeof(storage) && alignof(type) <= alignof(decltype(storage)), #type " is too small")

// Return 32 bit sizeof() to avoid widening/narrowing conversions with size_t
#define SIZE_32(type) static_cast<std::uint32_t>(sizeof(type))

// Return 32 bit alignof() to avoid widening/narrowing conversions with size_t
#define ALIGN_32(type) static_cast<std::uint32_t>(alignof(type))

// Return 32 bit custom offsetof()
#define OFFSET_32(type, x) static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&reinterpret_cast<const volatile char&>(reinterpret_cast<type*>(0ull)->x)))

// Sometimes to avoid writing std::remove_cv_t<>, example: std::is_same<CV T1, CV T2>
#define CV const volatile

#define CONCATENATE_DETAIL(x, y) x ## y
#define CONCATENATE(x, y) CONCATENATE_DETAIL(x, y)

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)

// Macro set, wraps an expression into lambda
#define WRAP_EXPR(expr, ...) [&](__VA_ARGS__) { return expr; }
#define COPY_EXPR(expr, ...) [=](__VA_ARGS__) { return expr; }
#define PURE_EXPR(expr, ...) [] (__VA_ARGS__) { return expr; }

#define return_ return

#define HERE "\n(in file " __FILE__ ":" STRINGIZE(__LINE__) ")"

// Ensure that the expression is evaluated to true. Always evaluated and allowed to have side effects (unlike assert() macro).
#define VERIFY(expr) do { if (!(expr)) throw std::runtime_error("Verification failed: " #expr HERE); } while (0)

// EXPECTS() and ENSURES() are intended to check function arguments and results.
// Expressions are not guaranteed to evaluate.
#define EXPECTS(expr) do { if (!(expr)) throw std::runtime_error("Precondition failed: " #expr HERE); } while (0)
#define ENSURES(expr) do { if (!(expr)) throw std::runtime_error("Postcondition failed: " #expr HERE); } while (0)

#define DECLARE(...) decltype(__VA_ARGS__) __VA_ARGS__

#define STR_CASE(value) case value: return #value
