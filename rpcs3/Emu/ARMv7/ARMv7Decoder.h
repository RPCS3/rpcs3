#pragma once
#include "Emu/CPU/CPUDecoder.h"

struct ARMv7Context;

class ARMv7Decoder : public CPUDecoder
{
	ARMv7Context& m_ctx;

public:
	ARMv7Decoder(ARMv7Context& context) : m_ctx(context)
	{
	}

	virtual u32 DecodeMemory(const u32 address);
};

void armv7_decoder_initialize(u32 addr, u32 end_addr, bool dump = false);
