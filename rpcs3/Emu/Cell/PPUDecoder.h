#pragma once

#include "Emu/Cell/PPUOpcodes.h"
#include "Emu/Cell/PPCDecoder.h"
#include "PPUInstrTable.h"

class PPU_Decoder : public PPC_Decoder
{
	PPU_Opcodes* m_op;

public:
	PPU_Decoder(PPU_Opcodes& op) : m_op(&op)
	{
	}

	~PPU_Decoder()
	{
		m_op->Exit();
		delete m_op;
	}

	virtual void Decode(const u32 code)
	{
		(*PPU_instr::main_list)(m_op, code);
	}
};