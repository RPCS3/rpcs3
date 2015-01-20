#pragma once

#include "Emu/CPU/CPUDecoder.h"
#include "ARMv7Thread.h"
#include "ARMv7Interpreter.h"
#include "ARMv7Opcodes.h"
#include "Utilities/Log.h"

class ARMv7Decoder : public CPUDecoder
{
	ARMv7Thread& m_thr;

public:
	ARMv7Decoder(ARMv7Thread& thr) : m_thr(thr)
	{
	}

	virtual u8 DecodeMemory(const u32 address)
	{
		ARMv7Code code;
		code.code0 = vm::psv::read16(address & ~1);
		code.code1 = vm::psv::read16(address + 2 & ~1);
		u32 arg = address & 0x1 ? code.data : (u32)code.code0 << 16 | code.code1;

		LOG_NOTICE(GENERAL, "code0 = 0x%04x, code1 = 0x%04x, data = 0x%08x, arg = 0x%08x", code.code0, code.code1, code.data, arg);

		// old decoding algorithm
		
		for (auto& opcode : ARMv7_opcode_table)
		{
			if ((opcode.type < A1) == ((address & 0x1) == 0) && (arg & opcode.mask) == opcode.code)
			{
				code.data = opcode.length == 2 ? code.code0 : arg;
				(*opcode.func)(m_thr.context, code, opcode.type);
				// LOG_NOTICE(GENERAL, "%s, %d \n\n", opcode.name, opcode.length);
				return opcode.length;
			}
		}

		ARMv7_instrs::UNK(m_thr.context, code);
		return address & 0x1 ? 4 : 2;
		

		//execute_main_group(&m_thr);
		//// LOG_NOTICE(GENERAL, "%s, %d \n\n", m_thr.m_last_instr_name, m_thr.m_last_instr_size);
		//m_thr.m_last_instr_name = "Unknown";
		//return m_thr.m_last_instr_size;
	}
};