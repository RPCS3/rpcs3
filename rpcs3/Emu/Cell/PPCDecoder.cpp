#include "stdafx.h"
#include "Emu/ConLog.h"
#include "Emu/Memory/Memory.h"
#include "PPCDecoder.h"

u8 PPCDecoder::DecodeMemory(const u64 address)
{
	u32 instr;
	Memory.Read32ByAddr(address, &instr);
	Decode(instr);

	return 4;
}