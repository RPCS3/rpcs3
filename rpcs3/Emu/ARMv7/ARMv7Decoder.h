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

	u32 branchTarget(u32 imm)
	{
		return imm << 1;
	}

	virtual u8 DecodeMemory(const u64 address)
	{
		using namespace ARMv7_opcodes;
		const u16 code0 = Memory.Read16(address);
		const u16 code1 = Memory.Read16(address + 2);

		switch(code0 >> 12) //15 - 12
		{
		case T1_CBZ:
			switch((code0 >> 10) & 0x1)
			{
			case 0:
				switch((code0 >> 8) & 0x1)
				{
				case 1:
					m_op.CBZ((code0 >> 11) & 0x1, branchTarget((((code0 >> 9) & 0x1) << 5) | (code0 >> 3) & 0x1f), code0 & 0x7, 2);
				return 2;
				}
			break;
			}
		break;

		case T1_B:
			m_op.B((code0 >> 8) & 0xf, branchTarget(code0 & 0xff), 2);
		return 2;
		}

		switch(code0 >> 11) //15 - 11
		{
		case T2_B:
			m_op.B(0xf, branchTarget(code0 & 0xfff), 2);
		return 2;

		case T3_B:
		{
			u8 S = (code0 >> 10) & 0x1;
			u8 J1 = (code1 >> 13) & 0x1;
			u8 J2 = (code1 >> 11) & 0x1;
			u8 I1 = 1 - (J1 ^ S);
			u8 I2 = 1 - (J2 ^ S);
			u16 imm11 = code1 & 0x7ff;
			u32 imm32 = 0;

			switch(code1 >> 14)
			{
			case 2: //B
			{
				u8 cond;
				switch((code1 >> 12) & 0x1)
				{
				case 0:
				{
					cond = (code0 >> 6) & 0xf;
					u32 imm6 = code0 & 0x3f;
					imm32 = sign<19, u32>((S << 19) | (I1 << 18) | (I2 << 17) | (imm6 << 11) | imm11);
				}
				break;

				case 1:
					cond = 0xf;
					u32 imm10 = code0 & 0x7ff;
					imm32 = sign<23, u32>((S << 23) | (I1 << 22) | (I2 << 21) | (imm10 << 11) | imm11);
				break;
				}

				m_op.B(cond, branchTarget(imm32), 4);
			}
			return 4;

			case 3: //BL
				switch((code1 >> 12) & 0x1)
				{
				case 0:
					break;

				case 1:
					u32 imm10 = code0 & 0x7ff;
					imm32 = sign<23, u32>((S << 23) | (I1 << 22) | (I2 << 21) | (imm10 << 11) | imm11);
					m_op.BL(branchTarget(imm32), 4);
				return 4;
				}
			break;
			}
		}
		break;
		}

		switch(code0 >> 9)
		{
		case T1_PUSH:
			m_op.PUSH((((code0 >> 8) & 0x1) << 14) | (code0 & 0xff));
		return 2;

		case T1_POP:
			m_op.POP((((code0 >> 8) & 0x1) << 15) | (code0 & 0xff));
		return 2;
		}

		switch(code0)
		{
		case T2_PUSH:
			m_op.PUSH(code1);
		return 4;

		case T2_POP:
			m_op.POP(code1);
		return 4;

		case T1_NOP:
			m_op.NOP();
		return 2;
		}

		m_op.UNK(code0, code1);
		return 2;
	}
};
