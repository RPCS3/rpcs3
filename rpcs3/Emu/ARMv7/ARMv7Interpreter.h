#pragma once
#include "Emu/ARMv7/ARMv7Opcodes.h"

class ARMv7Interpreter : public ARMv7Opcodes
{
	ARMv7Thread& CPU;

public:
	ARMv7Interpreter(ARMv7Thread& cpu) : CPU(cpu)
	{
	}

	enum SRType
	{
		SRType_None,
		SRType_LSL,
		SRType_LSR,
		SRType_ASR,
		SRType_ROR,
		SRType_RRX,
	};

	template<typename T>
	bool IsZero(T x)
	{
		return x == T(0);
	}

	template<typename T>
	bool IsOnes(T x)
	{
		return x == ~T(0);
	}

	template<typename T>
	u8 IsZeroBit(T x)
	{
		return IsZero(x) ? 1 : 0;
	}

	template<typename T>
	bool IsOnesBit(T x, u8 len = sizeof(T) * 8)
	{
		return IsOnes(x) ? 1 : 0;
	}

	template<typename T>
	u8 BitCount(T x, u8 len = sizeof(T) * 8)
	{
		u8 result = 0;

		for(u8 mask=1 << (len - 1); mask; mask >>= 1)
		{
			if(x & mask) result++;
		}

		return result;
	}

	template<typename T>
	s8 LowestSetBit(T x, u8 len = sizeof(T) * 8)
	{
		if(!x) return len;

		u8 result = 0;

		for(T mask=1, i=0; i<len && (x & mask) == 0; mask <<= 1, i++)
		{
			result++;
		}

		return result;
	}

	template<typename T>
	s8 HighestSetBit(T x, u8 len = sizeof(T) * 8)
	{
		if(!x) return -1;

		u8 result = len;

		for(T mask=T(1) << (len - 1); (x & mask) == 0; mask >>= 1)
		{
			result--;
		}

		return result;
	}

	template<typename T>
	s8 CountLeadingZeroBits(T x, u8 len = sizeof(T) * 8)
	{
		return len - 1 - HighestSetBit(x, len);
	}

	SRType DecodeImmShift(u8 type, u8 imm5, uint* shift_n)
	{
		SRType shift_t = SRType_None;

		switch(type)
		{
		case 0: shift_t = SRType_LSL; if(shift_n) *shift_n = imm5; break;
		case 1: shift_t = SRType_LSR; if(shift_n) *shift_n = imm5 == 0 ? 32 : imm5; break;
		case 2: shift_t = SRType_ASR; if(shift_n) *shift_n = imm5 == 0 ? 32 : imm5; break;
		case 3:
			if(imm5 == 0)
			{
				shift_t = SRType_RRX; if(shift_n) *shift_n = 1;
			}
			else
			{
				shift_t = SRType_ROR; if(shift_n) *shift_n = imm5;
			}
		break;
		}

		return shift_t;
	}

	SRType DecodeRegShift(u8 type)
	{
		SRType shift_t = SRType_None;

		switch(type)
		{
		case 0: shift_t = SRType_LSL; break;
		case 1: shift_t = SRType_LSR; break;
		case 2: shift_t = SRType_ASR; break;
		case 3: shift_t = SRType_ROR; break;
		}

		return shift_t;
	}

	u32 LSL_C(u32 x, int shift, bool& carry_out)
	{
		u32 extended_x = x << shift;
		carry_out = (extended_x >> 31) ? 1 : 0;
		return extended_x & ~(1 << 31);
	}

	u32 LSL(u32 x, int shift)
	{
		if(!shift) return x;
		bool carry_out;
		return LSL_C(x, shift, carry_out);
	}

	u32 LSR_C(u32 x, int shift, bool& carry_out)
	{
		carry_out = (x >> (shift - 1)) & 0x1;
		return x >> shift;
	}

	u32 LSR(u32 x, int shift)
	{
		if(!shift) return x;
		bool carry_out;
		return LSR_C(x, shift, carry_out);
	}

	s32 ASR_C(s32 x, int shift, bool& carry_out)
	{
		carry_out = (x >> (shift - 1)) & 0x1;
		return x >> shift;
	}

	s32 ASR(s32 x, int shift)
	{
		if(!shift) return x;
		bool carry_out;
		return ASR_C(x, shift, carry_out);
	}

	u32 ROR_C(u32 x, int shift, bool& carry_out)
	{
		u32 result = LSR(x, shift) | LSL(x, 32 - shift);
		carry_out = (result >> 30) & 0x1;
		return result;
	}

