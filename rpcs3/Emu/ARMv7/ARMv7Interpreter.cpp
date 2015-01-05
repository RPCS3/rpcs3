#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Memory/Memory.h"
#include "Emu/CPU/CPUDecoder.h"
#include "ARMv7Thread.h"
#include "PSVFuncList.h"
#include "ARMv7Interpreter.h"

template<typename T>
u8 ARMv7_instrs::BitCount(T x, u8 len)
{
	u8 result = 0;

	for (u8 mask = 1 << (len - 1); mask; mask >>= 1)
	{
		if (x & mask) result++;
	}

	return result;
}

template<typename T>
s8 ARMv7_instrs::LowestSetBit(T x, u8 len)
{
	if (!x) return len;

	u8 result = 0;

	for (T mask = 1, i = 0; i<len && (x & mask) == 0; mask <<= 1, i++)
	{
		result++;
	}

	return result;
}

template<typename T>
s8 ARMv7_instrs::HighestSetBit(T x, u8 len)
{
	if (!x) return -1;

	u8 result = len;

	for (T mask = T(1) << (len - 1); (x & mask) == 0; mask >>= 1)
	{
		result--;
	}

	return result;
}

template<typename T>
s8 ARMv7_instrs::CountLeadingZeroBits(T x, u8 len)
{
	return len - 1 - HighestSetBit(x, len);
}

SRType ARMv7_instrs::DecodeImmShift(u32 type, u32 imm5, u32* shift_n)
{
	SRType shift_t = SRType_None;

	switch (type)
	{
	case 0: shift_t = SRType_LSL; if (shift_n) *shift_n = imm5; break;
	case 1: shift_t = SRType_LSR; if (shift_n) *shift_n = imm5 == 0 ? 32 : imm5; break;
	case 2: shift_t = SRType_ASR; if (shift_n) *shift_n = imm5 == 0 ? 32 : imm5; break;
	case 3:
		if (imm5 == 0)
		{
			shift_t = SRType_RRX; if (shift_n) *shift_n = 1;
		}
		else
		{
			shift_t = SRType_ROR; if (shift_n) *shift_n = imm5;
		}
		break;
	}

	return shift_t;
}

SRType ARMv7_instrs::DecodeRegShift(u8 type)
{
	SRType shift_t = SRType_None;

	switch (type)
	{
	case 0: shift_t = SRType_LSL; break;
	case 1: shift_t = SRType_LSR; break;
	case 2: shift_t = SRType_ASR; break;
	case 3: shift_t = SRType_ROR; break;
	}

	return shift_t;
}

u32 ARMv7_instrs::LSL_C(u32 x, s32 shift, bool& carry_out)
{
	assert(shift > 0);
	carry_out = shift <= 32 ? x & (1 << (32 - shift)) : false;
	return shift < 32 ? x << shift : 0;
}

u32 ARMv7_instrs::LSL(u32 x, s32 shift)
{
	assert(shift >= 0);
	return shift < 32 ? x << shift : 0;
}

u32 ARMv7_instrs::LSR_C(u32 x, s32 shift, bool& carry_out)
{
	assert(shift > 0);
	carry_out = shift <= 32 ? x & (1 << (shift - 1)) : false;
	return shift < 32 ? x >> shift : 0;
}

u32 ARMv7_instrs::LSR(u32 x, s32 shift)
{
	assert(shift >= 0);
	return shift < 32 ? x >> shift : 0;
}

s32 ARMv7_instrs::ASR_C(s32 x, s32 shift, bool& carry_out)
{
	assert(shift > 0);
	carry_out = shift <= 32 ? x & (1 << (shift - 1)) : false;
	return shift < 32 ? x >> shift : x >> 31;
}

s32 ARMv7_instrs::ASR(s32 x, s32 shift)
{
	assert(shift >= 0);
	return shift < 32 ? x >> shift : x >> 31;
}

u32 ARMv7_instrs::ROR_C(u32 x, s32 shift, bool& carry_out)
{
	assert(shift);
	carry_out = x & (1 << (shift - 1));
	return x >> shift | x << (32 - shift);
}

u32 ARMv7_instrs::ROR(u32 x, s32 shift)
{
	return x >> shift | x << (32 - shift);
}

u32 ARMv7_instrs::RRX_C(u32 x, bool carry_in, bool& carry_out)
{
	carry_out = x & 0x1;
	return ((u32)carry_in << 31) | (x >> 1);
}

u32 ARMv7_instrs::RRX(u32 x, bool carry_in)
{
	return ((u32)carry_in << 31) | (x >> 1);
}

template<typename T> T ARMv7_instrs::Shift_C(T value, SRType type, s32 amount, bool carry_in, bool& carry_out)
{
	assert(type != SRType_RRX || amount == 1);

	if (amount)
	{
		switch (type)
		{
		case SRType_LSL: return LSL_C(value, amount, carry_out);
		case SRType_LSR: return LSR_C(value, amount, carry_out);
		case SRType_ASR: return ASR_C(value, amount, carry_out);
		case SRType_ROR: return ROR_C(value, amount, carry_out);
		case SRType_RRX: return RRX_C(value, carry_in, carry_out);
		default: throw __FUNCTION__;
		}
	}

	carry_out = carry_in;
	return value;
}

template<typename T> T ARMv7_instrs::Shift(T value, SRType type, s32 amount, bool carry_in)
{
	bool carry_out;
	return Shift_C(value, type, amount, carry_in, carry_out);
}

template<typename T> T ARMv7_instrs::AddWithCarry(T x, T y, bool carry_in, bool& carry_out, bool& overflow)
{
	const T sign_mask = (T)1 << (sizeof(T) - 1);

	T result = x + y;
	carry_out = ((x & y) | ((x ^ y) & ~result)) & sign_mask;
	overflow = (x ^ result) & (y ^ result) & sign_mask;
	if (carry_in)
	{
		result += 1;
		carry_out ^= (result == 0);
		overflow ^= (result == sign_mask);
	}
	return result;
}

