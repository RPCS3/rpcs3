#pragma once

#include <string>
#include "Utilities/StrFmt.h"

enum class cpu_disasm_mode
{
	dump,
	interpreter,
	normal,
	compiler_elf,
	list, // RSX exclusive
};

class cpu_thread;

class CPUDisAsm
{
protected:
	const cpu_disasm_mode m_mode{};
	const std::add_pointer_t<const u8> m_offset{};
	const std::add_pointer_t<const cpu_thread> m_cpu{};
	u32 m_op = 0;

	void Write(const std::string& value)
	{
		switch (m_mode)
		{
			case cpu_disasm_mode::dump:
			{
				last_opcode = fmt::format("\t%08x:\t%02x %02x %02x %02x\t%s\n", dump_pc,
					static_cast<u8>(m_op >> 24),
					static_cast<u8>(m_op >> 16),
					static_cast<u8>(m_op >> 8),
					static_cast<u8>(m_op >> 0), value);
				break;
			}

			case cpu_disasm_mode::interpreter:
			{
				last_opcode = fmt::format("[%08x]  %02x %02x %02x %02x: %s", dump_pc,
					static_cast<u8>(m_op >> 24),
					static_cast<u8>(m_op >> 16),
					static_cast<u8>(m_op >> 8),
					static_cast<u8>(m_op >> 0), value);
				break;
			}

			case cpu_disasm_mode::compiler_elf:
			{
				last_opcode = value + '\n';
				break;
			}
			case cpu_disasm_mode::normal:
			{
				last_opcode = value;
				break;
			}
			default: fmt::throw_exception("Unreachable");
		}
	}

public:
	std::string last_opcode{};
	u32 dump_pc{};

	template <typename T, std::enable_if_t<std::is_base_of_v<CPUDisAsm, T>, int> = 0>
	static T copy_and_change_mode(const T& dis, cpu_disasm_mode mode)
	{
		return T{mode, dis.m_offset, dis.m_cpu};
	}

protected:
	CPUDisAsm(cpu_disasm_mode mode, const u8* offset, const cpu_thread* cpu = nullptr)
		: m_mode(mode)
		, m_offset(offset)
		, m_cpu(cpu)
	{
	}

	CPUDisAsm(const CPUDisAsm&) = delete;

	CPUDisAsm& operator=(const CPUDisAsm&) = delete;

	virtual ~CPUDisAsm() = default;

	virtual u32 DisAsmBranchTarget(s32 /*imm*/);

	// TODO: Add builtin fmt helpper for best performance
	template <typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
	static std::string SignedHex(T value)
	{
		const auto v = static_cast<std::make_signed_t<T>>(value);

		if (v == smin)
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
		if (m_mode != cpu_disasm_mode::normal)
		{
			op.resize(std::max<usz>(op.length(), 10), ' ');
		}

		return op;
	}

public:
	virtual u32 disasm(u32 pc) = 0;
};
