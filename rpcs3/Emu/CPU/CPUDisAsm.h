#pragma once

#include <string>
#include <limits>
#include "Utilities/StrFmt.h"

enum CPUDisAsmMode
{
	CPUDisAsm_DumpMode,
	CPUDisAsm_InterpreterMode,
	CPUDisAsm_NormalMode,
	CPUDisAsm_CompilerElfMode,
};

class CPUDisAsm
{
protected:
	const CPUDisAsmMode m_mode;
	const std::add_pointer_t<const u8> m_offset;
	u32 m_op = 0;

	virtual void Write(const std::string& value)
	{
		switch (m_mode)
		{
			case CPUDisAsm_DumpMode:
			{
				last_opcode = fmt::format("\t%08x:\t%02x %02x %02x %02x\t%s\n", dump_pc,
					static_cast<u8>(m_op >> 24),
					static_cast<u8>(m_op >> 16),
					static_cast<u8>(m_op >> 8),
					static_cast<u8>(m_op >> 0), value);
				break;
			}

			case CPUDisAsm_InterpreterMode:
			{
				last_opcode = fmt::format("[%08x]  %02x %02x %02x %02x: %s", dump_pc,
					static_cast<u8>(m_op >> 24),
					static_cast<u8>(m_op >> 16),
					static_cast<u8>(m_op >> 8),
					static_cast<u8>(m_op >> 0), value);
				break;
			}

			case CPUDisAsm_CompilerElfMode:
			{
				last_opcode = value + '\n';
				break;
			}
			case CPUDisAsm_NormalMode:
			{
				last_opcode = value;
				break;
			}
			default: fmt::throw_exception("Unreachable");
		}
	}

public:
	std::string last_opcode;
	u32 dump_pc;

protected:
	CPUDisAsm(CPUDisAsmMode mode, const u8* offset)
		: m_mode(mode)
		, m_offset(offset)
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

	std::string FixOp(std::string op) const
	{
		if (m_mode != CPUDisAsm_NormalMode)
		{
			op.resize(std::max<std::size_t>(op.length(), 10), ' ');
		}

		return op;
	}

public:
	virtual u32 disasm(u32 pc) = 0;
};
