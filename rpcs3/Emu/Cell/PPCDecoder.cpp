#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "PPCDecoder.h"

u8 PPCDecoder::DecodeMemory(const u64 address)
{
	u32 instr = Memory.Read32(address);
	Decode(instr);

	return sizeof(u32);
}