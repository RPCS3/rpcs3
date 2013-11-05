#pragma once
#include "Emu/ARM9/ARM9Opcodes.h"

class ARM9Interpreter : public ARM9Opcodes
{
	ARM9Thread& CPU;

public:
	ARM9Interpreter(ARM9Thread& cpu) : CPU(cpu)
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