u32 ARMv7_instrs::ThumbExpandImm_C(u32 imm12, bool carry_in, bool& carry_out)
{
	if ((imm12 & 0xc00) >> 10)
	{
		u32 unrotated_value = (imm12 & 0x7f) | 0x80;

		return ROR_C(unrotated_value, (imm12 & 0xf80) >> 7, carry_out);
	}
	else
	{
		carry_out = carry_in;

		u32 imm8 = imm12 & 0xff;
		switch ((imm12 & 0x300) >> 8)
		{
		case 0: return imm8;
		case 1: return imm8 << 16 | imm8;
		case 2: return imm8 << 24 | imm8 << 8;
		default: return imm8 << 24 | imm8 << 16 | imm8 << 8 | imm8;
		}
	}
}

u32 ARMv7_instrs::ThumbExpandImm(ARMv7Thread* CPU, u32 imm12)
{
	bool carry = CPU->APSR.C;
	return ThumbExpandImm_C(imm12, carry, carry);
}

bool ARMv7_instrs::ConditionPassed(ARMv7Thread* CPU, u32 cond)
{
	bool result = false;

	switch (cond >> 1)
	{
	case 0: result = CPU->APSR.Z == 1; break;
	case 1: result = CPU->APSR.C == 1; break;
	case 2: result = CPU->APSR.N == 1; break;
	case 3: result = CPU->APSR.V == 1; break;
	case 4: result = CPU->APSR.C == 1 && CPU->APSR.Z == 0; break;
	case 5: result = CPU->APSR.N == CPU->APSR.V; break;
	case 6: result = CPU->APSR.N == CPU->APSR.V && CPU->APSR.Z == 0; break;
	case 7: return true;
	}

	if (cond & 0x1)
	{
		return !result;
	}

	return result;
}

// instructions
void ARMv7_instrs::UNK(ARMv7Thread* thr)
{
	LOG_ERROR(HLE, "Unknown/illegal opcode! (0x%04x : 0x%04x)", thr->code.data >> 16, thr->code.data & 0xffff);
	Emu.Pause();
}

void ARMv7_instrs::NULL_OP(ARMv7Thread* thr, const ARMv7_encoding type)
{
	LOG_ERROR(HLE, "Null opcode found: data = 0x%x", thr->m_arg);
	Emu.Pause();
}

void ARMv7_instrs::HACK(ARMv7Thread* thr, const ARMv7_encoding type)
{
	u32 cond = thr->ITSTATE.advance();
	u32 code = 0;

	switch (type)
	{
	case T1:
	{
		code = thr->code.data & 0xffff;
		break;
	}
	case A1:
	{
		cond = thr->code.data >> 28;
		code = (thr->code.data & 0xfff00) >> 4 | (thr->code.data & 0xf);
		break;
	}
	default: throw __FUNCTION__;
	}

	if (ConditionPassed(thr, cond))
	{
		execute_psv_func_by_index(*thr, code);
	}
}

void ARMv7_instrs::ADC_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::ADC_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::ADC_RSR(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::ADD_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	bool set_flags = !thr->ITSTATE;
	u32 cond = thr->ITSTATE.advance();
	u32 d = 0;
	u32 n = 0;
	u32 imm32 = 0;

	switch (type)
	{
	case T1:
	{
		d = (thr->code.data & 0x7);
		n = (thr->code.data & 0x38) >> 3;
		imm32 = (thr->code.data & 0x1c0) >> 6;
		break;
	}
	case T2:
	{
		d = n = (thr->code.data & 0x700) >> 8;
		imm32 = (thr->code.data & 0xff);
		break;
	}
	case T3:
	{
		d = (thr->code.data & 0xf00) >> 8;
		n = (thr->code.data & 0xf0000) >> 16;
		set_flags = (thr->code.data & 0x100000);
		imm32 = ThumbExpandImm(thr, (thr->code.data & 0x4000000) >> 15 | (thr->code.data & 0x7000) >> 4 | (thr->code.data & 0xff));

		if (d == 15 && set_flags)
		{
			throw "CMN (immediate)";
		}
		if (n == 13)
		{
			throw "ADD (SP plus immediate)";
		}
		break;
	}
	case T4:
	{
		d = (thr->code.data & 0xf00) >> 8;
		n = (thr->code.data & 0xf0000) >> 16;
		set_flags = false;
		imm32 = (thr->code.data & 0x4000000) >> 15 | (thr->code.data & 0x7000) >> 4 | (thr->code.data & 0xff);

		if (n == 15)
		{
			throw "ADR";
		}
		if (n == 13)
		{
			throw "ADD (SP plus immediate)";
		}
		break;
	}
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}

	if (ConditionPassed(thr, cond))
	{
		if (set_flags)
		{
			bool carry, overflow;
			const u32 res = AddWithCarry(thr->read_gpr(n), imm32, false, carry, overflow);
			thr->write_gpr(d, res);
			thr->APSR.N = res >> 31;
			thr->APSR.Z = res == 0;
			thr->APSR.C = carry;
			thr->APSR.V = overflow;
		}
		else
		{
			thr->write_gpr(d, thr->read_gpr(n) + imm32);
		}
	}
}

