#pragma once

using SceFiberEntry = void(u32 argOnInitialize, u32 argOnRun);

struct alignas(8) SceFiber
{
	le_t<u64> padding[16];
};

CHECK_SIZE_ALIGN(SceFiber, 128, 8);

struct alignas(8) SceFiberOptParam
{
	le_t<u64> padding[16];
};

CHECK_SIZE_ALIGN(SceFiberOptParam, 128, 8);

struct alignas(8) SceFiberInfo
{
	vm::lptr<SceFiberEntry> entry;
	le_t<u32> argOnInitialize;
	vm::lptr<void> addrContext;
	le_t<s32> sizeContext;
	char name[32];
	u8 padding[80];
};

CHECK_SIZE_ALIGN(SceFiberInfo, 128, 8);

extern psv_log_base sceFiber;
