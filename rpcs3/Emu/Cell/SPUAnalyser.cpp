#include "stdafx.h"
#include "SPUAnalyser.h"

template <>
void fmt_class_string<spu_iname::type>::format(std::string& out, u64 arg)
{
	// Decode instruction name from the enum value
	for (u32 i = 0; i < 10; i++)
	{
		if (u64 value = (arg >> (54 - i * 6)) & 0x3f)
		{
			out += static_cast<char>(value + 0x20);
		}
	}
}
