#pragma once
#include "Emu/CPU/CPUDecoder.h"
#include "ARMv7Opcodes.h"


class ARMv7Decoder : public CPUDecoder
{
	ARMv7Opcodes& m_op;
	u8 m_last_instr_size;

public:
	ARMv7Decoder(ARMv7Opcodes& op) : m_op(op)
	{
	}

	virtual u8 DecodeMemory(const u64 address)
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

		return 2;
	}
};