	s32 ROR(s32 x, int shift)
	{
		if(!shift) return x;
		bool carry_out;
		return ROR_C(x, shift, carry_out);
	}

	template<typename T> T RRX_C(T x, bool carry_in, bool& carry_out)
	{
		carry_out = x & 0x1;
		return ((u32)carry_in << 31) | (x & 0x7ffffffe);
	}

	s32 RRX(s32 x, int shift)
	{
		if(!shift) return x;
		bool carry_out;
		return RRX_C(x, shift, carry_out);
	}

	template<typename T> T Shift_C(T value, SRType type, uint amount, bool carry_in, bool& carry_out)
	{
		if(amount)
		{
			switch(type)
			{
			case SRType_LSL: return LSL_C(value, amount, carry_out);
			case SRType_LSR: return LSR_C(value, amount, carry_out);
			case SRType_ASR: return ASR_C(value, amount, carry_out);
			case SRType_ROR: return ROR_C(value, amount, carry_out);
			case SRType_RRX: return RRX_C(value, amount, carry_out);
			}
		}

		carry_out = carry_in;
		return value;
	}

	template<typename T> T Shift(T value, SRType type, uint amount, bool carry_in)
	{
		bool carry_out;
		return Shift_C(value, type, amount, carry_in, carry_out);
	}

	template<typename T> T AddWithCarry(T x, T y, bool carry_in, bool& carry_out, bool& overflow)
	{
		uint unsigned_sum = (uint)x + (uint)y + (uint)carry_in;
		int signed_sum = (int)x + (int)y + (int)carry_in;
		T result = unsigned_sum & ~(T(1) << (sizeof(T) - 1));
		carry_out = (uint)result != unsigned_sum;
		overflow = (int)result != signed_sum;
		return result;
	}

	bool ConditionPassed(u8 cond) const
	{
		bool result = false;

		switch(cond >> 1)
		{
		case 0: result = CPU.APSR.Z == 1; break;
		case 1: result = CPU.APSR.C == 1; break;
		case 2: result = CPU.APSR.N == 1; break;
		case 3: result = CPU.APSR.V == 1; break;
		case 4: result = CPU.APSR.C == 1 && CPU.APSR.Z == 0; break;
		case 5: result = CPU.APSR.N == CPU.APSR.V; break;
		case 6: result = CPU.APSR.N == CPU.APSR.V && CPU.APSR.Z == 0; break;
		case 7: return true;
		}

		if(cond & 0x1)
		{
			return !result;
		}

		return result;
	}

protected:
	virtual void UNK(const u32 data);

	virtual void NULL_OP(const u32 data, const ARMv7_encoding type);
	virtual void NOP(const u32 data, const ARMv7_encoding type);

	virtual void PUSH(const u32 data, const ARMv7_encoding type);
	virtual void POP(const u32 data, const ARMv7_encoding type);

	virtual void B(const u32 data, const ARMv7_encoding type);
	virtual void CBZ(const u32 data, const ARMv7_encoding type);
	virtual void CBNZ(const u32 data, const ARMv7_encoding type);
	virtual void BL(const u32 data, const ARMv7_encoding type);
	virtual void BLX(const u32 data, const ARMv7_encoding type);

	virtual void ADC_IMM(const u32 data, const ARMv7_encoding type);
	virtual void ADC_REG(const u32 data, const ARMv7_encoding type);
	virtual void ADC_RSR(const u32 data, const ARMv7_encoding type);

	virtual void ADD_IMM(const u32 data, const ARMv7_encoding type);
	virtual void ADD_REG(const u32 data, const ARMv7_encoding type);
	virtual void ADD_RSR(const u32 data, const ARMv7_encoding type);
	virtual void ADD_SPI(const u32 data, const ARMv7_encoding type);
	virtual void ADD_SPR(const u32 data, const ARMv7_encoding type);

	virtual void MOV_IMM(const u32 data, const ARMv7_encoding type);
	virtual void MOV_REG(const u32 data, const ARMv7_encoding type);
	virtual void MOVT(const u32 data, const ARMv7_encoding type);

	virtual void SUB_IMM(const u32 data, const ARMv7_encoding type);
	virtual void SUB_REG(const u32 data, const ARMv7_encoding type);
	virtual void SUB_RSR(const u32 data, const ARMv7_encoding type);
	virtual void SUB_SPI(const u32 data, const ARMv7_encoding type);
	virtual void SUB_SPR(const u32 data, const ARMv7_encoding type);

	virtual void STR_IMM(const u32 data, const ARMv7_encoding type);
	virtual void STR_REG(const u32 data, const ARMv7_encoding type);
};
