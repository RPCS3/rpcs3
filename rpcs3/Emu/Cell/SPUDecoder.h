#pragma once

#include "Emu/Cell/SPUOpcodes.h"
#include "Emu/Cell/PPCDecoder.h"
#include "Emu/Cell/SPUInstrtable.h"

class SPU_Decoder : public PPC_Decoder
{
	SPU_Opcodes* m_op;
	
public:
	SPU_Decoder(SPU_Opcodes& op) : m_op(&op)
	{
	}

	~SPU_Decoder()
	{
		m_op->Exit();
		delete m_op;
	}

	virtual void Decode(const u32 code)
	{
		(*SPU_instr::rrr_list)(m_op, code);
	}
};