void ARMv7_instrs::ADD_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	bool set_flags = !thr->ITSTATE;
	u32 cond = thr->ITSTATE.advance();
	u32 d = 0;
	u32 n = 0;
	u32 m = 0;
	auto shift_t = SRType_LSL;
	u32 shift_n = 0;

	switch (type)
	{
	case T1:
	{
		d = (thr->code.data & 0x7);
		n = (thr->code.data & 0x38) >> 3;
		m = (thr->code.data & 0x1c0) >> 6;
		break;
	}
	case T2:
	{
		n = d = (thr->code.data & 0x80) >> 4 | (thr->code.data & 0x7);
		m = (thr->code.data & 0x78) >> 3;
		set_flags = false;

		if (n == 13 || m == 13)
		{
			throw "ADD (SP plus register)";
		}
		break;
	}
	case T3:
	{
		d = (thr->code.data & 0xf00) >> 8;
		n = (thr->code.data & 0xf0000) >> 16;
		m = (thr->code.data & 0xf);
		set_flags = (thr->code.data & 0x100000);
		shift_t = DecodeImmShift((thr->code.data & 0x30) >> 4, (thr->code.data & 0x7000) >> 10 | (thr->code.data & 0xc0) >> 6, &shift_n);

		if (d == 15 && set_flags)
		{
			throw "CMN (register)";
		}
		if (n == 13)
		{
			throw "ADD (SP plus register)";
		}
		break;
	}
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}

	if (ConditionPassed(thr, cond))
	{
		const u32 shifted = Shift(thr->read_gpr(m), shift_t, shift_n, true);
		if (set_flags)
		{
			bool carry, overflow;
			const u32 res = AddWithCarry(thr->read_gpr(n), shifted, false, carry, overflow);
			thr->write_gpr(d, res);
			thr->APSR.N = res >> 31;
			thr->APSR.Z = res == 0;
			thr->APSR.C = carry;
			thr->APSR.V = overflow;
		}
		else
		{
			thr->write_gpr(d, thr->read_gpr(n) + shifted);
		}
	}
}

