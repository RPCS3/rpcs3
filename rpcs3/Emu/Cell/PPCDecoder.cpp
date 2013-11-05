#include "stdafx.h"
#include "PPCDecoder.h"

void PPCDecoder::DecodeMemory(const u64 address)
{
	Decode(Memory.Read32(address));
}