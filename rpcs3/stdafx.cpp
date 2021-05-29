#include "stdafx.h" // No BOM and only basic ASCII in this file, or a neko will die

static_assert(std::endian::native == std::endian::little || std::endian::native == std::endian::big);

CHECK_SIZE_ALIGN(u128, 16, 16);
CHECK_SIZE_ALIGN(s128, 16, 16);

CHECK_SIZE_ALIGN(f16, 2, 2);

static_assert(be_t<u16>(1) + be_t<u32>(2) + be_t<u64>(3) == 6);
static_assert(le_t<u16>(1) + le_t<u32>(2) + le_t<u64>(3) == 6);

static_assert(sizeof(nullptr) == sizeof(void*));

static_assert(__cpp_constexpr_dynamic_alloc >= 201907L);

namespace
{
	struct A
	{
		int a;
	};

	struct B : A
	{
		int b;
	};

	struct Z
	{
	};

	struct C
	{
		virtual ~C() = 0;
		int C;
	};

	struct D : Z, B
	{
		int d;
	};

	struct E : C, B
	{
		int e;
	};

	struct F : C
	{
		virtual ~F() = 0;
	};

	static_assert(is_same_ptr<B, A>());
	static_assert(is_same_ptr<A, B>());
	static_assert(is_same_ptr<D, B>());
	static_assert(is_same_ptr<B, D>());
	static_assert(!is_same_ptr<E, B>());
	static_assert(!is_same_ptr<B, E>());
	static_assert(is_same_ptr<F, C>());
	static_assert(is_same_ptr<C, F>());
}
