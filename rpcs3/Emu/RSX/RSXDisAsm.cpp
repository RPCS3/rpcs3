#include "stdafx.h"

#include "RSXDisAsm.h"
#include "RSXThread.h"
#include "gcm_enums.h"
#include "gcm_printing.h"
#include "rsx_methods.h"

namespace rsx
{
	void invalid_method(thread*, u32, u32);
}

u32 RSXDisAsm::disasm(u32 pc)
{
	last_opcode.clear();

	u32 addr = static_cast<const rsx::thread*>(m_cpu)->iomap_table.get_addr(pc);

	if (addr == umax) return 0;

	m_op = *reinterpret_cast<const atomic_be_t<u32>*>(m_offset + addr);
	dump_pc = pc;

	if (m_op & RSX_METHOD_NON_METHOD_CMD_MASK)
	{
		if (m_mode == cpu_disasm_mode::list)
		{
			return 0;
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
			Write("?? ??", -1);
		}

		return 4;
	}
	else if ((m_op & RSX_METHOD_NOP_MASK) == RSX_METHOD_NOP_CMD)
	{
		if (m_mode == cpu_disasm_mode::list)
		{
			return 0;
		}

		u32 i = 1;

		for (pc += 4; i < 4096; i++, pc += 4)
		{
			addr = static_cast<const rsx::thread*>(m_cpu)->iomap_table.get_addr(pc);

			if (addr == umax)
			{
				break;
			}

			m_op = *reinterpret_cast<const atomic_be_t<u32>*>(m_offset + addr);

			if ((m_op & RSX_METHOD_NOP_MASK) != RSX_METHOD_NOP_CMD)
			{
				break;
			}
		}

		if (i == 1)
			Write("nop", 0);
		else
			Write(fmt::format("nop x%u", i), 0);

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
			Write("?? ??", -1);
			return 4;
		}

		pc += 4;

		for (u32 i = 0; i < (m_mode == cpu_disasm_mode::list ? count : 1); i++, pc += 4)
		{
			addr = static_cast<const rsx::thread*>(m_cpu)->iomap_table.get_addr(pc);

			if (addr == umax)
			{
				last_opcode.clear();
				Write("?? ??", -1);
				return 4;
			}

			m_op = *reinterpret_cast<const atomic_be_t<u32>*>(m_offset + addr);

			const u32 id = id_start + (non_inc ? 0 : i);

			if (rsx::methods[id] == &rsx::invalid_method)
			{
				last_opcode.clear();
				Write("?? ??", -1);
				return 4;
			}

			std::string str = rsx::get_pretty_printing_function(id)(id, m_op);
			Write(str, m_mode == cpu_disasm_mode::list ? i : count, non_inc, id);
		}

		return (count + 1) * 4;
	}
}

void RSXDisAsm::Write(const std::string& str, s32 count, bool is_non_inc, u32 id)
{
	switch (m_mode)
	{
	case cpu_disasm_mode::interpreter:
	{
		last_opcode = count >= 0 ? fmt::format("[%08x] (%s%u)", dump_pc, is_non_inc ? "+" : "", count) : 
			fmt::format("[%08x] (x)", dump_pc);

		auto& res = last_opcode;

		res.resize(7 + 11);
		std::replace(res.begin(), res.end(), '\0', ' ');

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
