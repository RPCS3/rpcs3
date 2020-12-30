#include "stdafx.h" // No BOM and only basic ASCII in this file, or a neko will die

static_assert(std::endian::native == std::endian::little || std::endian::native == std::endian::big);

CHECK_SIZE_ALIGN(u128, 16, 16);
CHECK_SIZE_ALIGN(s128, 16, 16);

CHECK_SIZE_ALIGN(f16, 2, 2);

static_assert(be_t<u16>(1) + be_t<u32>(2) + be_t<u64>(3) == 6);
static_assert(le_t<u16>(1) + le_t<u32>(2) + le_t<u64>(3) == 6);

static_assert(sizeof(nullptr) == sizeof(void*));
