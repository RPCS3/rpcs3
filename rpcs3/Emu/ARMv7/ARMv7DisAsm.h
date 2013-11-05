#pragma once
#include "Emu/ARMv7/ARMv7Opcodes.h"
#include "Emu/CPU/CPUDisAsm.h"
#include "Gui/DisAsmFrame.h"
#include "Emu/Memory/Memory.h"

class ARMv7DisAsm
	: public CPUDisAsm
	, public ARMv7Opcodes
{
public:
	ARMv7DisAsm(CPUDisAsmMode mode) : CPUDisAsm(mode)
	{
	}

protected:
	virtual u32 DisAsmBranchTarget(const s32 imm)
	{
		return dump_pc + (imm << 2);
	}

	void NULL_OP()
	{
		Write("null");
	}

	void NOP()
	{
		Write("nop");
	}

	void UNK(const u16 opcode, const u16 code0, const u16 code1)
	{
		Write(wxString::Format("Unknown/Illegal opcode! (0x%04x : 0x%04x : 0x%04x)", opcode, code0, code1));
	}
};