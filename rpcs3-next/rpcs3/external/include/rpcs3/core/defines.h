#pragma once

#define CHECK_SIZE(type, size) static_assert(sizeof(type) == size, "Invalid " #type " type size")
#define CHECK_ALIGN(type, align) static_assert(alignof(type) == align, "Invalid " #type " type alignment")
#define CHECK_MAX_SIZE(type, size) static_assert(sizeof(type) <= size, #type " type size is too big")
#define CHECK_SIZE_ALIGN(type, size, align) CHECK_SIZE(type, size); CHECK_ALIGN(type, align)
#define CHECK_ASCENDING(constexpr_array) static_assert(::is_ascending(constexpr_array), #constexpr_array " is not sorted in ascending order")

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