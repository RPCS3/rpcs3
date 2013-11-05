#pragma once
#include "Emu/CPU/CPUDecoder.h"
#include "ARM9Opcodes.h"


class ARM9Decoder : public CPUDecoder
{
	ARM9Opcodes& m_op;

public:
	ARM9Decoder(ARM9Opcodes& op) : m_op(op)
	{
	}

	virtual void DecodeMemory(const u64 address)
	{
		const u16 code0 = Memory.Read16(address);
		const u16 code1 = Memory.Read16(address + 2);
		const u16 opcode = code0;

		switch(opcode)
		{
		case 0:
			m_op.NULL_OP();
		break;

		default:
			m_op.UNK(opcode, code0, code1);
		break;
		}
	}
};