void ARMv7_instrs::ADD_RSR(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::ADD_SPI(ARMv7Thread* thr, const ARMv7_encoding type)
{
	u32 cond = thr->ITSTATE.advance();
	u32 d = 13;
	bool set_flags = false;
	u32 imm32 = 0;

	switch (type)
	{
	case T1:
	{
		d = (thr->code.data & 0x700) >> 8;
		imm32 = (thr->code.data & 0xff) << 2;
		break;
	}
	case T2:
	{
		imm32 = (thr->code.data & 0x7f) << 2;
		break;
	}
	case T3:
	{
		d = (thr->code.data & 0xf00) >> 8;
		set_flags = (thr->code.data & 0x100000);
		imm32 = ThumbExpandImm(thr, (thr->code.data & 0x4000000) >> 15 | (thr->code.data & 0x7000) >> 4 | (thr->code.data & 0xff));

		if (d == 15 && set_flags)
		{
			throw "CMN (immediate)";
		}
		break;
	}
	case T4:
	{
		d = (thr->code.data & 0xf00) >> 8;
		set_flags = false;
		imm32 = (thr->code.data & 0x4000000) >> 15 | (thr->code.data & 0x7000) >> 4 | (thr->code.data & 0xff);
		break;
	}
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}

	if (ConditionPassed(thr, cond))
	{
		if (set_flags)
		{
			bool carry, overflow;
			const u32 res = AddWithCarry(thr->SP, imm32, false, carry, overflow);
			thr->write_gpr(d, res);
			thr->APSR.N = res >> 31;
			thr->APSR.Z = res == 0;
			thr->APSR.C = carry;
			thr->APSR.V = overflow;
		}
		else
		{
			thr->write_gpr(d, thr->SP + imm32);
		}
	}
}

void ARMv7_instrs::ADD_SPR(ARMv7Thread* thr, const ARMv7_encoding type)
{
	u32 cond = thr->ITSTATE.advance();
	u32 d = 13;
	u32 m = 0;
	bool set_flags = false;
	auto shift_t = SRType_LSL;
	u32 shift_n = 0;

	switch (type)
	{
	case T1:
	{
		m = d = (thr->code.data & 0x80) >> 4 | (thr->code.data & 0x7);
		break;
	}
	case T2:
	{
		m = (thr->code.data & 0x78) >> 3;

		if (m == 13)
		{
			throw "ADD_SPR_T2: T1";
		}
		break;
	}
	case T3:
	{
		d = (thr->code.data & 0xf00) >> 8;
		m = (thr->code.data & 0xf);
		set_flags = (thr->code.data & 0x100000);
		shift_t = DecodeImmShift((thr->code.data & 0x30) >> 4, (thr->code.data & 0x7000) >> 10 | (thr->code.data & 0xc0) >> 6, &shift_n);
		break;
	}
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}

	if (ConditionPassed(thr, cond))
	{
		const u32 shifted = Shift(thr->read_gpr(m), shift_t, shift_n, thr->APSR.C);
		if (set_flags)
		{
			bool carry, overflow;
			const u32 res = AddWithCarry(thr->SP, shifted, false, carry, overflow);
			thr->write_gpr(d, res);
			thr->APSR.N = res >> 31;
			thr->APSR.Z = res == 0;
			thr->APSR.C = carry;
			thr->APSR.V = overflow;
		}
		else
		{
			thr->write_gpr(d, thr->SP + thr->read_gpr(m));
		}
	}
}


void ARMv7_instrs::ADR(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::AND_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::AND_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::AND_RSR(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::ASR_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::ASR_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::B(ARMv7Thread* thr, const ARMv7_encoding type)
{
	u32 cond = thr->ITSTATE.advance();
	u32 jump = 0; // jump = instr_size + imm32 ???

	switch (type)
	{
	case T1:
	{
		cond = (thr->code.data >> 8) & 0xf;
		if (cond == 0xf)
		{
			throw "SVC";
		}

		jump = 4 + sign<9, u32>((thr->code.data & 0xff) << 1);
		break;
	}
	case T2:
	{
		jump = 4 + sign<12, u32>((thr->code.data & 0x7ff) << 1);
		break;
	}
	case T3:
	{
		cond = (thr->code.data >> 6) & 0xf;
		if (cond >= 0xe)
		{
			throw "B_T3: Related encodings";
		}

		u32 s = (thr->code.data >> 26) & 0x1;
		u32 j1 = (thr->code.data >> 13) & 0x1;
		u32 j2 = (thr->code.data >> 11) & 0x1;
		jump = 4 + sign<21, u32>(s << 20 | j2 << 19 | j1 << 18 | (thr->code.data & 0x3f0000) >> 4 | (thr->code.data & 0x7ff) << 1);
		break;
	}
	case T4:
	{
		u32 s = (thr->code.data >> 26) & 0x1;
		u32 i1 = (thr->code.data >> 13) & 0x1 ^ s ^ 1;
		u32 i2 = (thr->code.data >> 11) & 0x1 ^ s ^ 1;
		jump = 4 + sign<25, u32>(s << 24 | i2 << 23 | i1 << 22 | (thr->code.data & 0x3ff0000) >> 4 | (thr->code.data & 0x7ff) << 1);
		break;
	}
	case A1:
	{
		cond = thr->code.data >> 28;
		jump = 1 + 4 + sign<26, u32>((thr->code.data & 0xffffff) << 2);
		break;
	}
	default: throw __FUNCTION__;
	}

	if (ConditionPassed(thr, cond))
	{
		thr->SetBranch(thr->PC + jump);
	}
}


void ARMv7_instrs::BFC(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::BFI(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::BIC_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::BIC_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::BIC_RSR(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::BKPT(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::BL(ARMv7Thread* thr, const ARMv7_encoding type)
{
	u32 cond = thr->ITSTATE.advance();
	u32 newLR = thr->PC;
	u32 imm32 = 0;

	switch (type)
	{
	case T1:
	{
		u32 s = (thr->code.data >> 26) & 0x1;
		u32 i1 = (thr->code.data >> 13) & 0x1 ^ s ^ 1;
		u32 i2 = (thr->code.data >> 11) & 0x1 ^ s ^ 1;
		imm32 = 4 + sign<25, u32>(s << 24 | i2 << 23 | i1 << 22 | (thr->code.data & 0x3ff0000) >> 4 | (thr->code.data & 0x7ff) << 1);
		newLR = (thr->PC + 4) | 1;
		break;
	}
	case A1:
	{
		cond = thr->code.data >> 28;
		imm32 = 4 + sign<26, u32>((thr->code.data & 0xffffff) << 2);
		newLR = (thr->PC + 4) - 4;
		break;
	}
	default: throw __FUNCTION__;
	}

	if (ConditionPassed(thr, cond))
	{
		thr->LR = newLR;
		thr->SetBranch(thr->PC + imm32);
	}
}

void ARMv7_instrs::BLX(ARMv7Thread* thr, const ARMv7_encoding type)
{
	u32 cond = thr->ITSTATE.advance();
	u32 newLR = thr->PC;
	u32 target = 0;

	switch (type)
	{
	case T1:
	{
		target = thr->read_gpr((thr->code.data >> 3) & 0xf);
		newLR = (thr->PC + 2) | 1; // ???
		break;
	}
	case T2:
	{
		u32 s = (thr->code.data >> 26) & 0x1;
		u32 i1 = (thr->code.data >> 13) & 0x1 ^ s ^ 1;
		u32 i2 = (thr->code.data >> 11) & 0x1 ^ s ^ 1;
		target = (thr->PC + 4 & ~3) + sign<25, u32>(s << 24 | i2 << 23 | i1 << 22 | (thr->code.data & 0x3ff0000) >> 4 | (thr->code.data & 0x7ff) << 1);
		newLR = (thr->PC + 4) | 1;
		break;
	}
	case A1:
	{
		cond = thr->code.data >> 28;
		target = thr->read_gpr(thr->code.data & 0xf);
		newLR = (thr->PC + 4) - 4;
		break;
	}
	case A2:
	{
		target = (thr->PC + 4 | 1) + sign<25, u32>((thr->code.data & 0xffffff) << 2 | (thr->code.data & 0x1000000) >> 23);
		newLR = (thr->PC + 4) - 4;
		break;
	}
	default: throw __FUNCTION__;
	}

	if (ConditionPassed(thr, cond))
	{
		thr->LR = newLR;
		if (target & 1)
		{
			thr->ISET = Thumb;
			thr->SetBranch(target & ~1);
		}
		else
		{
			thr->ISET = ARM;
			thr->SetBranch(target);
		}
	}
}

void ARMv7_instrs::BX(ARMv7Thread* thr, const ARMv7_encoding type)
{
	u32 cond = thr->ITSTATE.advance();
	u32 target = 0;

	switch (type)
	{
	case T1:
	{
		target = thr->read_gpr((thr->code.data >> 3) & 0xf);
		break;
	}
	case A1:
	{
		cond = thr->code.data >> 28;
		target = thr->read_gpr(thr->code.data & 0xf);
	}
	default: throw __FUNCTION__;
	}

	if (ConditionPassed(thr, cond))
	{
		if (target & 1)
		{
			thr->ISET = Thumb;
			thr->SetBranch(target & ~1);
		}
		else
		{
			thr->ISET = ARM;
			thr->SetBranch(target);
		}
	}
}


void ARMv7_instrs::CB_Z(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case T1: break;
	default: throw __FUNCTION__;
	}

	if ((thr->read_gpr(thr->code.data & 0x7) == 0) ^ ((thr->code.data & 0x800) != 0))
	{
		thr->SetBranch(thr->PC + 2 + ((thr->code.data & 0xf8) >> 2) + ((thr->code.data & 0x200) >> 3));
	}
}


void ARMv7_instrs::CLZ(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::CMN_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::CMN_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::CMN_RSR(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::CMP_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	u32 cond = thr->ITSTATE.advance();
	u32 n = 0;
	u32 imm32 = 0;

	switch (type)
	{
	case T1:
	{
		n = (thr->code.data & 0x700) >> 8;
		imm32 = (thr->code.data & 0xff);
		break;
	}
	case T2:
	{
		n = (thr->code.data & 0xf0000) >> 16;
		imm32 = ThumbExpandImm(thr, (thr->code.data & 0x4000000) >> 15 | (thr->code.data & 0x7000) >> 4 | (thr->code.data & 0xff));
		break;
	}
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}

	if (ConditionPassed(thr, cond))
	{
		bool carry, overflow;
		const u32 res = AddWithCarry(thr->read_gpr(n), ~imm32, true, carry, overflow);
		thr->APSR.N = res >> 31;
		thr->APSR.Z = res == 0;
		thr->APSR.C = carry;
		thr->APSR.V = overflow;
	}
}

void ARMv7_instrs::CMP_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	u32 cond = thr->ITSTATE.advance();
	u32 n = 0;
	u32 m = 0;
	auto shift_t = SRType_LSL;
	u32 shift_n = 0;

	switch (type)
	{
	case T1:
	{
		n = (thr->code.data & 0x7);
		m = (thr->code.data & 0x38) >> 3;
		break;
	}
	case T2:
	{
		n = (thr->code.data & 0x80) >> 4 | (thr->code.data & 0x7);
		m = (thr->code.data & 0x78) >> 3;
		break;
	}
	case T3:
	{
		n = (thr->code.data & 0xf0000) >> 16;
		m = (thr->code.data & 0xf);
		shift_t = DecodeImmShift((thr->code.data & 0x30) >> 4, (thr->code.data & 0x7000) >> 10 | (thr->code.data & 0xc0) >> 6, &shift_n);
		break;
	}
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}

	if (ConditionPassed(thr, cond))
	{
		bool carry, overflow;
		const u32 shifted = Shift(thr->read_gpr(m), shift_t, shift_n, true);
		const u32 res = AddWithCarry(thr->read_gpr(n), ~shifted, true, carry, overflow);
		thr->APSR.N = res >> 31;
		thr->APSR.Z = res == 0;
		thr->APSR.C = carry;
		thr->APSR.V = overflow;
	}
}

void ARMv7_instrs::CMP_RSR(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::EOR_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::EOR_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::EOR_RSR(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::IT(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case T1:
	{
		if ((thr->code.data & 0xf) == 0)
		{
			throw "IT_T1: Related encodings";
		}

		thr->ITSTATE.IT = thr->code.data & 0xff;
		return;
	}
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::LDM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::LDMDA(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::LDMDB(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::LDMIB(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::LDR_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	u32 cond = thr->ITSTATE.advance();
	u32 t = 0;
	u32 n = 13;
	u32 imm32 = 0;
	bool index = true;
	bool add = true;
	bool wback = false;

	switch (type)
	{
	case T1:
	{
		t = (thr->code.data & 0x7);
		n = (thr->code.data & 0x38) >> 3;
		imm32 = (thr->code.data & 0x7c0) >> 4;
		break;
	}
	case T2:
	{
		t = (thr->code.data & 0x700) >> 8;
		imm32 = (thr->code.data & 0xff) << 2;
		break;
	}
	case T3:
	{
		t = (thr->code.data & 0xf000) >> 12;
		n = (thr->code.data & 0xf0000) >> 16;
		imm32 = (thr->code.data & 0xfff);

		if (n == 15)
		{
			throw "LDR (literal)";
		}
		break;
	}
	case T4:
	{
		t = (thr->code.data & 0xf000) >> 12;
		n = (thr->code.data & 0xf0000) >> 16;
		imm32 = (thr->code.data & 0xff);
		index = (thr->code.data & 0x400);
		add = (thr->code.data & 0x200);
		wback = (thr->code.data & 0x100);

		if (n == 15)
		{
			throw "LDR (literal)";
		}
		if (index && add && !wback)
		{
			throw "LDRT";
		}
		if (n == 13 && !index && add && wback && imm32 == 4)
		{
			throw "POP";
		}
		if (!index && !wback)
		{
			throw "LDR_IMM_T4: undefined";
		}
		break;
	}
	default: throw __FUNCTION__;
	}

	if (ConditionPassed(thr, cond))
	{
		const u32 offset_addr = add ? thr->read_gpr(n) + imm32 : thr->read_gpr(n) - imm32;
		const u32 addr = index ? offset_addr : thr->read_gpr(n);

		if (wback)
		{
			thr->write_gpr(n, offset_addr);
		}

		thr->write_gpr(t, vm::psv::read32(addr));
	}
}

void ARMv7_instrs::LDR_LIT(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::LDR_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::LDRB_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::LDRB_LIT(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::LDRB_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::LDRD_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::LDRD_LIT(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::LDRD_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::LDRH_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::LDRH_LIT(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::LDRH_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::LDRSB_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::LDRSB_LIT(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::LDRSB_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::LDRSH_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::LDRSH_LIT(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::LDRSH_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::LSL_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	bool set_flags = !thr->ITSTATE;
	u32 cond = thr->ITSTATE.advance();
	u32 d = 0;
	u32 m = 0;
	u32 shift_n = 0;

	switch (type)
	{
	case T1:
	{
		d = (thr->code.data & 0x7);
		m = (thr->code.data & 0x38) >> 3;
		shift_n = (thr->code.data & 0x7c0) >> 6;

		if (!shift_n)
		{
			throw "MOV (register)";
		}
		break;
	}
	case T2:
	{
		d = (thr->code.data & 0xf00) >> 8;
		m = (thr->code.data & 0xf);
		set_flags = (thr->code.data & 0x100000);
		shift_n = (thr->code.data & 0x7000) >> 10 | (thr->code.data & 0xc0) >> 6;

		if (!shift_n)
		{
			throw "MOV (register)";
		}
		break;
	}
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}

	if (ConditionPassed(thr, cond))
	{
		bool carry;
		const u32 res = Shift_C<u32>(thr->read_gpr(m), SRType_LSL, shift_n, thr->APSR.C, carry);
		thr->write_gpr(d, res);
		if (set_flags)
		{
			thr->APSR.N = res >> 31;
			thr->APSR.Z = res == 0;
			thr->APSR.C = carry;
		}
	}
}

void ARMv7_instrs::LSL_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	bool set_flags = !thr->ITSTATE;
	u32 cond = thr->ITSTATE.advance();
	u32 d = 0;
	u32 n = 0;
	u32 m = 0;

	switch (type)
	{
	case T1:
	{
		d = n = (thr->code.data & 0x7);
		m = (thr->code.data & 0x38) >> 3;
		break;
	}
	case T2:
	{
		d = (thr->code.data & 0xf00) >> 8;
		n = (thr->code.data & 0xf0000) >> 16;
		m = (thr->code.data & 0xf);
		set_flags = (thr->code.data & 0x100000);
		break;
	}
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}

	if (ConditionPassed(thr, cond))
	{
		bool carry;
		const u32 res = Shift_C(thr->read_gpr(n), SRType_LSL, (thr->read_gpr(m) & 0xff), thr->APSR.C, carry);
		thr->write_gpr(d, res);
		if (set_flags)
		{
			thr->APSR.N = res >> 31;
			thr->APSR.Z = res == 0;
			thr->APSR.C = carry;
		}
	}
}


void ARMv7_instrs::LSR_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::LSR_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::MLA(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::MLS(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::MOV_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	bool set_flags = !thr->ITSTATE;
	bool carry = thr->APSR.C;
	u32 cond = thr->ITSTATE.advance();
	u32 d = 0;
	u32 imm32 = 0;

	switch (type)
	{
	case T1:
	{
		d = (thr->code.data >> 8) & 0x7;
		imm32 = sign<8, u32>(thr->code.data & 0xff);
		break;
	}
	case T2:
	{
		set_flags = thr->code.data & 0x100000;
		d = (thr->code.data >> 8) & 0xf;
		imm32 = ThumbExpandImm_C((thr->code.data & 0x4000000) >> 15 | (thr->code.data & 0x7000) >> 4 | (thr->code.data & 0xff), carry, carry);
		break;
	}
	case T3:
	{
		set_flags = false;
		d = (thr->code.data >> 8) & 0xf;
		imm32 = (thr->code.data & 0xf0000) >> 4 | (thr->code.data & 0x4000000) >> 15 | (thr->code.data & 0x7000) >> 4 | (thr->code.data & 0xff);
		break;
	}
	default: throw __FUNCTION__;
	}

	if (ConditionPassed(thr, cond))
	{
		thr->write_gpr(d, imm32);
		if (set_flags)
		{
			thr->APSR.N = imm32 >> 31;
			thr->APSR.Z = imm32 == 0;
			thr->APSR.C = carry;
		}
	}
}

void ARMv7_instrs::MOV_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	u32 cond = thr->ITSTATE.advance();
	u32 d = 0;
	u32 m = 0;
	bool set_flags = false;

	switch (type)
	{
	case T1:
	{
		d = (thr->code.data & 0x80) >> 4 | (thr->code.data & 0x7);
		m = (thr->code.data & 0x78) >> 3;
		break;
	}
	case T2:
	{
		d = (thr->code.data & 0x7);
		m = (thr->code.data & 0x38) >> 3;
		set_flags = true;
		break;
	}
	case T3:
	{
		d = (thr->code.data & 0xf00) >> 8;
		m = (thr->code.data & 0xf);
		set_flags = (thr->code.data & 0x100000);
		break;
	}
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}

	if (ConditionPassed(thr, cond))
	{
		const u32 res = thr->read_gpr(m);
		thr->write_gpr(d, res);
		if (set_flags)
		{
			thr->APSR.N = res >> 31;
			thr->APSR.Z = res == 0;
			//thr->APSR.C = ?
		}
	}
}

void ARMv7_instrs::MOVT(ARMv7Thread* thr, const ARMv7_encoding type)
{
	u32 cond = thr->ITSTATE.advance();
	u32 d = 0;
	u32 imm16 = 0;

	switch (type)
	{
	case T1:
	{
		d = (thr->code.data & 0xf00) >> 8;
		imm16 = (thr->code.data & 0xf0000) >> 4 | (thr->code.data & 0x4000000) >> 14 | (thr->code.data & 0x7000) >> 4 | (thr->code.data & 0xff);
		break;
	}
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}

	if (ConditionPassed(thr, cond))
	{
		thr->write_gpr(d, (thr->read_gpr(d) & 0xffff) | (imm16 << 16));
	}
}


void ARMv7_instrs::MRS(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::MSR_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::MSR_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::MUL(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::MVN_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::MVN_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::MVN_RSR(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::NOP(ARMv7Thread* thr, const ARMv7_encoding type)
{
	u32 cond = thr->ITSTATE.advance();

	switch (type)
	{
	case T1:
	{
		break;
	}
	case T2:
	{
		break;
	}
	case A1:
	{
		cond = thr->code.data >> 28;
		break;
	}
	default: throw __FUNCTION__;
	}

	if (ConditionPassed(thr, cond))
	{
	}
}


void ARMv7_instrs::ORN_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::ORN_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::ORR_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::ORR_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::ORR_RSR(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::PKH(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::POP(ARMv7Thread* thr, const ARMv7_encoding type)
{
	u32 cond = thr->ITSTATE.advance();
	u16 reg_list = 0;

	switch (type)
	{
	case T1:
	{
		reg_list = ((thr->code.data & 0x100) << 7) | (thr->code.data & 0xff);
		break;
	}
	case T2:
	{
		reg_list = thr->code.data & 0xdfff;
		break;
	}
	case T3:
	{
		reg_list = 1 << (thr->code.data >> 12);
		break;
	}
	case A1:
	{
		cond = thr->code.data >> 28;
		reg_list = thr->code.data & 0xffff;
		if (BitCount(reg_list) < 2)
		{
			throw "LDM / LDMIA / LDMFD";
		}
		break;
	}
	case A2:
	{
		cond = thr->code.data >> 28;
		reg_list = 1 << ((thr->code.data >> 12) & 0xf);
		break;
	}
	default: throw __FUNCTION__;
	}

	if (ConditionPassed(thr, cond))
	{
		for (u16 mask = 1, i = 0; mask; mask <<= 1, i++)
		{
			if (reg_list & mask)
			{
				thr->write_gpr(i, vm::psv::read32(thr->SP));
				thr->SP += 4;
			}
		}
	}
}

void ARMv7_instrs::PUSH(ARMv7Thread* thr, const ARMv7_encoding type)
{
	u32 cond = thr->ITSTATE.advance();
	u16 reg_list = 0;

	switch (type)
	{
	case T1:
	{
		reg_list = ((thr->code.data & 0x100) << 6) | (thr->code.data & 0xff);
		break;
	}
	case T2:
	{
		reg_list = thr->code.data & 0x5fff;
		break;
	}
	case T3:
	{
		reg_list = 1 << (thr->code.data >> 12);
		break;
	}
	case A1:
	{
		cond = thr->code.data >> 28;
		reg_list = thr->code.data & 0xffff;
		if (BitCount(reg_list) < 2)
		{
			throw "STMDB / STMFD";
		}
		break;
	}
	case A2:
	{
		cond = thr->code.data >> 28;
		reg_list = 1 << ((thr->code.data >> 12) & 0xf);
		break;
	}
	default: throw __FUNCTION__;
	}

	if (ConditionPassed(thr, cond))
	{
		for (u16 mask = 1 << 15, i = 15; mask; mask >>= 1, i--)
		{
			if (reg_list & mask)
			{
				thr->SP -= 4;
				vm::psv::write32(thr->SP, thr->read_gpr(i));
			}
		}
	}
}


void ARMv7_instrs::QADD(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::QADD16(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::QADD8(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::QASX(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::QDADD(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::QDSUB(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::QSAX(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::QSUB(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::QSUB16(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::QSUB8(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::RBIT(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::REV(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::REV16(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::REVSH(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::ROR_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::ROR_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::RRX(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::RSB_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::RSB_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::RSB_RSR(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::RSC_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::RSC_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::RSC_RSR(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::SADD16(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SADD8(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SASX(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::SBC_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SBC_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SBC_RSR(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::SBFX(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::SDIV(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::SEL(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::SHADD16(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SHADD8(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SHASX(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SHSAX(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SHSUB16(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SHSUB8(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::SMLA__(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SMLAD(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SMLAL(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SMLAL__(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SMLALD(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SMLAW_(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SMLSD(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SMLSLD(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SMMLA(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SMMLS(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SMMUL(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SMUAD(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SMUL__(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SMULL(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SMULW_(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SMUSD(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::SSAT(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SSAT16(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SSAX(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SSUB16(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SSUB8(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::STM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::STMDA(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::STMDB(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::STMIB(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::STR_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	u32 cond = thr->ITSTATE.advance();
	u32 t = 16;
	u32 n = 13;
	u32 imm32 = 0;
	bool index = true;
	bool add = true;
	bool wback = false;

	switch (type)
	{
	case T1:
	{
		t = (thr->code.data & 0x7);
		n = (thr->code.data & 0x38) >> 3;
		imm32 = (thr->code.data & 0x7c0) >> 4;
		break;
	}
	case T2:
	{
		t = (thr->code.data & 0x700) >> 8;
		imm32 = (thr->code.data & 0xff) << 2;
		break;
	}
	case T3:
	{
		t = (thr->code.data & 0xf000) >> 12;
		n = (thr->code.data & 0xf0000) >> 16;
		imm32 = (thr->code.data & 0xfff);

		if (n == 0xf)
		{
			throw "STR_IMM_T3: undefined";
		}
		break;
	}
	case T4:
	{
		t = (thr->code.data & 0xf000) >> 12;
		n = (thr->code.data & 0xf0000) >> 16;
		imm32 = (thr->code.data & 0xff);
		index = (thr->code.data & 0x400);
		add = (thr->code.data & 0x200);
		wback = (thr->code.data & 0x100);

		if (index && add && !wback)
		{
			throw "STRT";
		}
		if (n == 13 && index && !add && wback && imm32 == 4)
		{
			throw "PUSH";
		}
		if (n == 15 || (!index && !wback))
		{
			throw "STR_IMM_T4: undefined";
		}
		break;
	}
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}

	if (ConditionPassed(thr, cond))
	{
		const u32 offset_addr = add ? thr->read_gpr(n) + imm32 : thr->read_gpr(n) - imm32;
		const u32 addr = index ? offset_addr : thr->read_gpr(n);

		vm::psv::write32(addr, thr->read_gpr(t));

		if (wback)
		{
			thr->write_gpr(n, offset_addr);
		}
	}
}

void ARMv7_instrs::STR_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	u32 cond = thr->ITSTATE.advance();
	u32 t = 0;
	u32 n = 0;
	u32 m = 0;
	bool index = true;
	bool add = true;
	bool wback = false;
	auto shift_t = SRType_LSL;
	u32 shift_n = 0;

	switch (type)
	{
	case T1:
	{
		t = (thr->code.data & 0x7);
		n = (thr->code.data & 0x38) >> 3;
		m = (thr->code.data & 0x1c0) >> 6;
		break;
	}
	case T2:
	{
		t = (thr->code.data & 0xf000) >> 12;
		n = (thr->code.data & 0xf0000) >> 16;
		m = (thr->code.data & 0xf);
		shift_n = (thr->code.data & 0x30) >> 4;

		if (n == 15)
		{
			throw "STR_REG_T2: undefined";
		}
		break;
	}
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}

	if (ConditionPassed(thr, cond))
	{
		const u32 offset = Shift(thr->read_gpr(m), shift_t, shift_n, thr->APSR.C);
		const u32 offset_addr = add ? thr->read_gpr(n) + offset : thr->read_gpr(n) - offset;
		const u32 addr = index ? offset_addr : thr->read_gpr(n);

		vm::psv::write32(addr, thr->read_gpr(t));

		if (wback)
		{
			thr->write_gpr(n, offset_addr);
		}
	}
}


void ARMv7_instrs::STRB_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::STRB_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::STRD_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::STRD_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::STRH_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::STRH_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::SUB_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	bool set_flags = !thr->ITSTATE;
	u32 cond = thr->ITSTATE.advance();
	u32 d = 0;
	u32 n = 0;
	u32 imm32 = 0;

	switch (type)
	{
	case T1:
	{
		d = (thr->code.data & 0x7);
		n = (thr->code.data & 0x38) >> 3;
		imm32 = (thr->code.data & 0x1c) >> 6;
		break;
	}
	case T2:
	{
		d = n = (thr->code.data & 0x700) >> 8;
		imm32 = (thr->code.data & 0xff);
		break;
	}
	case T3:
	{
		d = (thr->code.data & 0xf00) >> 8;
		n = (thr->code.data & 0xf0000) >> 16;
		set_flags = (thr->code.data & 0x100000);
		imm32 = ThumbExpandImm(thr, (thr->code.data & 0x4000000) >> 15 | (thr->code.data & 0x7000) >> 4 | (thr->code.data & 0xff));

		if (d == 15 && set_flags)
		{
			throw "CMP (immediate)";
		}
		if (n == 13)
		{
			throw "SUB (SP minus immediate)";
		}
		break;
	}
	case T4:
	{
		d = (thr->code.data & 0xf00) >> 8;
		n = (thr->code.data & 0xf0000) >> 16;
		set_flags = false;
		imm32 = (thr->code.data & 0x4000000) >> 15 | (thr->code.data & 0x7000) >> 4 | (thr->code.data & 0xff);

		if (d == 15)
		{
			throw "ADR";
		}
		if (n == 13)
		{
			throw "SUB (SP minus immediate)";
		}
		break;
	}
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}

	if (ConditionPassed(thr, cond))
	{
		if (set_flags)
		{
			bool carry, overflow;
			const u32 res = AddWithCarry(thr->read_gpr(n), ~imm32, true, carry, overflow);
			thr->write_gpr(d, res);
			thr->APSR.N = res >> 31;
			thr->APSR.Z = res == 0;
			thr->APSR.C = carry;
			thr->APSR.V = overflow;
		}
		else
		{
			thr->write_gpr(d, thr->read_gpr(n) - imm32);
		}
	}
}

void ARMv7_instrs::SUB_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	bool set_flags = !thr->ITSTATE;
	u32 cond = thr->ITSTATE.advance();
	u32 d = 0;
	u32 n = 0;
	u32 m = 0;
	auto shift_t = SRType_LSL;
	u32 shift_n = 0;

	switch (type)
	{
	case T1:
	{
		d = (thr->code.data & 0x7);
		n = (thr->code.data & 0x38) >> 3;
		m = (thr->code.data & 0x1c0) >> 6;
		break;
	}
	case T2:
	{
		d = (thr->code.data & 0xf00) >> 8;
		n = (thr->code.data & 0xf0000) >> 16;
		m = (thr->code.data & 0xf);
		set_flags = (thr->code.data & 0x100000);
		shift_t = DecodeImmShift((thr->code.data & 0x30) >> 4, (thr->code.data & 0x7000) >> 10 | (thr->code.data & 0xc0) >> 6, &shift_n);

		if (d == 15 && set_flags)
		{
			throw "CMP (register)";
		}
		if (n == 13)
		{
			throw "SUB (SP minus register)";
		}
		break;
	}
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}

	if (ConditionPassed(thr, cond))
	{
		const u32 shifted = Shift(thr->read_gpr(m), shift_t, shift_n, thr->APSR.C);
		if (set_flags)
		{
			bool carry, overflow;
			const u32 res = AddWithCarry(thr->read_gpr(n), ~shifted, true, carry, overflow);
			thr->write_gpr(d, res);
			thr->APSR.N = res >> 31;
			thr->APSR.Z = res == 0;
			thr->APSR.C = carry;
			thr->APSR.V = overflow;
		}
		else
		{
			thr->write_gpr(d, thr->read_gpr(n) - shifted);
		}
	}
}

void ARMv7_instrs::SUB_RSR(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SUB_SPI(ARMv7Thread* thr, const ARMv7_encoding type)
{
	u32 cond = thr->ITSTATE.advance();
	u32 d = 13;
	bool set_flags = false;
	u32 imm32 = 0;

	switch (type)
	{
	case T1:
	{
		imm32 = (thr->code.data & 0x7f) << 2;
		break;
	}
	case T2:
	{
		d = (thr->code.data & 0xf00) >> 8;
		set_flags = (thr->code.data & 0x100000);
		imm32 = ThumbExpandImm(thr, (thr->code.data & 0x4000000) >> 15 | (thr->code.data & 0x7000) >> 4 | (thr->code.data & 0xff));

		if (d == 15 && set_flags)
		{
			throw "CMP (immediate)";
		}
		break;
	}
	case T3:
	{
		d = (thr->code.data & 0xf00) >> 8;
		set_flags = false;
		imm32 = (thr->code.data & 0x4000000) >> 15 | (thr->code.data & 0x7000) >> 4 | (thr->code.data & 0xff);
		break;
	}
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}

	if (ConditionPassed(thr, cond))
	{
		if (set_flags)
		{
			bool carry, overflow;
			const u32 res = AddWithCarry(thr->SP, ~imm32, true, carry, overflow);
			thr->write_gpr(d, res);
			thr->APSR.N = res >> 31;
			thr->APSR.Z = res == 0;
			thr->APSR.C = carry;
			thr->APSR.V = overflow;
		}
		else
		{
			thr->write_gpr(d, thr->SP - imm32);
		}
	}
}

void ARMv7_instrs::SUB_SPR(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::SVC(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::SXTAB(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SXTAB16(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SXTAH(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SXTB(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SXTB16(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::SXTH(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::TB_(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::TEQ_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::TEQ_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::TEQ_RSR(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::TST_IMM(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::TST_REG(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::TST_RSR(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7_instrs::UADD16(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::UADD8(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::UASX(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::UBFX(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::UDIV(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::UHADD16(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::UHADD8(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::UHASX(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::UHSAX(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::UHSUB16(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::UHSUB8(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::UMAAL(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::UMLAL(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::UMULL(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::UQADD16(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::UQADD8(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::UQASX(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::UQSAX(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::UQSUB16(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::UQSUB8(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::USAD8(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::USADA8(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::USAT(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::USAT16(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::USAX(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::USUB16(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::USUB8(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::UXTAB(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::UXTAB16(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::UXTAH(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::UXTB(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::UXTB16(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7_instrs::UXTH(ARMv7Thread* thr, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}