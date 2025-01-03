#pragma once

#include <string>
#include "Emu/CPU/CPUThread.h"
#include "Utilities/StrFmt.h"

enum class cpu_disasm_mode
{
	dump,
	interpreter,
	normal,
	compiler_elf,
	list, // RSX exclusive
	survey_cmd_size, // RSX exclusive
};

class cpu_thread;

class CPUDisAsm
{
protected:
	cpu_disasm_mode m_mode{};
	const u8* m_offset{};
	const u32 m_start_pc;
	std::add_pointer_t<const cpu_thread> m_cpu{};
	shared_ptr<cpu_thread> m_cpu_handle;
	u32 m_op = 0;

	void format_by_mode()
	{
		switch (m_mode)
		{
			case cpu_disasm_mode::dump:
			{
				last_opcode = fmt::format("\t%08x:\t%02x %02x %02x %02x\t%s\n", dump_pc,
					static_cast<u8>(m_op >> 24),
					static_cast<u8>(m_op >> 16),
					static_cast<u8>(m_op >> 8),
					static_cast<u8>(m_op >> 0), last_opcode);
				break;
			}

			case cpu_disasm_mode::interpreter:
			{
				last_opcode.insert(0, fmt::format("[%08x]  %02x %02x %02x %02x: ", dump_pc,
					static_cast<u8>(m_op >> 24),
					static_cast<u8>(m_op >> 16),
					static_cast<u8>(m_op >> 8),
					static_cast<u8>(m_op >> 0)));
				break;
			}

			case cpu_disasm_mode::compiler_elf:
			{
				last_opcode += '\n';
				break;
			}
			case cpu_disasm_mode::normal:
			{
				break;
			}
			default: fmt::throw_exception("Unreachable");
		}
	}

public:
	std::string last_opcode{};
	u32 dump_pc{};

	cpu_disasm_mode change_mode(cpu_disasm_mode mode)
	{
		return std::exchange(m_mode, mode);
	}

	const u8* change_ptr(const u8* ptr)
	{
		return std::exchange(m_offset, ptr);
	}

	cpu_thread* get_cpu() const
	{
		return const_cast<cpu_thread*>(m_cpu);
	}

	void set_cpu_handle(shared_ptr<cpu_thread> cpu)
	{
		m_cpu_handle = std::move(cpu);

		if (!m_cpu)
		{
			m_cpu = m_cpu_handle.get();
		}
		else
		{
			AUDIT(m_cpu == m_cpu_handle.get());
		}
	}

protected:
	CPUDisAsm(cpu_disasm_mode mode, const u8* offset, u32 start_pc = 0, const cpu_thread* cpu = nullptr)
		: m_mode(mode)
		, m_offset(offset - start_pc)
		, m_start_pc(start_pc)
		, m_cpu(cpu)
	{
	}

	CPUDisAsm& operator=(const CPUDisAsm&) = delete;

	virtual u32 DisAsmBranchTarget(s32 /*imm*/);

	// TODO: Add builtin fmt helpper for best performance
	template <typename T>
		requires std::is_integral_v<T>
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

	// Signify the formatting function the minimum required amount of characters to print for an instruction
	// Padding with spaces
	int PadOp(std::string_view op = {}, int min_spaces = 0) const
	{
		return m_mode == cpu_disasm_mode::normal ? (static_cast<int>(op.size()) + min_spaces) : 10;
	}

public:
	virtual ~CPUDisAsm() = default;

	virtual u32 disasm(u32 pc) = 0;
	virtual std::pair<const void*, usz> get_memory_span() const = 0;
	virtual std::unique_ptr<CPUDisAsm> copy_type_erased() const = 0;
};
