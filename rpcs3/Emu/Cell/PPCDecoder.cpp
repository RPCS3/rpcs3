#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "PPCDecoder.h"

u8 PPCDecoder::DecodeMemory(const u32 address)
{
	u32 instr = vm::read32(address);
	Decode(instr);

	return sizeof(u32);
}
