#pragma once

#include <string>
#include "Utilities/StrFmt.h"

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
				last_opcode = fmt::format("\t%08x:\t%02x %02x %02x %02x\t%s\n", dump_pc,
					offset[dump_pc],
					offset[dump_pc + 1],
					offset[dump_pc + 2],
					offset[dump_pc + 3], value);
			break;

			case CPUDisAsm_InterpreterMode:
				last_opcode = fmt::format("[%08x]  %02x %02x %02x %02x: %s", dump_pc,
					offset[dump_pc],
					offset[dump_pc + 1],
					offset[dump_pc + 2],
					offset[dump_pc + 3], value);
			break;

			case CPUDisAsm_CompilerElfMode:
				last_opcode = value + "\n";
			break;
		}
	}

public:
	std::string last_opcode;
	u32 dump_pc;
	const u8* offset;

protected:
	CPUDisAsm(CPUDisAsmMode mode)
		: m_mode(mode)
		, offset(0)
	{
	}

	virtual u32 DisAsmBranchTarget(const s32 imm) = 0;

	// TODO: Add builtin fmt helpper for best performance
	template <typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
	static std::string SignedHex(T value)
	{
		const auto v = static_cast<std::make_signed_t<T>>(value);

		if (v == std::numeric_limits<std::make_signed_t<T>>::min())
		{
			// for INTx_MIN
			return fmt::format("-0x%x", v);
		}

		const auto av = std::abs(v);

		if (av < 10)
		{
			// Does not need hex
			return fmt::format("%d", v);
		}

		return fmt::format("%s%s", v < 0 ? "-" : "", av);
	}

	static std::string FixOp(std::string op)
	{
		op.resize(std::max<std::size_t>(op.length(), 10), ' ');
		return op;
	}

public:
	virtual u32 disasm(u32 pc) = 0;
};
