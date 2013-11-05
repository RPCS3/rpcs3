#pragma once

namespace ARMv7_opcodes
{
	enum ARMv7_MainOpcodes
	{

	};
}

class ARMv7Opcodes
{
public:
	virtual void NULL_OP() = 0;
	virtual void NOP() = 0;

	virtual void UNK(const u16 opcode, const u16 code0, const u16 code1) = 0;
};
