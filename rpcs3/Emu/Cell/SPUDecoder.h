#pragma once

#include "Emu/Cell/SPUOpcodes.h"
#include "Emu/Cell/PPCDecoder.h"
#include "Emu/Cell/SPUInstrTable.h"

class SPUDecoder : public PPCDecoder
{
	SPUOpcodes* m_op;
	
public:
	SPUDecoder(SPUOpcodes& op) : m_op(&op)
	{
	}

	~SPUDecoder()
	{
		delete m_op;
	}

	virtual void Decode(const u32 code)
	{
		(*SPU_instr::rrr_list)(m_op, code);
	}
};
