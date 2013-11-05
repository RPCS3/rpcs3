#include "stdafx.h"
#include "PPCDecoder.h"

u8 PPCDecoder::DecodeMemory(const u64 address)
{
	Decode(Memory.Read32(address));

	return 4;
}