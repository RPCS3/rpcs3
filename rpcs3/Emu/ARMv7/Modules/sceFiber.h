#pragma once

using SceFiberEntry = func_def<void(u32 argOnInitialize, u32 argOnRun)>;

struct SceFiber
{
	le_t<u64> padding[16];
};

CHECK_SIZE_ALIGN(SceFiber, 128, 8);

struct SceFiberOptParam
{
	le_t<u64> padding[16];
};

CHECK_SIZE_ALIGN(SceFiberOptParam, 128, 8);

struct SceFiberInfo
{
	union
	{
		le_t<u64> padding[16];

		struct
		{
			vm::lptr<SceFiberEntry> entry;
			le_t<u32> argOnInitialize;
			vm::lptr<void> addrContext;
			le_t<s32> sizeContext;
			char name[32];
		};
	};
};

CHECK_SIZE_ALIGN(SceFiberInfo, 128, 8);

extern psv_log_base sceFiber;
