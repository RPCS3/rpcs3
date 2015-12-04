#pragma once

#include "Emu/Cell/PPUOpcodes.h"
#include "Emu/Cell/PPCDecoder.h"
#include "PPUInstrTable.h"

class PPUDecoder : public PPCDecoder
{
	PPUOpcodes* m_op;

public:
	PPUDecoder(PPUOpcodes* op) : m_op(op)
	{
	}

	virtual ~PPUDecoder()
	{
		delete m_op;
	}

	virtual void Decode(const u32 code)
	{
		(*PPU_instr::main_list)(m_op, code);
	}
};