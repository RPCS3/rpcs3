#pragma once
#include "Emu/ARMv7/ARMv7Opcodes.h"

class ARMv7Interpreter : public ARMv7Opcodes
{
	ARMv7Thread& CPU;

public:
	ARMv7Interpreter(ARMv7Thread& cpu) : CPU(cpu)
	{
	}

protected:
	void NULL_OP()
	{
		ConLog.Error("null");
		Emu.Pause();
	}

	void NOP()
	{
	}

	void UNK(const u16 opcode, const u16 code0, const u16 code1)
	{
		ConLog.Error("Unknown/Illegal opcode! (0x%04x : 0x%04x : 0x%04x)", opcode, code0, code1);
		Emu.Pause();
	}
};