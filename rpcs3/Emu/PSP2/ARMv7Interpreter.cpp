#include "stdafx.h"
#include "Utilities/sysinfo.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "ARMv7Thread.h"
#include "ARMv7Interpreter.h"

const bool s_use_rtm = utils::has_rtm();

using namespace arm_code::arm_encoding_alias;

#define ARG(arg, ...) const u32 arg = args::arg::extract(__VA_ARGS__);

extern void arm_execute_function(ARMv7Thread& cpu, u32 index);

namespace vm { using namespace psv; }

void arm_interpreter::UNK(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	if (cpu.ISET == Thumb)
	{
		if (op > 0xffff)
		{
			fmt::throw_exception("Unknown/Illegal opcode: 0x%04X 0x%04X (cond=0x%x)", op >> 16, op & 0xffff, cond);
		}
		else
		{
			fmt::throw_exception("Unknown/Illegal opcode: 0x%04X (cond=0x%x)", op, cond);
		}
	}
	else
	{
		fmt::throw_exception("Unknown/Illegal opcode: 0x%08X", op);
	}
}

template<arm_encoding type>
void arm_interpreter::HACK(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::hack<type>;
	ARG(index, op);

	if (ConditionPassed(cpu, cond))
	{
		arm_execute_function(cpu, index);
	}
}


template<arm_encoding type>
void arm_interpreter::MRC_(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::mrc<type>;
	ARG(t, op);
	ARG(cp, op);
	ARG(opc1, op);
	ARG(opc2, op);
	ARG(cn, op);
	ARG(cm, op);

	if (ConditionPassed(cpu, cond))
	{
		// APSR flags are written if t = 15

		if (t < 15 && cp == 15 && opc1 == 0 && cn == 13 && cm == 0 && opc2 == 3)
		{
			// Read CP15 User Read-only Thread ID Register (seems used as TLS address)

			if (!cpu.TLS)
			{
				fmt::throw_exception("TLS not initialized" HERE);
			}

			cpu.GPR[t] = cpu.TLS;
			return;
		}

		fmt::throw_exception("mrc?? p%d,%d,r%d,c%d,c%d,%d" HERE, cp, opc1, t, cn, cm, opc2);
	}
}

template<arm_encoding type>
void arm_interpreter::ADC_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::adc_imm<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(set_flags, op, cond);

	if (ConditionPassed(cpu, cond))
	{
		bool carry, overflow;
		const u32 result = AddWithCarry<u32>(cpu.read_gpr(n), imm32, cpu.APSR.C, carry, overflow);
		cpu.write_gpr(d, result, 4);

		if (set_flags)
		{
			cpu.APSR.N = result >> 31;
			cpu.APSR.Z = result == 0;
			cpu.APSR.C = carry;
			cpu.APSR.V = overflow;
		}
	}
}

template<arm_encoding type>
void arm_interpreter::ADC_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::adc_reg<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(m, op);
	ARG(shift_t, op);
	ARG(shift_n, op);
	ARG(set_flags, op, cond);

	if (ConditionPassed(cpu, cond))
	{
		bool carry, overflow;
		const u32 shifted = Shift(cpu.read_gpr(m), shift_t, shift_n, cpu.APSR.C);
		const u32 result = AddWithCarry(cpu.read_gpr(n), shifted, cpu.APSR.C, carry, overflow);
		cpu.write_gpr(d, result, type == T1 ? 2 : 4);

		if (set_flags)
		{
			cpu.APSR.N = result >> 31;
			cpu.APSR.Z = result == 0;
			cpu.APSR.C = carry;
			cpu.APSR.V = overflow;
		}
	}
}

template<arm_encoding type>
void arm_interpreter::ADC_RSR(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::ADD_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::add_imm<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(set_flags, op, cond);

	if (ConditionPassed(cpu, cond))
	{
		bool carry, overflow;
		const u32 result = AddWithCarry<u32>(cpu.read_gpr(n), imm32, false, carry, overflow);
		cpu.write_gpr(d, result, type < T3 ? 2 : 4);

		if (set_flags)
		{
			cpu.APSR.N = result >> 31;
			cpu.APSR.Z = result == 0;
			cpu.APSR.C = carry;
			cpu.APSR.V = overflow;
		}
	}
}

template<arm_encoding type>
void arm_interpreter::ADD_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::add_reg<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(m, op);
	ARG(shift_t, op);
	ARG(shift_n, op);
	ARG(set_flags, op, cond);

	if (ConditionPassed(cpu, cond))
	{
		bool carry, overflow;
		const u32 shifted = Shift(cpu.read_gpr(m), shift_t, shift_n, true);
		const u32 result = AddWithCarry(cpu.read_gpr(n), shifted, false, carry, overflow);
		cpu.write_gpr(d, result, type < T3 ? 2 : 4);

		if (set_flags)
		{
			cpu.APSR.N = result >> 31;
			cpu.APSR.Z = result == 0;
			cpu.APSR.C = carry;
			cpu.APSR.V = overflow;
		}
	}
}

template<arm_encoding type>
void arm_interpreter::ADD_RSR(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::ADD_SPI(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::add_spi<type>;
	ARG(d, op);
	ARG(imm32, op);
	ARG(set_flags, op, cond);

	if (ConditionPassed(cpu, cond))
	{
		bool carry, overflow;
		const u32 result = AddWithCarry<u32>(cpu.SP, imm32, false, carry, overflow);
		cpu.write_gpr(d, result, type < T3 ? 2 : 4);

		if (set_flags)
		{
			cpu.APSR.N = result >> 31;
			cpu.APSR.Z = result == 0;
			cpu.APSR.C = carry;
			cpu.APSR.V = overflow;
		}
	}
}

template<arm_encoding type>
void arm_interpreter::ADD_SPR(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::add_spr<type>;
	ARG(d, op);
	ARG(m, op);
	ARG(shift_t, op);
	ARG(shift_n, op);
	ARG(set_flags, op, cond);

	if (ConditionPassed(cpu, cond))
	{
		bool carry, overflow;
		const u32 shifted = Shift(cpu.read_gpr(m), shift_t, shift_n, cpu.APSR.C);
		const u32 result = AddWithCarry(cpu.SP, shifted, false, carry, overflow);
		cpu.write_gpr(d, result, type < T3 ? 2 : 4);

		if (set_flags)
		{
			cpu.APSR.N = result >> 31;
			cpu.APSR.Z = result == 0;
			cpu.APSR.C = carry;
			cpu.APSR.V = overflow;
		}
	}
}


template<arm_encoding type>
void arm_interpreter::ADR(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::adr<type>;
	ARG(d, op);
	ARG(i, op);

	if (ConditionPassed(cpu, cond))
	{
		cpu.write_gpr(d, (cpu.read_pc() & ~3) + i, type == T1 ? 2 : 4);
	}
}


template<arm_encoding type>
void arm_interpreter::AND_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::and_imm<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(set_flags, op, cond);

	if (ConditionPassed(cpu, cond))
	{
		const u32 result = cpu.read_gpr(n) & imm32;
		cpu.write_gpr(d, result, 4);

		if (set_flags)
		{
			cpu.APSR.N = result >> 31;
			cpu.APSR.Z = result == 0;
			cpu.APSR.C = args::carry::extract(op, cpu.APSR.C);
		}
	}
}

template<arm_encoding type>
void arm_interpreter::AND_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::and_reg<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(m, op);
	ARG(shift_t, op);
	ARG(shift_n, op);
	ARG(set_flags, op, cond);

	if (ConditionPassed(cpu, cond))
	{
		bool carry;
		const u32 shifted = Shift_C(cpu.read_gpr(m), shift_t, shift_n, cpu.APSR.C, carry);
		const u32 result = cpu.read_gpr(n) & shifted;
		cpu.write_gpr(d, result, type == T1 ? 2 : 4);

		if (set_flags)
		{
			cpu.APSR.N = result >> 31;
			cpu.APSR.Z = result == 0;
			cpu.APSR.C = carry;
		}
	}
}

