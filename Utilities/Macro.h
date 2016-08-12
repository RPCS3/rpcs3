#pragma once

#include "types.h"

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

#define CONCATENATE_DETAIL(x, y) x ## y
#define CONCATENATE(x, y) CONCATENATE_DETAIL(x, y)

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)

// Macro set, wraps an expression into lambda
#define WRAP_EXPR(expr, ...) [&](__VA_ARGS__) { return expr; }
#define COPY_EXPR(expr, ...) [=](__VA_ARGS__) { return expr; }
#define PURE_EXPR(expr, ...) [] (__VA_ARGS__) { return expr; }

#define HERE "\n(in file " __FILE__ ":" STRINGIZE(__LINE__) ")"

// Ensure that the expression is evaluated to true. Always evaluated and allowed to have side effects (unlike assert() macro).
#define VERIFY(expr) do { if (!(expr)) fmt::raw_error("Verification failed: " #expr HERE); } while (0)

// EXPECTS() and ENSURES() are intended to check function arguments and results.
// Expressions are not guaranteed to evaluate.
#define EXPECTS(expr) do { if (!(expr)) fmt::raw_error("Precondition failed: " #expr HERE); } while (0)
#define ENSURES(expr) do { if (!(expr)) fmt::raw_error("Postcondition failed: " #expr HERE); } while (0)

#define DECLARE(...) decltype(__VA_ARGS__) __VA_ARGS__

#define STR_CASE(value) case value: return #value
