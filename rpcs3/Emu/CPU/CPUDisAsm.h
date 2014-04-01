#pragma once

#include "Emu/Memory/Memory.h"

enum CPUDisAsmMode
{
	CPUDisAsm_DumpMode,
	CPUDisAsm_InterpreterMode,
	//CPUDisAsm_NormalMode,
	CPUDisAsm_CompilerElfMode,
};

class CPUDisAsm
{
protected:
	const CPUDisAsmMode m_mode;

	virtual void Write(const std::string& value)
	{
		switch(m_mode)
		{
			case CPUDisAsm_DumpMode:
				last_opcode = fmt::Format("\t%08llx:\t%02x %02x %02x %02x\t%s\n", dump_pc,
					Memory.Read8(offset + dump_pc),
					Memory.Read8(offset + dump_pc + 1),
					Memory.Read8(offset + dump_pc + 2),
					Memory.Read8(offset + dump_pc + 3), value.c_str());
			break;

			case CPUDisAsm_InterpreterMode:
				last_opcode = fmt::Format("[%08llx]  %02x %02x %02x %02x: %s", dump_pc,
					Memory.Read8(offset + dump_pc),
					Memory.Read8(offset + dump_pc + 1),
					Memory.Read8(offset + dump_pc + 2),
					Memory.Read8(offset + dump_pc + 3), value.c_str());
			break;

			case CPUDisAsm_CompilerElfMode:
				last_opcode = value + "\n";
			break;
		}
	}

public:
	std::string last_opcode;
	u64 dump_pc;
	u64 offset;

protected:
	CPUDisAsm(CPUDisAsmMode mode) 
		: m_mode(mode)
		, offset(0)
	{
	}

	virtual u32 DisAsmBranchTarget(const s32 imm)=0;

	std::string FixOp(std::string op)
	{
		op.append(max<int>(10 - (int)op.length(), 0),' ');
		return op;
	}
};
