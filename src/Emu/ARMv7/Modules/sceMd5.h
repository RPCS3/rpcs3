#pragma once

struct SceMd5Context
{
	le_t<u32> h[4];
	le_t<u32> pad;
	le_t<u16> usRemains;
	le_t<u16> usComputed;
	le_t<u64> ullTotalLen;
	u8 buf[64];
	u8 result[64];
};

extern psv_log_base sceMd5;
