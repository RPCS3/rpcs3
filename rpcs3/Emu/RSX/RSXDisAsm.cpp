#include "stdafx.h"

#include "RSXDisAsm.h"
#include "RSXThread.h"
#include "gcm_enums.h"
#include "gcm_printing.h"
#include "rsx_methods.h"

namespace rsx
{
	void invalid_method(context*, u32, u32);
}

u32 RSXDisAsm::disasm(u32 pc)
{
	last_opcode.clear();

	auto try_read_op = [this](u32 pc) -> bool
	{
		if (pc < m_start_pc)
		{
			return false;
		}

		if (m_offset == vm::g_sudo_addr)
		{
			// Translation needed
			pc = static_cast<const rsx::thread*>(m_cpu)->iomap_table.get_addr(pc);

			if (pc == umax) return false;
		}

		m_op = *reinterpret_cast<const atomic_be_t<u32>*>(m_offset + pc);
		return true;
	};

	if (!try_read_op(pc))
	{
		return 0;
	}

	dump_pc = pc;

	if (m_op & RSX_METHOD_NON_METHOD_CMD_MASK)
	{
		if (m_mode == cpu_disasm_mode::survey_cmd_size)
		{
			return 4;
		}

		if ((m_op & RSX_METHOD_OLD_JUMP_CMD_MASK) == RSX_METHOD_OLD_JUMP_CMD)
		{
			u32 jumpAddr = m_op & RSX_METHOD_OLD_JUMP_OFFSET_MASK;
			Write(fmt::format("jump 0x%07x", jumpAddr), -1);
		}
		else if ((m_op & RSX_METHOD_NEW_JUMP_CMD_MASK) == RSX_METHOD_NEW_JUMP_CMD)
		{
			u32 jumpAddr = m_op & RSX_METHOD_NEW_JUMP_OFFSET_MASK;
			Write(fmt::format("jump 0x%07x", jumpAddr), -1);
		}
		else if ((m_op & RSX_METHOD_CALL_CMD_MASK) == RSX_METHOD_CALL_CMD)
		{
			u32 callAddr = m_op & RSX_METHOD_CALL_OFFSET_MASK;
			Write(fmt::format("call 0x%07x", callAddr), -1);
		}
		else if ((m_op & RSX_METHOD_RETURN_MASK) == RSX_METHOD_RETURN_CMD)
		{
			Write("ret", -1);
		}
		else
		{
			Write(fmt::format("?? ?? (0x%x)", m_op), -1);
		}

		return 4;
	}
	else if ((m_op & RSX_METHOD_NOP_MASK) == RSX_METHOD_NOP_CMD)
	{
		u32 i = 1;

		for (pc += 4; m_mode != cpu_disasm_mode::list && pc % (4096 * 4); i++, pc += 4)
		{
			if (!try_read_op(pc))
			{
				break;
			}

			if ((m_op & RSX_METHOD_NOP_MASK) != RSX_METHOD_NOP_CMD)
			{
				break;
			}
		}

		if (m_mode != cpu_disasm_mode::survey_cmd_size)
		{
			if (i == 1)
				Write("nop", 0);
			else
				Write(fmt::format("nop x%u", i), 0);
		}

		return i * 4;
	}
	else
	{
		const u32 count = (m_op & RSX_METHOD_COUNT_MASK) >> RSX_METHOD_COUNT_SHIFT;
		const bool non_inc = (m_op & RSX_METHOD_NON_INCREMENT_CMD_MASK) == RSX_METHOD_NON_INCREMENT_CMD && count > 1;
		const u32 id_start = (m_op & 0x3ffff) >> 2;

		if (count > 10 && id_start == NV4097_SET_OBJECT)
		{
			// Hack: 0 method with large count is unlikely to be a command
			// But is very common in floating point args, messing up debugger's code-flow
			Write(fmt::format("?? ?? (0x%x)", m_op), -1);
			return 4;
		}

		pc += 4;

		std::string str;

		for (u32 i = 0; i < count; i++, pc += 4)
		{
			if (!try_read_op(pc))
			{
				last_opcode.clear();
				Write(fmt::format("?? ?? (0x%08x:unmapped)", m_op), -1);
				return 4;
			}

			const u32 id = id_start + (non_inc ? 0 : i);

			if (rsx::methods[id] == &rsx::invalid_method)
			{
				last_opcode.clear();
				Write(fmt::format("?? ?? (0x%08x:method)", m_op), -1);
				return 4;
			}

			if (m_mode == cpu_disasm_mode::survey_cmd_size)
			{
				continue;
			}

			if (m_mode != cpu_disasm_mode::list && !last_opcode.empty())
			{
				continue;
			}

			str.clear();
			rsx::get_pretty_printing_function(id)(str, id, m_op);

			Write(str, m_mode == cpu_disasm_mode::list ? i : count, non_inc, id);
		}

		return (count + 1) * 4;
	}
}

std::pair<const void*, usz> RSXDisAsm::get_memory_span() const
{
	return {m_offset + m_start_pc, (1ull << 32) - m_start_pc};
}

std::unique_ptr<CPUDisAsm> RSXDisAsm::copy_type_erased() const
{
	return std::make_unique<RSXDisAsm>(*this);
}

void RSXDisAsm::Write(std::string_view str, s32 count, bool is_non_inc, u32 id)
{
	switch (m_mode)
	{
	case cpu_disasm_mode::interpreter:
	{
		last_opcode.clear();

		if (count == 1 && !is_non_inc)
		{
			fmt::append(last_opcode, "[%08x] ( )", dump_pc);
		}
		else if (count >= 0)
		{
			fmt::append(last_opcode, "[%08x] (%s%u)", dump_pc, is_non_inc ? "+" : "", count);
		}
		else
		{
			fmt::append(last_opcode, "[%08x] (x)", dump_pc);
		}

		auto& res = last_opcode;

		res.resize(7 + 11, ' ');

		res += str;
		break;
	}
	case cpu_disasm_mode::list:
	{
		if (!last_opcode.empty())
			last_opcode += '\n';

		fmt::append(last_opcode, "[%04x] 0x%08x: %s", id, m_op, str);
		break;
	}
	default:
		break;
	}
}
