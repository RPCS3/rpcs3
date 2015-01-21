#pragma once
#include "Emu/CPU/CPUDecoder.h"

class ARMv7Thread;

class ARMv7Decoder : public CPUDecoder
{
	ARMv7Thread& m_thr;

public:
	ARMv7Decoder(ARMv7Thread& thr) : m_thr(thr)
	{
	}

	virtual u8 DecodeMemory(const u32 address);
};