template<arm_encoding type>
void arm_interpreter::AND_RSR(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::ASR_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::ASR_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::B(ARMv7Thread& cpu, const u32 op, const u32 _cond)
{
	using args = arm_code::b<type>;
	ARG(imm32, op);

	if (ConditionPassed(cpu, args::cond::extract(op, _cond)))
	{
		cpu.PC = cpu.read_pc() + imm32 - (type < T3 ? 2 : 4);
	}
}


template<arm_encoding type>
void arm_interpreter::BFC(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::BFI(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::BIC_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::bic_imm<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(set_flags, op, cond);

	if (ConditionPassed(cpu, cond))
	{
		const u32 result = cpu.read_gpr(n) & ~imm32;
		cpu.write_gpr(d, result, 4);

		if (set_flags)
		{
			cpu.APSR.N = result >> 31;
			cpu.APSR.Z = result == 0;
			cpu.APSR.C = args::carry::extract(op, cpu.APSR.C);
		}
	}
}

template<arm_encoding type>
void arm_interpreter::BIC_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::bic_reg<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(m, op);
	ARG(shift_t, op);
	ARG(shift_n, op);
	ARG(set_flags, op, cond);

	if (ConditionPassed(cpu, cond))
	{
		bool carry;
		const u32 shifted = Shift_C(cpu.read_gpr(m), shift_t, shift_n, cpu.APSR.C, carry);
		const u32 result = cpu.read_gpr(n) & ~shifted;
		cpu.write_gpr(d, result, type == T1 ? 2 : 4);

		if (set_flags)
		{
			cpu.APSR.N = result >> 31;
			cpu.APSR.Z = result == 0;
			cpu.APSR.C = carry;
		}
	}
}

template<arm_encoding type>
void arm_interpreter::BIC_RSR(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::BKPT(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::BL(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::bl<type>;
	ARG(imm32, op);
	ARG(to_arm);

	if (ConditionPassed(cpu, cond))
	{
		cpu.LR = (cpu.PC + 4) | (cpu.ISET != ARM);

		// TODO: this is quite a mess
		if ((cpu.ISET == ARM) == to_arm)
		{
			const u32 pc = cpu.ISET == ARM ? (cpu.read_pc() & ~3) + imm32 : cpu.read_pc() + imm32;

			cpu.PC = pc - 4;
		}
		else
		{
			const u32 pc = type == T2 ? ~3 & cpu.PC + 4 + imm32 : 1 | cpu.PC + 8 + imm32;

			cpu.write_pc(pc, type == T1 ? 2 : 4);
		}
	}
}

template<arm_encoding type>
void arm_interpreter::BLX(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::blx<type>;
	ARG(m, op);

	if (ConditionPassed(cpu, cond))
	{
		cpu.LR = type == T1 ? (cpu.PC + 2) | 1 : cpu.PC + 4;
		cpu.write_pc(cpu.read_gpr(m), type == T1 ? 2 : 4);
	}
}

template<arm_encoding type>
void arm_interpreter::BX(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::bx<type>;
	ARG(m, op);

	if (ConditionPassed(cpu, cond))
	{
		cpu.write_pc(cpu.read_gpr(m), type == T1 ? 2 : 4);
	}
}


template<arm_encoding type>
void arm_interpreter::CB_Z(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::cb_z<type>;
	ARG(n, op);
	ARG(imm32, op);
	ARG(nonzero, op);

	if ((cpu.read_gpr(n) == 0) ^ nonzero)
	{
		cpu.PC = cpu.read_pc() + imm32 - 2;
	}
}


template<arm_encoding type>
void arm_interpreter::CLZ(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::clz<type>;
	ARG(d, op);
	ARG(m, op);

	if (ConditionPassed(cpu, cond))
	{
		cpu.write_gpr(d, cntlz32(cpu.read_gpr(m)), 4);
	}
}


template<arm_encoding type>
void arm_interpreter::CMN_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::CMN_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::CMN_RSR(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::CMP_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::cmp_imm<type>;
	ARG(n, op);
	ARG(imm32, op);

	if (ConditionPassed(cpu, cond))
	{
		bool carry, overflow;
		const u32 n_value = cpu.read_gpr(n);
		const u32 result = AddWithCarry(n_value, ~imm32, true, carry, overflow);
		cpu.APSR.N = result >> 31;
		cpu.APSR.Z = result == 0;
		cpu.APSR.C = carry;
		cpu.APSR.V = overflow;

		//LOG_NOTICE(ARMv7, "CMP: r%d=0x%08x <> 0x%08x, res=0x%08x", n, n_value, imm32, res);
	}
}

template<arm_encoding type>
void arm_interpreter::CMP_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::cmp_reg<type>;
	ARG(n, op);
	ARG(m, op);
	ARG(shift_t, op);
	ARG(shift_n, op);

	if (ConditionPassed(cpu, cond))
	{
		bool carry, overflow;
		const u32 m_value = cpu.read_gpr(m);
		const u32 n_value = cpu.read_gpr(n);
		const u32 shifted = Shift(m_value, shift_t, shift_n, true);
		const u32 result = AddWithCarry(n_value, ~shifted, true, carry, overflow);
		cpu.APSR.N = result >> 31;
		cpu.APSR.Z = result == 0;
		cpu.APSR.C = carry;
		cpu.APSR.V = overflow;

		//LOG_NOTICE(ARMv7, "CMP: r%d=0x%08x <> r%d=0x%08x, shifted=0x%08x, res=0x%08x", n, n_value, m, m_value, shifted, res);
	}
}

template<arm_encoding type>
void arm_interpreter::CMP_RSR(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::DBG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::DMB(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::DSB(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::EOR_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::eor_imm<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(set_flags, op, cond);

	if (ConditionPassed(cpu, cond))
	{
		const u32 result = cpu.read_gpr(n) ^ imm32;
		cpu.write_gpr(d, result, 4);

		if (set_flags)
		{
			cpu.APSR.N = result >> 31;
			cpu.APSR.Z = result == 0;
			cpu.APSR.C = args::carry::extract(op, cpu.APSR.C);
		}
	}
}

template<arm_encoding type>
void arm_interpreter::EOR_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::eor_reg<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(m, op);
	ARG(shift_t, op);
	ARG(shift_n, op);
	ARG(set_flags, op, cond);

	if (ConditionPassed(cpu, cond))
	{
		bool carry;
		const u32 shifted = Shift_C(cpu.read_gpr(m), shift_t, shift_n, cpu.APSR.C, carry);
		const u32 result = cpu.read_gpr(n) ^ shifted;
		cpu.write_gpr(d, result, type == T1 ? 2 : 4);

		if (set_flags)
		{
			cpu.APSR.N = result >> 31;
			cpu.APSR.Z = result == 0;
			cpu.APSR.C = carry;
		}
	}
}

template<arm_encoding type>
void arm_interpreter::EOR_RSR(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::IT(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	cpu.ITSTATE.IT = op & 0xff;
}


template<arm_encoding type>
void arm_interpreter::LDM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::ldm<type>;
	ARG(n, op);
	ARG(registers, op);
	ARG(wback, op);

	if (ConditionPassed(cpu, cond))
	{
		vm::ptr<u32> memory(vm::cast(cpu.read_gpr(n)));

		for (u32 i = 0; i < 16; i++)
		{
			if (registers & (1 << i))
			{
				cpu.write_gpr(i, *memory++, type == T1 ? 2 : 4);
			}
		}
		
		// Warning: wback set true for T1
		if (wback && ~registers & (1 << n))
		{
			cpu.write_gpr(n, memory.addr(), type == T1 ? 2 : 4);
		}
	}
}

template<arm_encoding type>
void arm_interpreter::LDMDA(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::LDMDB(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::LDMIB(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::LDR_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::ldr_imm<type>;
	ARG(t, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(index, op);
	ARG(add, op);
	ARG(wback, op);

	if (ConditionPassed(cpu, cond))
	{
		const u32 offset_addr = add ? cpu.read_gpr(n) + imm32 : cpu.read_gpr(n) - imm32;
		const u32 addr = index ? offset_addr : cpu.read_gpr(n);
		cpu.write_gpr(t, vm::read32(addr), type < T3 ? 2 : 4);

		if (wback)
		{
			cpu.write_gpr(n, offset_addr, type < T3 ? 2 : 4);
		}
	}
}

template<arm_encoding type>
void arm_interpreter::LDR_LIT(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::ldr_lit<type>;
	ARG(t, op);
	ARG(imm32, op);
	ARG(add, op);

	const u32 base = cpu.read_pc() & ~3;
	const u32 addr = add ? base + imm32 : base - imm32;

	if (ConditionPassed(cpu, cond))
	{
		const u32 data = vm::read32(addr);
		cpu.write_gpr(t, data, type == T1 ? 2 : 4);
	}
}

template<arm_encoding type>
void arm_interpreter::LDR_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::ldr_reg<type>;
	ARG(t, op);
	ARG(n, op);
	ARG(m, op);
	ARG(shift_t, op);
	ARG(shift_n, op);
	ARG(index, op);
	ARG(add, op);
	ARG(wback, op);

	if (ConditionPassed(cpu, cond))
	{
		const u32 offset = Shift(cpu.read_gpr(m), shift_t, shift_n, cpu.APSR.C);
		const u32 offset_addr = add ? cpu.read_gpr(n) + offset : cpu.read_gpr(n) - offset;
		const u32 addr = index ? offset_addr : cpu.read_gpr(n);
		cpu.write_gpr(t, vm::read32(addr), type == T1 ? 2 : 4);

		if (wback)
		{
			cpu.write_gpr(n, offset_addr, type == T1 ? 2 : 4);
		}
	}
}


template<arm_encoding type>
void arm_interpreter::LDRB_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::ldrb_imm<type>;
	ARG(t, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(index, op);
	ARG(add, op);
	ARG(wback, op);

	if (ConditionPassed(cpu, cond))
	{
		const u32 offset_addr = add ? cpu.read_gpr(n) + imm32 : cpu.read_gpr(n) - imm32;
		const u32 addr = index ? offset_addr : cpu.read_gpr(n);
		cpu.write_gpr(t, vm::read8(addr), type == T1 ? 2 : 4);

		if (wback)
		{
			cpu.write_gpr(n, offset_addr, type == T1 ? 2 : 4);
		}
	}
}

template<arm_encoding type>
void arm_interpreter::LDRB_LIT(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::LDRB_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::ldrb_reg<type>;
	ARG(t, op);
	ARG(n, op);
	ARG(m, op);
	ARG(shift_t, op);
	ARG(shift_n, op);
	ARG(index, op);
	ARG(add, op);
	ARG(wback, op);

	if (ConditionPassed(cpu, cond))
	{
		const u32 offset = Shift(cpu.read_gpr(m), shift_t, shift_n, cpu.APSR.C);
		const u32 offset_addr = add ? cpu.read_gpr(n) + offset : cpu.read_gpr(n) - offset;
		const u32 addr = index ? offset_addr : cpu.read_gpr(n);
		cpu.write_gpr(t, vm::read8(addr), type == T1 ? 2 : 4);

		if (wback)
		{
			cpu.write_gpr(n, offset_addr, type == T1 ? 2 : 4);
		}
	}
}


template<arm_encoding type>
void arm_interpreter::LDRD_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::ldrd_imm<type>;
	ARG(t, op);
	ARG(t2, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(index, op);
	ARG(add, op);
	ARG(wback, op);

	if (ConditionPassed(cpu, cond))
	{
		const u32 offset_addr = add ? cpu.read_gpr(n) + imm32 : cpu.read_gpr(n) - imm32;
		const u32 addr = index ? offset_addr : cpu.read_gpr(n);
		const u64 value = vm::read64(addr);
		cpu.write_gpr(t, (u32)(value), 4);
		cpu.write_gpr(t2, (u32)(value >> 32), 4);

		if (wback)
		{
			cpu.write_gpr(n, offset_addr, 4);
		}
	}
}

template<arm_encoding type>
void arm_interpreter::LDRD_LIT(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::ldrd_lit<type>;
	ARG(t, op);
	ARG(t2, op);
	ARG(imm32, op);
	ARG(add, op);

	const u32 base = cpu.read_pc() & ~3;
	const u32 addr = add ? base + imm32 : base - imm32;

	if (ConditionPassed(cpu, cond))
	{
		const u64 value = vm::read64(addr);
		cpu.write_gpr(t, (u32)(value), 4);
		cpu.write_gpr(t2, (u32)(value >> 32), 4);
	}
}

template<arm_encoding type>
void arm_interpreter::LDRD_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::LDRH_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::ldrh_imm<type>;
	ARG(t, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(index, op);
	ARG(add, op);
	ARG(wback, op);

	if (ConditionPassed(cpu, cond))
	{
		const u32 offset_addr = add ? cpu.read_gpr(n) + imm32 : cpu.read_gpr(n) - imm32;
		const u32 addr = index ? offset_addr : cpu.read_gpr(n);
		cpu.write_gpr(t, vm::read16(addr), type == T1 ? 2 : 4);

		if (wback)
		{
			cpu.write_gpr(n, offset_addr, type == T1 ? 2 : 4);
		}
	}
}

template<arm_encoding type>
void arm_interpreter::LDRH_LIT(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::LDRH_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::LDRSB_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::ldrsb_imm<type>;
	ARG(t, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(index, op);
	ARG(add, op);
	ARG(wback, op);

	if (ConditionPassed(cpu, cond))
	{
		const u32 offset_addr = add ? cpu.read_gpr(n) + imm32 : cpu.read_gpr(n) - imm32;
		const u32 addr = index ? offset_addr : cpu.read_gpr(n);
		const s8 value = vm::read8(addr);
		cpu.write_gpr(t, value, 4); // sign-extend

		if (wback)
		{
			cpu.write_gpr(n, offset_addr, 4);
		}
	}
}

template<arm_encoding type>
void arm_interpreter::LDRSB_LIT(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::LDRSB_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::LDRSH_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::LDRSH_LIT(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::LDRSH_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::LDREX(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::ldrex<type>;
	ARG(t, op);
	ARG(n, op);
	ARG(imm32, op);

	if (ConditionPassed(cpu, cond))
	{
		const u32 addr = cpu.read_gpr(n) + imm32;

		cpu.rtime = vm::reservation_acquire(addr, sizeof(u32));
		_mm_lfence();
		cpu.raddr = addr;
		cpu.rdata = vm::_ref<const atomic_le_t<u32>>(addr);

		cpu.write_gpr(t, cpu.rdata, 4);
	}
}

template<arm_encoding type>
void arm_interpreter::LDREXB(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::LDREXD(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::LDREXH(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::LSL_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::lsl_imm<type>;
	ARG(d, op);
	ARG(m, op);
	ARG(shift_n, op);
	ARG(set_flags, op, cond);

	if (ConditionPassed(cpu, cond))
	{
		bool carry;
		const u32 result = Shift_C(cpu.read_gpr(m), arm_code::SRType_LSL, shift_n, cpu.APSR.C, carry);
		cpu.write_gpr(d, result, type == T1 ? 2 : 4);

		if (set_flags)
		{
			cpu.APSR.N = result >> 31;
			cpu.APSR.Z = result == 0;
			cpu.APSR.C = carry;
		}
	}
}

template<arm_encoding type>
void arm_interpreter::LSL_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::lsl_reg<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(m, op);
	ARG(set_flags, op, cond);

	if (ConditionPassed(cpu, cond))
	{
		bool carry;
		const u32 result = Shift_C(cpu.read_gpr(n), arm_code::SRType_LSL, (cpu.read_gpr(m) & 0xff), cpu.APSR.C, carry);
		cpu.write_gpr(d, result, type == T1 ? 2 : 4);

		if (set_flags)
		{
			cpu.APSR.N = result >> 31;
			cpu.APSR.Z = result == 0;
			cpu.APSR.C = carry;
		}
	}
}


template<arm_encoding type>
void arm_interpreter::LSR_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::lsr_imm<type>;
	ARG(d, op);
	ARG(m, op);
	ARG(shift_n, op);
	ARG(set_flags, op, cond);

	if (ConditionPassed(cpu, cond))
	{
		bool carry;
		const u32 result = Shift_C(cpu.read_gpr(m), arm_code::SRType_LSR, shift_n, cpu.APSR.C, carry);
		cpu.write_gpr(d, result, type == T1 ? 2 : 4);

		if (set_flags)
		{
			cpu.APSR.N = result >> 31;
			cpu.APSR.Z = result == 0;
			cpu.APSR.C = carry;
		}
	}
}

template<arm_encoding type>
void arm_interpreter::LSR_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::MLA(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::MLS(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::MOV_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::mov_imm<type>;
	ARG(d, op);
	ARG(imm32, op);
	ARG(set_flags, op, cond);

	if (ConditionPassed(cpu, cond))
	{
		const u32 result = imm32;
		cpu.write_gpr(d, result, type == T1 ? 2 : 4);

		if (set_flags)
		{
			cpu.APSR.N = result >> 31;
			cpu.APSR.Z = result == 0;
			cpu.APSR.C = args::carry::extract(op, cpu.APSR.C);
		}
	}
}

template<arm_encoding type>
void arm_interpreter::MOV_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::mov_reg<type>;
	ARG(d, op);
	ARG(m, op);
	ARG(set_flags, op, cond);

	if (ConditionPassed(cpu, cond))
	{
		const u32 result = cpu.read_gpr(m);
		cpu.write_gpr(d, result, type < T3 ? 2 : 4);

		if (set_flags) // cond is not used
		{
			cpu.APSR.N = result >> 31;
			cpu.APSR.Z = result == 0;
			//cpu.APSR.C = carry;
		}
	}
}

template<arm_encoding type>
void arm_interpreter::MOVT(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::movt<type>;
	ARG(d, op);
	ARG(imm16, op);

	if (ConditionPassed(cpu, cond))
	{
		cpu.write_gpr(d, (cpu.read_gpr(d) & 0xffff) | (imm16 << 16), 4);
	}
}


template<arm_encoding type>
void arm_interpreter::MRS(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::MSR_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::MSR_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::MUL(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::mul<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(m, op);
	ARG(set_flags, op, cond);

	if (ConditionPassed(cpu, cond))
	{
		const u32 op1 = cpu.read_gpr(n);
		const u32 op2 = cpu.read_gpr(m);
		const u32 result = op1 * op2;
		cpu.write_gpr(d, result, type == T1 ? 2 : 4);

		if (set_flags)
		{
			cpu.APSR.N = result >> 31;
			cpu.APSR.Z = result == 0;
		}
	}
}


template<arm_encoding type>
void arm_interpreter::MVN_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::mvn_imm<type>;
	ARG(d, op);
	ARG(imm32, op);
	ARG(set_flags, op, cond);

	if (ConditionPassed(cpu, cond))
	{
		const u32 result = ~imm32;
		cpu.write_gpr(d, result, 4);

		if (set_flags)
		{
			cpu.APSR.N = result >> 31;
			cpu.APSR.Z = result == 0;
			cpu.APSR.C = args::carry::extract(op, cpu.APSR.C);
		}
	}
}

template<arm_encoding type>
void arm_interpreter::MVN_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::mvn_reg<type>;
	ARG(d, op);
	ARG(m, op);
	ARG(shift_t, op);
	ARG(shift_n, op);
	ARG(set_flags, op, cond);

	if (ConditionPassed(cpu, cond))
	{
		bool carry;
		const u32 shifted = Shift_C(cpu.read_gpr(m), shift_t, shift_n, cpu.APSR.C, carry);
		const u32 result = ~shifted;
		cpu.write_gpr(d, result, type == T1 ? 2 : 4);

		if (set_flags)
		{
			cpu.APSR.N = result >> 31;
			cpu.APSR.Z = result == 0;
			cpu.APSR.C = carry;
		}
	}
}

template<arm_encoding type>
void arm_interpreter::MVN_RSR(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::NOP(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	if (ConditionPassed(cpu, cond))
	{
	}
}


template<arm_encoding type>
void arm_interpreter::ORN_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::ORN_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::ORR_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::orr_imm<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(set_flags, op, cond);

	if (ConditionPassed(cpu, cond))
	{
		const u32 result = cpu.read_gpr(n) | imm32;
		cpu.write_gpr(d, result, 4);

		if (set_flags)
		{
			cpu.APSR.N = result >> 31;
			cpu.APSR.Z = result == 0;
			cpu.APSR.C = args::carry::extract(op, cpu.APSR.C);
		}
	}
}

template<arm_encoding type>
void arm_interpreter::ORR_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::orr_reg<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(m, op);
	ARG(shift_t, op);
	ARG(shift_n, op);
	ARG(set_flags, op, cond);

	if (ConditionPassed(cpu, cond))
	{
		bool carry;
		const u32 shifted = Shift_C(cpu.read_gpr(m), shift_t, shift_n, cpu.APSR.C, carry);
		const u32 result = cpu.read_gpr(n) | shifted;
		cpu.write_gpr(d, result, type == T1 ? 2 : 4);

		if (set_flags)
		{
			cpu.APSR.N = result >> 31;
			cpu.APSR.Z = result == 0;
			cpu.APSR.C = carry;
		}
	}
}

template<arm_encoding type>
void arm_interpreter::ORR_RSR(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::PKH(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::POP(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	const u32 registers = arm_code::pop<type>::registers::extract(op);

	if (ConditionPassed(cpu, cond))
	{
		auto stack = vm::ptr<u32>::make(cpu.SP);

		for (u32 i = 0; i < 16; i++)
		{
			if (registers & (1 << i))
			{
				cpu.write_gpr(i, *stack++, type == T1 ? 2 : 4);
			}
		}

		cpu.SP = stack.addr();
	}
}

template<arm_encoding type>
void arm_interpreter::PUSH(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	const u32 registers = arm_code::push<type>::registers::extract(op);

	if (ConditionPassed(cpu, cond))
	{
		vm::ptr<u32> memory(vm::cast(cpu.SP));

		for (u32 i = 15; ~i; i--)
		{
			if (registers & (1 << i))
			{
				*--memory = cpu.read_gpr(i);
			}
		}

		cpu.SP = memory.addr();
	}
}


template<arm_encoding type>
void arm_interpreter::QADD(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::QADD16(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::QADD8(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::QASX(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::QDADD(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::QDSUB(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::QSAX(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::QSUB(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::QSUB16(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::QSUB8(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::RBIT(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::REV(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::rev<type>;
	ARG(d, op);
	ARG(m, op);

	if (ConditionPassed(cpu, cond))
	{
		cpu.write_gpr(d, se_storage<u32>::swap(cpu.read_gpr(m)), type == T1 ? 2 : 4);
	}
}

template<arm_encoding type>
void arm_interpreter::REV16(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::REVSH(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::ROR_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::ror_imm<type>;
	ARG(d, op);
	ARG(m, op);
	ARG(shift_n, op);
	ARG(set_flags, op, cond);

	if (ConditionPassed(cpu, cond))
	{
		bool carry;
		const u32 result = Shift_C(cpu.read_gpr(m), arm_code::SRType_ROR, shift_n, cpu.APSR.C, carry);
		cpu.write_gpr(d, result, 4);

		if (set_flags)
		{
			cpu.APSR.N = result >> 31;
			cpu.APSR.Z = result == 0;
			cpu.APSR.C = carry;
		}
	}
}

template<arm_encoding type>
void arm_interpreter::ROR_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::ror_reg<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(m, op);
	ARG(set_flags, op, cond);

	if (ConditionPassed(cpu, cond))
	{
		bool carry;
		const u32 shift_n = cpu.read_gpr(m) & 0xff;
		const u32 result = Shift_C(cpu.read_gpr(n), arm_code::SRType_ROR, shift_n, cpu.APSR.C, carry);
		cpu.write_gpr(d, result, type == T1 ? 2 : 4);

		if (set_flags)
		{
			cpu.APSR.N = result >> 31;
			cpu.APSR.Z = result == 0;
			cpu.APSR.C = carry;
		}
	}
}


template<arm_encoding type>
void arm_interpreter::RRX(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::RSB_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::rsb_imm<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(set_flags, op, cond);

	if (ConditionPassed(cpu, cond))
	{
		bool carry, overflow;
		const u32 result = AddWithCarry(~cpu.read_gpr(n), imm32, true, carry, overflow);
		cpu.write_gpr(d, result, type == T1 ? 2 : 4);

		if (set_flags)
		{
			cpu.APSR.N = result >> 31;
			cpu.APSR.Z = result == 0;
			cpu.APSR.C = carry;
			cpu.APSR.V = overflow;
		}
	}
}

template<arm_encoding type>
void arm_interpreter::RSB_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::RSB_RSR(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::RSC_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::RSC_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::RSC_RSR(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::SADD16(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SADD8(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SASX(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::SBC_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SBC_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SBC_RSR(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::SBFX(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::SDIV(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::SEL(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::SHADD16(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SHADD8(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SHASX(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SHSAX(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SHSUB16(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SHSUB8(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::SMLA__(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SMLAD(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SMLAL(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SMLAL__(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SMLALD(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SMLAW_(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SMLSD(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SMLSLD(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SMMLA(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SMMLS(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SMMUL(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SMUAD(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SMUL__(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SMULL(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SMULW_(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SMUSD(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::SSAT(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SSAT16(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SSAX(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SSUB16(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SSUB8(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::STM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::stm<type>;
	ARG(n, op);
	ARG(registers, op);
	ARG(wback, op);

	if (ConditionPassed(cpu, cond))
	{
		auto memory = vm::ptr<u32>::make(cpu.read_gpr(n));

		for (u32 i = 0; i < 16; i++)
		{
			if (registers & (1 << i))
			{
				*memory++ = cpu.read_gpr(i);
			}
		}

		if (wback)
		{
			cpu.write_gpr(n, memory.addr(), type == T1 ? 2 : 4);
		}
	}
}

template<arm_encoding type>
void arm_interpreter::STMDA(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::STMDB(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::STMIB(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::STR_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::str_imm<type>;
	ARG(t, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(index, op);
	ARG(add, op);
	ARG(wback, op);

	if (ConditionPassed(cpu, cond))
	{
		const u32 offset_addr = add ? cpu.read_gpr(n) + imm32 : cpu.read_gpr(n) - imm32;
		const u32 addr = index ? offset_addr : cpu.read_gpr(n);
		vm::write32(addr, cpu.read_gpr(t));

		if (wback)
		{
			cpu.write_gpr(n, offset_addr, type < T3 ? 2 : 4);
		}
	}
}

template<arm_encoding type>
void arm_interpreter::STR_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::str_reg<type>;
	ARG(t, op);
	ARG(n, op);
	ARG(m, op);
	ARG(shift_t, op);
	ARG(shift_n, op);
	ARG(index, op);
	ARG(add, op);
	ARG(wback, op);

	if (ConditionPassed(cpu, cond))
	{
		const u32 offset = Shift(cpu.read_gpr(m), shift_t, shift_n, cpu.APSR.C);
		const u32 offset_addr = add ? cpu.read_gpr(n) + offset : cpu.read_gpr(n) - offset;
		const u32 addr = index ? offset_addr : cpu.read_gpr(n);
		vm::write32(addr, cpu.read_gpr(t));

		if (wback)
		{
			cpu.write_gpr(n, offset_addr, type == T1 ? 2 : 4);
		}
	}
}


template<arm_encoding type>
void arm_interpreter::STRB_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::strb_imm<type>;
	ARG(t, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(index, op);
	ARG(add, op);
	ARG(wback, op);

	if (ConditionPassed(cpu, cond))
	{
		const u32 offset_addr = add ? cpu.read_gpr(n) + imm32 : cpu.read_gpr(n) - imm32;
		const u32 addr = index ? offset_addr : cpu.read_gpr(n);
		vm::write8(addr, (u8)cpu.read_gpr(t));

		if (wback)
		{
			cpu.write_gpr(n, offset_addr, type == T1 ? 2 : 4);
		}
	}
}

template<arm_encoding type>
void arm_interpreter::STRB_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::strb_reg<type>;
	ARG(t, op);
	ARG(n, op);
	ARG(m, op);
	ARG(shift_t, op);
	ARG(shift_n, op);
	ARG(index, op);
	ARG(add, op);
	ARG(wback, op);

	if (ConditionPassed(cpu, cond))
	{
		const u32 offset = Shift(cpu.read_gpr(m), shift_t, shift_n, cpu.APSR.C);
		const u32 offset_addr = add ? cpu.read_gpr(n) + offset : cpu.read_gpr(n) - offset;
		const u32 addr = index ? offset_addr : cpu.read_gpr(n);
		vm::write8(addr, (u8)cpu.read_gpr(t));

		if (wback)
		{
			cpu.write_gpr(n, offset_addr, type == T1 ? 2 : 4);
		}
	}
}


template<arm_encoding type>
void arm_interpreter::STRD_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::strd_imm<type>;
	ARG(t, op);
	ARG(t2, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(index, op);
	ARG(add, op);
	ARG(wback, op);

	if (ConditionPassed(cpu, cond))
	{
		const u32 n_value = cpu.read_gpr(n);
		const u32 offset = add ? n_value + imm32 : n_value - imm32;
		const u32 addr = index ? offset : n_value;
		vm::write64(addr, (u64)cpu.read_gpr(t2) << 32 | (u64)cpu.read_gpr(t));

		if (wback)
		{
			cpu.write_gpr(n, offset, 4);
		}
	}
}

template<arm_encoding type>
void arm_interpreter::STRD_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::STRH_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::strh_imm<type>;
	ARG(t, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(index, op);
	ARG(add, op);
	ARG(wback, op);

	if (ConditionPassed(cpu, cond))
	{
		const u32 offset_addr = add ? cpu.read_gpr(n) + imm32 : cpu.read_gpr(n) - imm32;
		const u32 addr = index ? offset_addr : cpu.read_gpr(n);
		vm::write16(addr, (u16)cpu.read_gpr(t));

		if (wback)
		{
			cpu.write_gpr(n, offset_addr, type == T1 ? 2 : 4);
		}
	}
}

template<arm_encoding type>
void arm_interpreter::STRH_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::strh_reg<type>;
	ARG(t, op);
	ARG(n, op);
	ARG(m, op);
	ARG(shift_t, op);
	ARG(shift_n, op);
	ARG(index, op);
	ARG(add, op);
	ARG(wback, op);

	if (ConditionPassed(cpu, cond))
	{
		const u32 offset = Shift(cpu.read_gpr(m), shift_t, shift_n, cpu.APSR.C);
		const u32 offset_addr = add ? cpu.read_gpr(n) + offset : cpu.read_gpr(n) - offset;
		const u32 addr = index ? offset_addr : cpu.read_gpr(n);
		vm::write16(addr, (u16)cpu.read_gpr(t));

		if (wback)
		{
			cpu.write_gpr(n, offset_addr, type == T1 ? 2 : 4);
		}
	}
}


template<arm_encoding type>
void arm_interpreter::STREX(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::strex<type>;
	ARG(d, op);
	ARG(t, op);
	ARG(n, op);
	ARG(imm32, op);

	if (ConditionPassed(cpu, cond))
	{
		const u32 addr = cpu.read_gpr(n) + imm32;
		const u32 value = cpu.read_gpr(t);

		atomic_le_t<u32>& data = vm::_ref<atomic_le_t<u32>>(addr);

		if (cpu.raddr != addr || cpu.rdata != data.load())
		{
			// Failure
			cpu.raddr = 0;
			cpu.write_gpr(d, true, 4);
			return;
		}

		bool result;

		if (s_use_rtm && utils::transaction_enter())
		{
			if (!vm::reader_lock{vm::try_to_lock})
			{
				_xabort(0);
			}

			result = cpu.rtime == vm::reservation_acquire(addr, sizeof(u32)) && data.compare_and_swap_test(cpu.rdata, value);

			if (result)
			{
				vm::reservation_update(addr, sizeof(u32));
			}

			_xend();
		}
		else
		{
			vm::writer_lock lock(0);

			result = cpu.rtime == vm::reservation_acquire(addr, sizeof(u32)) && data.compare_and_swap_test(cpu.rdata, value);

			if (result)
			{
				vm::reservation_update(addr, sizeof(u32));
			}
		}
		
		cpu.raddr = 0;
		cpu.write_gpr(d, !result, 4);
	}
}

template<arm_encoding type>
void arm_interpreter::STREXB(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::STREXD(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::STREXH(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::SUB_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::sub_imm<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(set_flags, op, cond);

	if (ConditionPassed(cpu, cond))
	{
		bool carry, overflow;
		const u32 result = AddWithCarry(cpu.read_gpr(n), ~imm32, true, carry, overflow);
		cpu.write_gpr(d, result, type < T3 ? 2 : 4);

		if (set_flags)
		{
			cpu.APSR.N = result >> 31;
			cpu.APSR.Z = result == 0;
			cpu.APSR.C = carry;
			cpu.APSR.V = overflow;
		}
	}
}

template<arm_encoding type>
void arm_interpreter::SUB_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::sub_reg<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(m, op);
	ARG(shift_t, op);
	ARG(shift_n, op);
	ARG(set_flags, op, cond);

	if (ConditionPassed(cpu, cond))
	{
		bool carry, overflow;
		const u32 shifted = Shift(cpu.read_gpr(m), shift_t, shift_n, cpu.APSR.C);
		const u32 result = AddWithCarry(cpu.read_gpr(n), ~shifted, true, carry, overflow);
		cpu.write_gpr(d, result, type == T1 ? 2 : 4);

		if (set_flags)
		{
			cpu.APSR.N = result >> 31;
			cpu.APSR.Z = result == 0;
			cpu.APSR.C = carry;
			cpu.APSR.V = overflow;
		}
	}
}

template<arm_encoding type>
void arm_interpreter::SUB_RSR(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SUB_SPI(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::sub_spi<type>;
	ARG(d, op);
	ARG(imm32, op);
	ARG(set_flags, op, cond);

	if (ConditionPassed(cpu, cond))
	{
		bool carry, overflow;
		const u32 result = AddWithCarry(cpu.SP, ~imm32, true, carry, overflow);
		cpu.write_gpr(d, result, type == T1 ? 2 : 4);

		if (set_flags)
		{
			cpu.APSR.N = result >> 31;
			cpu.APSR.Z = result == 0;
			cpu.APSR.C = carry;
			cpu.APSR.V = overflow;
		}
	}
}

template<arm_encoding type>
void arm_interpreter::SUB_SPR(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::SVC(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::SXTAB(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SXTAB16(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SXTAH(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SXTB(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SXTB16(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::SXTH(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::TB_(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::TEQ_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::TEQ_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::TEQ_RSR(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::TST_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::tst_imm<type>;
	ARG(n, op);
	ARG(imm32, op);

	if (ConditionPassed(cpu, cond))
	{
		const u32 result = cpu.read_gpr(n) & imm32;
		cpu.APSR.N = result >> 31;
		cpu.APSR.Z = result == 0;
		cpu.APSR.C = args::carry::extract(op, cpu.APSR.C);
	}
}

template<arm_encoding type>
void arm_interpreter::TST_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::TST_RSR(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::UADD16(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::UADD8(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::UASX(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::UBFX(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::UDIV(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::UHADD16(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::UHADD8(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::UHASX(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::UHSAX(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::UHSUB16(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::UHSUB8(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::UMAAL(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::UMLAL(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::UMULL(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::umull<type>;
	ARG(d0, op);
	ARG(d1, op);
	ARG(n, op);
	ARG(m, op);
	ARG(set_flags, op, cond);

	if (ConditionPassed(cpu, cond))
	{
		const u64 result = (u64)cpu.read_gpr(n) * (u64)cpu.read_gpr(m);
		cpu.write_gpr(d1, (u32)(result >> 32), 4);
		cpu.write_gpr(d0, (u32)(result), 4);

		if (set_flags)
		{
			cpu.APSR.N = result >> 63 != 0;
			cpu.APSR.Z = result == 0;
		}
	}
}

template<arm_encoding type>
void arm_interpreter::UQADD16(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::UQADD8(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::UQASX(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::UQSAX(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::UQSUB16(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::UQSUB8(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::USAD8(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::USADA8(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::USAT(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::USAT16(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::USAX(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::USUB16(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::USUB8(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::UXTAB(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::UXTAB16(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::UXTAH(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::UXTB(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	using args = arm_code::uxtb<type>;
	ARG(d, op);
	ARG(m, op);
	ARG(rotation, op);

	if (ConditionPassed(cpu, cond))
	{
		cpu.write_gpr(d, u8(cpu.read_gpr(m) >> rotation), type == T1 ? 2 : 4);
	}
}

template<arm_encoding type>
void arm_interpreter::UXTB16(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::UXTH(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::VABA_(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VABD_(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VABD_FP(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VABS(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VAC__(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VADD(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VADD_FP(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VADDHN(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VADD_(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VAND(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VBIC_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VBIC_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VB__(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VCEQ_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VCEQ_ZERO(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VCGE_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VCGE_ZERO(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VCGT_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VCGT_ZERO(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VCLE_ZERO(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VCLS(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VCLT_ZERO(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VCLZ(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VCMP_(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VCNT(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VCVT_FIA(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VCVT_FIF(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VCVT_FFA(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VCVT_FFF(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VCVT_DF(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VCVT_HFA(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VCVT_HFF(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VDIV(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VDUP_S(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VDUP_R(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VEOR(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VEXT(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VHADDSUB(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VLD__MS(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VLD1_SL(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VLD1_SAL(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VLD2_SL(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VLD2_SAL(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VLD3_SL(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VLD3_SAL(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VLD4_SL(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VLD4_SAL(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VLDM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VLDR(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VMAXMIN(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VMAXMIN_FP(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VML__(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VML__FP(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VML__S(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VMOV_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VMOV_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VMOV_RS(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VMOV_SR(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VMOV_RF(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VMOV_2RF(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VMOV_2RD(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VMOVL(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VMOVN(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VMRS(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VMSR(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VMUL_(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VMUL_FP(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VMUL_S(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VMVN_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VMVN_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VNEG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VNM__(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VORN_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VORR_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VORR_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VPADAL(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VPADD(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VPADD_FP(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VPADDL(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VPMAXMIN(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VPMAXMIN_FP(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VPOP(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VPUSH(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VQABS(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VQADD(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VQDML_L(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VQDMULH(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VQDMULL(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VQMOV_N(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VQNEG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VQRDMULH(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VQRSHL(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VQRSHR_N(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VQSHL_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VQSHL_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VQSHR_N(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VQSUB(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VRADDHN(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VRECPE(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VRECPS(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VREV__(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VRHADD(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VRSHL(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VRSHR(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VRSHRN(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VRSQRTE(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VRSQRTS(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VRSRA(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VRSUBHN(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VSHL_IMM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VSHL_REG(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VSHLL(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VSHR(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VSHRN(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VSLI(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VSQRT(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VSRA(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VSRI(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VST__MS(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VST1_SL(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VST2_SL(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VST3_SL(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VST4_SL(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VSTM(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VSTR(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VSUB(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VSUB_FP(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VSUBHN(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VSUB_(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VSWP(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VTB_(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VTRN(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VTST(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VUZP(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::VZIP(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}


template<arm_encoding type>
void arm_interpreter::WFE(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::WFI(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template<arm_encoding type>
void arm_interpreter::YIELD(ARMv7Thread& cpu, const u32 op, const u32 cond)
{
	fmt::throw_exception("TODO" HERE);
}

template void arm_interpreter::HACK<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::HACK<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ADC_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ADC_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ADC_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ADC_REG<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ADC_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ADC_RSR<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ADD_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ADD_IMM<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ADD_IMM<T3>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ADD_IMM<T4>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ADD_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ADD_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ADD_REG<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ADD_REG<T3>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ADD_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ADD_RSR<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ADD_SPI<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ADD_SPI<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ADD_SPI<T3>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ADD_SPI<T4>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ADD_SPI<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ADD_SPR<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ADD_SPR<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ADD_SPR<T3>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ADD_SPR<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ADR<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ADR<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ADR<T3>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ADR<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ADR<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::AND_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::AND_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::AND_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::AND_REG<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::AND_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::AND_RSR<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ASR_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ASR_IMM<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ASR_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ASR_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ASR_REG<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ASR_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::B<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::B<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::B<T3>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::B<T4>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::B<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::BFC<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::BFC<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::BFI<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::BFI<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::BIC_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::BIC_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::BIC_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::BIC_REG<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::BIC_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::BIC_RSR<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::BKPT<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::BKPT<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::BL<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::BL<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::BL<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::BL<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::BLX<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::BLX<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::BX<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::BX<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::CB_Z<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::CLZ<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::CLZ<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::CMN_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::CMN_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::CMN_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::CMN_REG<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::CMN_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::CMN_RSR<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::CMP_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::CMP_IMM<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::CMP_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::CMP_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::CMP_REG<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::CMP_REG<T3>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::CMP_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::CMP_RSR<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::DBG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::DBG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::DMB<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::DMB<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::DSB<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::DSB<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::EOR_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::EOR_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::EOR_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::EOR_REG<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::EOR_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::EOR_RSR<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::IT<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDM<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDMDA<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDMDB<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDMDB<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDMIB<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDR_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDR_IMM<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDR_IMM<T3>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDR_IMM<T4>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDR_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDR_LIT<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDR_LIT<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDR_LIT<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDR_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDR_REG<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDR_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRB_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRB_IMM<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRB_IMM<T3>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRB_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRB_LIT<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRB_LIT<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRB_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRB_REG<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRB_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRD_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRD_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRD_LIT<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRD_LIT<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRD_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDREX<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDREX<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDREXB<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDREXB<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDREXD<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDREXD<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDREXH<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDREXH<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRH_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRH_IMM<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRH_IMM<T3>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRH_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRH_LIT<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRH_LIT<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRH_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRH_REG<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRH_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRSB_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRSB_IMM<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRSB_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRSB_LIT<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRSB_LIT<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRSB_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRSB_REG<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRSB_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRSH_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRSH_IMM<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRSH_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRSH_LIT<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRSH_LIT<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRSH_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRSH_REG<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LDRSH_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LSL_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LSL_IMM<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LSL_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LSL_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LSL_REG<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LSL_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LSR_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LSR_IMM<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LSR_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LSR_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LSR_REG<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::LSR_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MLA<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MLA<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MLS<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MLS<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MOV_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MOV_IMM<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MOV_IMM<T3>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MOV_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MOV_IMM<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MOV_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MOV_REG<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MOV_REG<T3>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MOV_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MOVT<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MOVT<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MRC_<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MRC_<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MRC_<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MRC_<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MRS<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MRS<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MSR_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MSR_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MSR_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MUL<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MUL<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MUL<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MVN_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MVN_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MVN_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MVN_REG<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MVN_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::MVN_RSR<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::NOP<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::NOP<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::NOP<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ORN_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ORN_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ORR_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ORR_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ORR_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ORR_REG<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ORR_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ORR_RSR<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::PKH<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::PKH<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::POP<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::POP<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::POP<T3>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::POP<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::POP<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::PUSH<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::PUSH<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::PUSH<T3>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::PUSH<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::PUSH<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::QADD<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::QADD<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::QADD16<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::QADD16<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::QADD8<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::QADD8<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::QASX<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::QASX<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::QDADD<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::QDADD<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::QDSUB<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::QDSUB<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::QSAX<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::QSAX<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::QSUB<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::QSUB<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::QSUB16<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::QSUB16<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::QSUB8<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::QSUB8<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::RBIT<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::RBIT<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::REV<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::REV<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::REV<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::REV16<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::REV16<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::REV16<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::REVSH<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::REVSH<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::REVSH<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ROR_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ROR_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ROR_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ROR_REG<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::ROR_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::RRX<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::RRX<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::RSB_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::RSB_IMM<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::RSB_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::RSB_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::RSB_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::RSB_RSR<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::RSC_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::RSC_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::RSC_RSR<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SADD16<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SADD16<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SADD8<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SADD8<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SASX<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SASX<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SBC_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SBC_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SBC_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SBC_REG<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SBC_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SBC_RSR<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SBFX<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SBFX<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SDIV<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SEL<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SEL<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SHADD16<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SHADD16<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SHADD8<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SHADD8<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SHASX<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SHASX<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SHSAX<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SHSAX<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SHSUB16<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SHSUB16<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SHSUB8<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SHSUB8<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMLA__<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMLA__<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMLAD<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMLAD<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMLAL<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMLAL<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMLAL__<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMLAL__<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMLALD<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMLALD<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMLAW_<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMLAW_<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMLSD<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMLSD<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMLSLD<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMLSLD<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMMLA<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMMLA<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMMLS<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMMLS<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMMUL<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMMUL<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMUAD<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMUAD<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMUL__<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMUL__<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMULL<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMULL<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMULW_<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMULW_<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMUSD<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SMUSD<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SSAT<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SSAT<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SSAT16<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SSAT16<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SSAX<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SSAX<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SSUB16<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SSUB16<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SSUB8<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SSUB8<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STM<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STMDA<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STMDB<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STMDB<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STMIB<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STR_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STR_IMM<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STR_IMM<T3>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STR_IMM<T4>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STR_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STR_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STR_REG<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STR_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STRB_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STRB_IMM<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STRB_IMM<T3>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STRB_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STRB_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STRB_REG<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STRB_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STRD_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STRD_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STRD_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STREX<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STREX<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STREXB<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STREXB<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STREXD<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STREXD<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STREXH<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STREXH<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STRH_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STRH_IMM<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STRH_IMM<T3>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STRH_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STRH_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STRH_REG<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::STRH_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SUB_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SUB_IMM<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SUB_IMM<T3>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SUB_IMM<T4>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SUB_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SUB_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SUB_REG<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SUB_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SUB_RSR<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SUB_SPI<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SUB_SPI<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SUB_SPI<T3>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SUB_SPI<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SUB_SPR<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SUB_SPR<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SVC<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SVC<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SXTAB<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SXTAB<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SXTAB16<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SXTAB16<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SXTAH<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SXTAH<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SXTB<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SXTB<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SXTB<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SXTB16<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SXTB16<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SXTH<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SXTH<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::SXTH<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::TB_<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::TEQ_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::TEQ_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::TEQ_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::TEQ_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::TEQ_RSR<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::TST_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::TST_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::TST_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::TST_REG<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::TST_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::TST_RSR<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UADD16<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UADD16<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UADD8<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UADD8<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UASX<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UASX<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UBFX<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UBFX<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UDIV<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UHADD16<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UHADD16<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UHADD8<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UHADD8<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UHASX<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UHASX<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UHSAX<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UHSAX<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UHSUB16<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UHSUB16<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UHSUB8<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UHSUB8<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UMAAL<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UMAAL<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UMLAL<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UMLAL<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UMULL<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UMULL<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UQADD16<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UQADD16<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UQADD8<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UQADD8<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UQASX<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UQASX<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UQSAX<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UQSAX<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UQSUB16<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UQSUB16<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UQSUB8<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UQSUB8<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::USAD8<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::USAD8<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::USADA8<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::USADA8<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::USAT<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::USAT<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::USAT16<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::USAT16<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::USAX<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::USAX<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::USUB16<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::USUB16<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::USUB8<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::USUB8<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UXTAB<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UXTAB<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UXTAB16<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UXTAB16<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UXTAH<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UXTAH<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UXTB<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UXTB<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UXTB<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UXTB16<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UXTB16<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UXTH<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UXTH<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::UXTH<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VABA_<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VABA_<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VABA_<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VABA_<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VABD_<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VABD_<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VABD_<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VABD_<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VABD_FP<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VABD_FP<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VABS<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VABS<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VABS<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VABS<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VAC__<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VAC__<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VADD<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VADD<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VADD_FP<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VADD_FP<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VADD_FP<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VADD_FP<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VADDHN<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VADDHN<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VADD_<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VADD_<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VAND<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VAND<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VBIC_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VBIC_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VBIC_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VBIC_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VB__<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VB__<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCEQ_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCEQ_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCEQ_REG<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCEQ_REG<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCEQ_ZERO<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCEQ_ZERO<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCGE_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCGE_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCGE_REG<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCGE_REG<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCGE_ZERO<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCGE_ZERO<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCGT_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCGT_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCGT_REG<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCGT_REG<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCGT_ZERO<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCGT_ZERO<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCLE_ZERO<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCLE_ZERO<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCLS<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCLS<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCLT_ZERO<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCLT_ZERO<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCLZ<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCLZ<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCMP_<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCMP_<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCMP_<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCMP_<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCNT<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCNT<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCVT_FIA<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCVT_FIA<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCVT_FIF<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCVT_FIF<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCVT_FFA<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCVT_FFA<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCVT_FFF<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCVT_FFF<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCVT_DF<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCVT_DF<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCVT_HFA<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCVT_HFA<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCVT_HFF<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VCVT_HFF<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VDIV<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VDIV<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VDUP_S<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VDUP_S<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VDUP_R<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VDUP_R<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VEOR<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VEOR<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VEXT<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VEXT<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VHADDSUB<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VHADDSUB<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VLD__MS<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VLD__MS<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VLD1_SAL<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VLD1_SAL<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VLD1_SL<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VLD1_SL<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VLD2_SAL<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VLD2_SAL<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VLD2_SL<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VLD2_SL<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VLD3_SAL<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VLD3_SAL<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VLD3_SL<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VLD3_SL<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VLD4_SAL<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VLD4_SAL<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VLD4_SL<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VLD4_SL<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VLDM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VLDM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VLDM<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VLDM<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VLDR<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VLDR<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VLDR<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VLDR<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMAXMIN<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMAXMIN<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMAXMIN_FP<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMAXMIN_FP<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VML__<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VML__<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VML__<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VML__<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VML__FP<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VML__FP<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VML__FP<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VML__FP<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VML__S<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VML__S<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VML__S<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VML__S<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMOV_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMOV_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMOV_IMM<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMOV_IMM<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMOV_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMOV_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMOV_REG<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMOV_REG<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMOV_RS<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMOV_RS<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMOV_SR<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMOV_SR<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMOV_RF<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMOV_RF<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMOV_2RF<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMOV_2RF<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMOV_2RD<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMOV_2RD<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMOVL<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMOVL<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMOVN<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMOVN<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMRS<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMRS<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMSR<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMSR<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMUL_<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMUL_<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMUL_<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMUL_<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMUL_FP<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMUL_FP<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMUL_FP<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMUL_FP<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMUL_S<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMUL_S<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMUL_S<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMUL_S<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMVN_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMVN_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMVN_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VMVN_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VNEG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VNEG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VNEG<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VNEG<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VNM__<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VNM__<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VNM__<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VNM__<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VORN_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VORN_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VORR_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VORR_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VORR_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VORR_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VPADAL<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VPADAL<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VPADD<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VPADD<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VPADD_FP<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VPADD_FP<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VPADDL<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VPADDL<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VPMAXMIN<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VPMAXMIN<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VPMAXMIN_FP<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VPMAXMIN_FP<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VPOP<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VPOP<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VPOP<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VPOP<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VPUSH<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VPUSH<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VPUSH<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VPUSH<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQABS<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQABS<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQADD<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQADD<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQDML_L<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQDML_L<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQDML_L<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQDML_L<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQDMULH<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQDMULH<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQDMULH<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQDMULH<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQDMULL<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQDMULL<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQDMULL<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQDMULL<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQMOV_N<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQMOV_N<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQNEG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQNEG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQRDMULH<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQRDMULH<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQRDMULH<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQRDMULH<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQRSHL<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQRSHL<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQRSHR_N<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQRSHR_N<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQSHL_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQSHL_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQSHL_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQSHL_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQSHR_N<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQSHR_N<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQSUB<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VQSUB<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VRADDHN<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VRADDHN<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VRECPE<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VRECPE<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VRECPS<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VRECPS<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VREV__<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VREV__<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VRHADD<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VRHADD<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VRSHL<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VRSHL<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VRSHR<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VRSHR<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VRSHRN<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VRSHRN<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VRSQRTE<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VRSQRTE<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VRSQRTS<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VRSQRTS<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VRSRA<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VRSRA<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VRSUBHN<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VRSUBHN<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSHL_IMM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSHL_IMM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSHL_REG<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSHL_REG<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSHLL<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSHLL<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSHLL<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSHLL<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSHR<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSHR<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSHRN<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSHRN<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSLI<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSLI<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSQRT<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSQRT<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSRA<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSRA<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSRI<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSRI<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VST__MS<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VST__MS<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VST1_SL<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VST1_SL<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VST2_SL<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VST2_SL<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VST3_SL<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VST3_SL<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VST4_SL<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VST4_SL<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSTM<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSTM<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSTM<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSTM<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSTR<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSTR<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSTR<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSTR<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSUB<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSUB<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSUB_FP<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSUB_FP<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSUB_FP<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSUB_FP<A2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSUBHN<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSUBHN<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSUB_<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSUB_<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSWP<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VSWP<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VTB_<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VTB_<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VTRN<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VTRN<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VTST<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VTST<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VUZP<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VUZP<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VZIP<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::VZIP<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::WFE<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::WFE<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::WFE<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::WFI<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::WFI<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::WFI<A1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::YIELD<T1>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::YIELD<T2>(ARMv7Thread&, const u32, const u32);
template void arm_interpreter::YIELD<A1>(ARMv7Thread&, const u32, const u32);
