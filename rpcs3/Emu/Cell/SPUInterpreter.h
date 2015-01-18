#pragma once

#include <fenv.h>

static void SetHostRoundingMode(u32 rn)
{
	switch (rn)
	{
	case FPSCR_RN_NEAR:
		fesetround(FE_TONEAREST);
		break;
	case FPSCR_RN_ZERO:
		fesetround(FE_TOWARDZERO);
		break;
	case FPSCR_RN_PINF:
		fesetround(FE_UPWARD);
		break;
	case FPSCR_RN_MINF:
		fesetround(FE_DOWNWARD);
		break;
	}
}

#define UNIMPLEMENTED() UNK(__FUNCTION__)

#define MEM_AND_REG_HASH() \
	unsigned char mem_h[20]; sha1(vm::get_ptr<u8>(CPU.ls_offset), 256*1024, mem_h); \
	unsigned char reg_h[20]; sha1((const unsigned char*)CPU.GPR, sizeof(CPU.GPR), reg_h); \
	LOG_NOTICE(Log::SPU, "Mem hash: 0x%llx, reg hash: 0x%llx", *(u64*)mem_h, *(u64*)reg_h);

#define LOG2_OPCODE(...) //MEM_AND_REG_HASH(); LOG_NOTICE(Log::SPU, __FUNCTION__ "(): " __VA_ARGS__)

#define LOG5_OPCODE(...) ///

// Floating-point utility constants and functions
static const u32 FLOAT_MAX_NORMAL_I = 0x7F7FFFFF;
static const float& FLOAT_MAX_NORMAL = (float&)FLOAT_MAX_NORMAL_I;
static const u32 FLOAT_NAN_I = 0x7FC00000;
static const float& FLOAT_NAN = (float&)FLOAT_NAN_I;
static const u64 DOUBLE_NAN_I = 0x7FF8000000000000ULL;
static const double& DOUBLE_NAN = (double&)DOUBLE_NAN_I;
static inline bool issnan(double x) {return isnan(x) && ((s64&)x)<<12 > 0;}
static inline bool issnan(float x) {return isnan(x) && ((s32&)x)<<9 > 0;}
static inline int fexpf(float x) {return ((u32&)x >> 23) & 0xFF;}
static inline bool isextended(float x) {return fexpf(x) == 255;}
static inline float extended(bool sign, u32 mantissa) // returns -1^sign * 2^127 * (1.mantissa)
{
    u32 bits = sign<<31 | 0x7F800000 | mantissa;
    return (float&)bits;
}
static inline float ldexpf_extended(float x, int exp)  // ldexpf() for extended values, assumes result is in range
{
    u32 bits = (u32&)x;
    if (bits << 1 != 0) bits += exp * 0x00800000;
    return (float&)bits;
}
static inline bool isdenormal(float x)
{
	const int fpc = _fpclass(x);
#ifdef __GNUG__
	return fpc == FP_SUBNORMAL;
#else
	return (fpc & (_FPCLASS_PD | _FPCLASS_ND)) != 0;
#endif
}
static inline bool isdenormal(double x)
{
	const int fpc = _fpclass(x);
#ifdef __GNUG__
	return fpc == FP_SUBNORMAL;
#else
	return (fpc & (_FPCLASS_PD | _FPCLASS_ND)) != 0;
#endif
}
static double SilenceNaN(double x)
{
	u64 bits = (u64&)x;
	bits |= 0x0008000000000000ULL;
	return (double&)bits;
}


class SPUInterpreter : public SPUOpcodes
{
private:
	SPUThread& CPU;

public:
	SPUInterpreter(SPUThread& cpu) : CPU(cpu)
	{
	}

private:
	void SysCall()
	{
	}

	//0 - 10
	void STOP(u32 code)
	{
		CPU.StopAndSignal(code);
		LOG2_OPCODE();
	}
	void LNOP()
	{
	}
	void SYNC(u32 Cbit)
	{
		// This instruction must be used following a store instruction that modifies the instruction stream.
		_mm_mfence();
	}
	void DSYNC()
	{
		// This instruction forces all earlier load, store, and channel instructions to complete before proceeding.
		_mm_mfence();
	}
	void MFSPR(u32 rt, u32 sa)
	{
		CPU.GPR[rt].clear();  // All SPRs read as zero.
	}
	void RDCH(u32 rt, u32 ra)
	{
		CPU.ReadChannel(CPU.GPR[rt], ra);
	}
	void RCHCNT(u32 rt, u32 ra)
	{
		CPU.GPR[rt].clear();
		CPU.GPR[rt]._u32[3] = CPU.GetChannelCount(ra);
	}
	void SF(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._u32[0] = CPU.GPR[rb]._u32[0] - CPU.GPR[ra]._u32[0];
		CPU.GPR[rt]._u32[1] = CPU.GPR[rb]._u32[1] - CPU.GPR[ra]._u32[1];
		CPU.GPR[rt]._u32[2] = CPU.GPR[rb]._u32[2] - CPU.GPR[ra]._u32[2];
		CPU.GPR[rt]._u32[3] = CPU.GPR[rb]._u32[3] - CPU.GPR[ra]._u32[3];
	}
	void OR(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._u32[0] = CPU.GPR[ra]._u32[0] | CPU.GPR[rb]._u32[0];
		CPU.GPR[rt]._u32[1] = CPU.GPR[ra]._u32[1] | CPU.GPR[rb]._u32[1];
		CPU.GPR[rt]._u32[2] = CPU.GPR[ra]._u32[2] | CPU.GPR[rb]._u32[2];
		CPU.GPR[rt]._u32[3] = CPU.GPR[ra]._u32[3] | CPU.GPR[rb]._u32[3];
	}
	void BG(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._u32[0] = CPU.GPR[ra]._u32[0] > CPU.GPR[rb]._u32[0] ? 0 : 1;
		CPU.GPR[rt]._u32[1] = CPU.GPR[ra]._u32[1] > CPU.GPR[rb]._u32[1] ? 0 : 1;
		CPU.GPR[rt]._u32[2] = CPU.GPR[ra]._u32[2] > CPU.GPR[rb]._u32[2] ? 0 : 1;
		CPU.GPR[rt]._u32[3] = CPU.GPR[ra]._u32[3] > CPU.GPR[rb]._u32[3] ? 0 : 1;
	}
	void SFH(u32 rt, u32 ra, u32 rb)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = CPU.GPR[rb]._u16[h] - CPU.GPR[ra]._u16[h];
	}
	void NOR(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._u32[0] = ~(CPU.GPR[ra]._u32[0] | CPU.GPR[rb]._u32[0]);
		CPU.GPR[rt]._u32[1] = ~(CPU.GPR[ra]._u32[1] | CPU.GPR[rb]._u32[1]);
		CPU.GPR[rt]._u32[2] = ~(CPU.GPR[ra]._u32[2] | CPU.GPR[rb]._u32[2]);
		CPU.GPR[rt]._u32[3] = ~(CPU.GPR[ra]._u32[3] | CPU.GPR[rb]._u32[3]);
	}
	void ABSDB(u32 rt, u32 ra, u32 rb)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = CPU.GPR[rb]._u8[b] > CPU.GPR[ra]._u8[b] ? CPU.GPR[rb]._u8[b] - CPU.GPR[ra]._u8[b] : CPU.GPR[ra]._u8[b] - CPU.GPR[rb]._u8[b];
	}
	void ROT(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._u32[0] = (CPU.GPR[ra]._u32[0] << (CPU.GPR[rb]._u32[0] & 0x1f)) | (CPU.GPR[ra]._u32[0] >> (32 - (CPU.GPR[rb]._u32[0] & 0x1f)));
		CPU.GPR[rt]._u32[1] = (CPU.GPR[ra]._u32[1] << (CPU.GPR[rb]._u32[1] & 0x1f)) | (CPU.GPR[ra]._u32[1] >> (32 - (CPU.GPR[rb]._u32[1] & 0x1f)));
		CPU.GPR[rt]._u32[2] = (CPU.GPR[ra]._u32[2] << (CPU.GPR[rb]._u32[2] & 0x1f)) | (CPU.GPR[ra]._u32[2] >> (32 - (CPU.GPR[rb]._u32[2] & 0x1f)));
		CPU.GPR[rt]._u32[3] = (CPU.GPR[ra]._u32[3] << (CPU.GPR[rb]._u32[3] & 0x1f)) | (CPU.GPR[ra]._u32[3] >> (32 - (CPU.GPR[rb]._u32[3] & 0x1f)));
	}
	void ROTM(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._u32[0] = ((0 - CPU.GPR[rb]._u32[0]) & 0x3f) < 32 ? CPU.GPR[ra]._u32[0] >> ((0 - CPU.GPR[rb]._u32[0]) & 0x3f) : 0;
		CPU.GPR[rt]._u32[1] = ((0 - CPU.GPR[rb]._u32[1]) & 0x3f) < 32 ? CPU.GPR[ra]._u32[1] >> ((0 - CPU.GPR[rb]._u32[1]) & 0x3f) : 0;
		CPU.GPR[rt]._u32[2] = ((0 - CPU.GPR[rb]._u32[2]) & 0x3f) < 32 ? CPU.GPR[ra]._u32[2] >> ((0 - CPU.GPR[rb]._u32[2]) & 0x3f) : 0;
		CPU.GPR[rt]._u32[3] = ((0 - CPU.GPR[rb]._u32[3]) & 0x3f) < 32 ? CPU.GPR[ra]._u32[3] >> ((0 - CPU.GPR[rb]._u32[3]) & 0x3f) : 0;
	}
	void ROTMA(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._s32[0] = ((0 - CPU.GPR[rb]._u32[0]) & 0x3f) < 32 ? CPU.GPR[ra]._s32[0] >> ((0 - CPU.GPR[rb]._u32[0]) & 0x3f) : CPU.GPR[ra]._s32[0] >> 31;
		CPU.GPR[rt]._s32[1] = ((0 - CPU.GPR[rb]._u32[1]) & 0x3f) < 32 ? CPU.GPR[ra]._s32[1] >> ((0 - CPU.GPR[rb]._u32[1]) & 0x3f) : CPU.GPR[ra]._s32[1] >> 31;
		CPU.GPR[rt]._s32[2] = ((0 - CPU.GPR[rb]._u32[2]) & 0x3f) < 32 ? CPU.GPR[ra]._s32[2] >> ((0 - CPU.GPR[rb]._u32[2]) & 0x3f) : CPU.GPR[ra]._s32[2] >> 31;
		CPU.GPR[rt]._s32[3] = ((0 - CPU.GPR[rb]._u32[3]) & 0x3f) < 32 ? CPU.GPR[ra]._s32[3] >> ((0 - CPU.GPR[rb]._u32[3]) & 0x3f) : CPU.GPR[ra]._s32[3] >> 31;
	}
	void SHL(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._u32[0] = (CPU.GPR[rb]._u32[0] & 0x3f) > 31 ? 0 : CPU.GPR[ra]._u32[0] << (CPU.GPR[rb]._u32[0] & 0x3f);
		CPU.GPR[rt]._u32[1] = (CPU.GPR[rb]._u32[1] & 0x3f) > 31 ? 0 : CPU.GPR[ra]._u32[1] << (CPU.GPR[rb]._u32[1] & 0x3f);
		CPU.GPR[rt]._u32[2] = (CPU.GPR[rb]._u32[2] & 0x3f) > 31 ? 0 : CPU.GPR[ra]._u32[2] << (CPU.GPR[rb]._u32[2] & 0x3f);
		CPU.GPR[rt]._u32[3] = (CPU.GPR[rb]._u32[3] & 0x3f) > 31 ? 0 : CPU.GPR[ra]._u32[3] << (CPU.GPR[rb]._u32[3] & 0x3f);
	}
	void ROTH(u32 rt, u32 ra, u32 rb)
	{
		for (int h = 0; h < 8; h++) 
			CPU.GPR[rt]._u16[h] = (CPU.GPR[ra]._u16[h] << (CPU.GPR[rb]._u16[h] & 0xf)) | (CPU.GPR[ra]._u16[h] >> (16 - (CPU.GPR[rb]._u16[h] & 0xf)));
	}
	void ROTHM(u32 rt, u32 ra, u32 rb)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = ((0 - CPU.GPR[rb]._u16[h]) & 0x1f) < 16 ? CPU.GPR[ra]._u16[h] >> ((0 - CPU.GPR[rb]._u16[h]) & 0x1f) : 0;
	}
	void ROTMAH(u32 rt, u32 ra, u32 rb)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._s16[h] = ((0 - CPU.GPR[rb]._u16[h]) & 0x1f) < 16 ? CPU.GPR[ra]._s16[h] >> ((0 - CPU.GPR[rb]._u16[h]) & 0x1f) : CPU.GPR[ra]._s16[h] >> 15;
	}
	void SHLH(u32 rt, u32 ra, u32 rb)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = (CPU.GPR[rb]._u16[h] & 0x1f) > 15 ? 0 : CPU.GPR[ra]._u16[h] << (CPU.GPR[rb]._u16[h] & 0x1f);
	}
	void ROTI(u32 rt, u32 ra, s32 i7)
	{
		const int nRot = i7 & 0x1f;
		CPU.GPR[rt]._u32[0] = (CPU.GPR[ra]._u32[0] << nRot) | (CPU.GPR[ra]._u32[0] >> (32 - nRot));
		CPU.GPR[rt]._u32[1] = (CPU.GPR[ra]._u32[1] << nRot) | (CPU.GPR[ra]._u32[1] >> (32 - nRot));
		CPU.GPR[rt]._u32[2] = (CPU.GPR[ra]._u32[2] << nRot) | (CPU.GPR[ra]._u32[2] >> (32 - nRot));
		CPU.GPR[rt]._u32[3] = (CPU.GPR[ra]._u32[3] << nRot) | (CPU.GPR[ra]._u32[3] >> (32 - nRot));
	}
	void ROTMI(u32 rt, u32 ra, s32 i7)
	{
		const int nRot = (0 - i7) & 0x3f;
		CPU.GPR[rt]._u32[0] = nRot < 32 ? CPU.GPR[ra]._u32[0] >> nRot : 0;
		CPU.GPR[rt]._u32[1] = nRot < 32 ? CPU.GPR[ra]._u32[1] >> nRot : 0;
		CPU.GPR[rt]._u32[2] = nRot < 32 ? CPU.GPR[ra]._u32[2] >> nRot : 0;
		CPU.GPR[rt]._u32[3] = nRot < 32 ? CPU.GPR[ra]._u32[3] >> nRot : 0;
	}
	void ROTMAI(u32 rt, u32 ra, s32 i7)
	{
		const int nRot = (0 - i7) & 0x3f;
		CPU.GPR[rt]._s32[0] = nRot < 32 ? CPU.GPR[ra]._s32[0] >> nRot : CPU.GPR[ra]._s32[0] >> 31;
		CPU.GPR[rt]._s32[1] = nRot < 32 ? CPU.GPR[ra]._s32[1] >> nRot : CPU.GPR[ra]._s32[1] >> 31;
		CPU.GPR[rt]._s32[2] = nRot < 32 ? CPU.GPR[ra]._s32[2] >> nRot : CPU.GPR[ra]._s32[2] >> 31;
		CPU.GPR[rt]._s32[3] = nRot < 32 ? CPU.GPR[ra]._s32[3] >> nRot : CPU.GPR[ra]._s32[3] >> 31;
	}
	void SHLI(u32 rt, u32 ra, s32 i7)
	{
		const u32 s = i7 & 0x3f;

		for (u32 j = 0; j < 4; ++j)
			CPU.GPR[rt]._u32[j] = (s >= 32) ? 0 : CPU.GPR[ra]._u32[j] << s;
	}
	void ROTHI(u32 rt, u32 ra, s32 i7)
	{
		const int nRot = i7 & 0xf;

		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = (CPU.GPR[ra]._u16[h] << nRot) | (CPU.GPR[ra]._u16[h] >> (16 - nRot));
	}
	void ROTHMI(u32 rt, u32 ra, s32 i7)
	{
		const int nRot = (0 - i7) & 0x1f;

		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = nRot < 16 ? CPU.GPR[ra]._u16[h] >> nRot : 0;
	}
	void ROTMAHI(u32 rt, u32 ra, s32 i7)
	{
		const int nRot = (0 - i7) & 0x1f;

		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._s16[h] = nRot < 16 ? CPU.GPR[ra]._s16[h] >> nRot : CPU.GPR[ra]._s16[h] >> 15;
	}
	void SHLHI(u32 rt, u32 ra, s32 i7)
	{
		const int nRot = i7 & 0x1f;

		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = nRot > 15 ? 0 : CPU.GPR[ra]._u16[h] << nRot;
	}
	void A(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._u32[0] = CPU.GPR[ra]._u32[0] + CPU.GPR[rb]._u32[0];
		CPU.GPR[rt]._u32[1] = CPU.GPR[ra]._u32[1] + CPU.GPR[rb]._u32[1];
		CPU.GPR[rt]._u32[2] = CPU.GPR[ra]._u32[2] + CPU.GPR[rb]._u32[2];
		CPU.GPR[rt]._u32[3] = CPU.GPR[ra]._u32[3] + CPU.GPR[rb]._u32[3];
	}
	void AND(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._u32[0] = CPU.GPR[ra]._u32[0] & CPU.GPR[rb]._u32[0];
		CPU.GPR[rt]._u32[1] = CPU.GPR[ra]._u32[1] & CPU.GPR[rb]._u32[1];
		CPU.GPR[rt]._u32[2] = CPU.GPR[ra]._u32[2] & CPU.GPR[rb]._u32[2];
		CPU.GPR[rt]._u32[3] = CPU.GPR[ra]._u32[3] & CPU.GPR[rb]._u32[3];
	}
	void CG(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._u32[0] = ((CPU.GPR[ra]._u32[0] + CPU.GPR[rb]._u32[0]) < CPU.GPR[ra]._u32[0]) ? 1 : 0;
		CPU.GPR[rt]._u32[1] = ((CPU.GPR[ra]._u32[1] + CPU.GPR[rb]._u32[1]) < CPU.GPR[ra]._u32[1]) ? 1 : 0;
		CPU.GPR[rt]._u32[2] = ((CPU.GPR[ra]._u32[2] + CPU.GPR[rb]._u32[2]) < CPU.GPR[ra]._u32[2]) ? 1 : 0;
		CPU.GPR[rt]._u32[3] = ((CPU.GPR[ra]._u32[3] + CPU.GPR[rb]._u32[3]) < CPU.GPR[ra]._u32[3]) ? 1 : 0;
	}
	void AH(u32 rt, u32 ra, u32 rb)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = CPU.GPR[ra]._u16[h] + CPU.GPR[rb]._u16[h];
	}
	void NAND(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._u32[0] = ~(CPU.GPR[ra]._u32[0] & CPU.GPR[rb]._u32[0]);
		CPU.GPR[rt]._u32[1] = ~(CPU.GPR[ra]._u32[1] & CPU.GPR[rb]._u32[1]);
		CPU.GPR[rt]._u32[2] = ~(CPU.GPR[ra]._u32[2] & CPU.GPR[rb]._u32[2]);
		CPU.GPR[rt]._u32[3] = ~(CPU.GPR[ra]._u32[3] & CPU.GPR[rb]._u32[3]);
	}
	void AVGB(u32 rt, u32 ra, u32 rb)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = (CPU.GPR[ra]._u8[b] + CPU.GPR[rb]._u8[b] + 1) >> 1;
	}
	void MTSPR(u32 rt, u32 sa)
	{
		// SPR writes are ignored.
	}
	void WRCH(u32 ra, u32 rt)
	{
		CPU.WriteChannel(ra, CPU.GPR[rt]);
	}
	void BIZ(u32 intr, u32 rt, u32 ra)
	{
		switch (intr)
		{
		case 0: break;
		case 0x10: break; // enable interrupts
		case 0x20: break; // disable interrupts
		default: UNIMPLEMENTED(); return;
		}

		u32 target = branchTarget(CPU.GPR[ra]._u32[3], 0);
		if (CPU.GPR[rt]._u32[3] == 0)
		{
			LOG5_OPCODE("taken (0x%x)", target);
			CPU.SetBranch(target);
		}
		else
		{
			LOG5_OPCODE("not taken (0x%x)", target);
		}
	}
	void BINZ(u32 intr, u32 rt, u32 ra)
	{
		switch (intr)
		{
		case 0: break;
		case 0x10: break; // enable interrupts
		case 0x20: break; // disable interrupts
		default: UNIMPLEMENTED(); return;
		}

		u32 target = branchTarget(CPU.GPR[ra]._u32[3], 0);
		if (CPU.GPR[rt]._u32[3] != 0)
		{
			LOG5_OPCODE("taken (0x%x)", target);
			CPU.SetBranch(target);
		}
		else
		{
			LOG5_OPCODE("not taken (0x%x)", target);
		}
	}
	void BIHZ(u32 intr, u32 rt, u32 ra)
	{
		switch (intr)
		{
		case 0: break;
		case 0x10: break; // enable interrupts
		case 0x20: break; // disable interrupts
		default: UNIMPLEMENTED(); return;
		}

		u32 target = branchTarget(CPU.GPR[ra]._u32[3], 0);
		if (CPU.GPR[rt]._u16[6] == 0)
		{
			LOG5_OPCODE("taken (0x%x)", target);
			CPU.SetBranch(target);
		}
		else
		{
			LOG5_OPCODE("not taken (0x%x)", target);
		}
	}
	void BIHNZ(u32 intr, u32 rt, u32 ra)
	{
		switch (intr)
		{
		case 0: break;
		case 0x10: break; // enable interrupts
		case 0x20: break; // disable interrupts
		default: UNIMPLEMENTED(); return;
		}

		u32 target = branchTarget(CPU.GPR[ra]._u32[3], 0);
		if (CPU.GPR[rt]._u16[6] != 0)
		{
			LOG5_OPCODE("taken (0x%x)", target);
			CPU.SetBranch(target);
		}
		else
		{
			LOG5_OPCODE("not taken (0x%x)", target);
		}
	}
	void STOPD(u32 rc, u32 ra, u32 rb)
	{
		UNIMPLEMENTED(); // not used
	}
	void STQX(u32 rt, u32 ra, u32 rb)
	{
		u32 lsa = (CPU.GPR[ra]._u32[3] + CPU.GPR[rb]._u32[3]) & 0x3fff0;

		CPU.WriteLS128(lsa, CPU.GPR[rt]);
	}
	void BI(u32 intr, u32 ra)
	{
		switch (intr)
		{
		case 0: break;
		case 0x10: break; // enable interrupts
		case 0x20: break; // disable interrupts
		default: UNIMPLEMENTED(); return;
		}

		u32 target = branchTarget(CPU.GPR[ra]._u32[3], 0);
		LOG5_OPCODE("branch (0x%x)", target);
		CPU.SetBranch(target);
	}
	void BISL(u32 intr, u32 rt, u32 ra)
	{
		switch (intr)
		{
		case 0: break;
		case 0x10: break; // enable interrupts
		case 0x20: break; // disable interrupts
		default: UNIMPLEMENTED(); return;
		}

		u32 target = branchTarget(CPU.GPR[ra]._u32[3], 0);
		CPU.GPR[rt].clear();
		CPU.GPR[rt]._u32[3] = CPU.PC + 4;		
		LOG5_OPCODE("branch (0x%x)", target);
		CPU.SetBranch(target);
	}
	void IRET(u32 ra)
	{
		UNIMPLEMENTED(); // not used
	}
	void BISLED(u32 intr, u32 rt, u32 ra)
	{
		UNIMPLEMENTED(); // not used
	}
	void HBR(u32 p, u32 ro, u32 ra)
	{
	}
	void GB(u32 rt, u32 ra)
	{
		CPU.GPR[rt]._u32[3] =	(CPU.GPR[ra]._u32[0] & 1) |
										((CPU.GPR[ra]._u32[1] & 1) << 1) |
										((CPU.GPR[ra]._u32[2] & 1) << 2) |
										((CPU.GPR[ra]._u32[3] & 1) << 3);
		CPU.GPR[rt]._u32[2] = 0;
		CPU.GPR[rt]._u64[0] = 0;
	}
	void GBH(u32 rt, u32 ra)
	{
		u32 temp = 0;
		for (int h = 0; h < 8; h++)
			temp |= (CPU.GPR[ra]._u16[h] & 1) << h;
		CPU.GPR[rt]._u32[3] = temp;
		CPU.GPR[rt]._u32[2] = 0;
		CPU.GPR[rt]._u64[0] = 0;
	}
	void GBB(u32 rt, u32 ra)
	{
		u32 temp = 0;
		for (int b = 0; b < 16; b++)
			temp |= (CPU.GPR[ra]._u8[b] & 1) << b;
		CPU.GPR[rt]._u32[3] = temp;
		CPU.GPR[rt]._u32[2] = 0;
		CPU.GPR[rt]._u64[0] = 0;
	}
	void FSM(u32 rt, u32 ra)
	{
		const u32 pref = CPU.GPR[ra]._u32[3];
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = (pref & (1 << w)) ? ~0 : 0;
	}
	void FSMH(u32 rt, u32 ra)
	{
		const u32 pref = CPU.GPR[ra]._u32[3];
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = (pref & (1 << h)) ? ~0 : 0;
	}
	void FSMB(u32 rt, u32 ra)
	{
		const u32 pref = CPU.GPR[ra]._u32[3];
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = (pref & (1 << b)) ? ~0 : 0;
	}
	void FREST(u32 rt, u32 ra)
	{
		SetHostRoundingMode(FPSCR_RN_ZERO);
		for (int i = 0; i < 4; i++)
		{
			const float a = CPU.GPR[ra]._f[i];
			float result;
			if (fexpf(a) == 0)
			{
				CPU.FPSCR.setDivideByZeroFlag(i);
				result = extended(std::signbit(a), 0x7FFFFF);
			}
			else if (isextended(a))
				result = 0.0f;
			else
				result = 1 / a;
			CPU.GPR[rt]._f[i] = result;
		}
	}
	void FRSQEST(u32 rt, u32 ra)
	{
		SetHostRoundingMode(FPSCR_RN_ZERO);
		for (int i = 0; i < 4; i++)
		{
			const float a = CPU.GPR[ra]._f[i];
			float result;
			if (fexpf(a) == 0)
			{
				CPU.FPSCR.setDivideByZeroFlag(i);
				result = extended(0, 0x7FFFFF);
			}
			else if (isextended(a))
				result = 0.5f / sqrtf(fabsf(ldexpf_extended(a, -2)));
			else
				result = 1 / sqrtf(fabsf(a));
			CPU.GPR[rt]._f[i] = result;
		}
	}
	void LQX(u32 rt, u32 ra, u32 rb)
	{
		u32 lsa = (CPU.GPR[ra]._u32[3] + CPU.GPR[rb]._u32[3]) & 0x3fff0;

		CPU.GPR[rt] = CPU.ReadLS128(lsa);
	}
	void ROTQBYBI(u32 rt, u32 ra, u32 rb)
	{
		const int s = (CPU.GPR[rb]._u32[3] >> 3) & 0xf;
		const u128 temp = CPU.GPR[ra];
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = temp._u8[(b - s) & 0xf];
	}
	void ROTQMBYBI(u32 rt, u32 ra, u32 rb)
	{
		const int s = (0 - (CPU.GPR[rb]._u32[3] >> 3)) & 0x1f;
		const u128 temp = CPU.GPR[ra];
		CPU.GPR[rt].clear();
		for (int b = 0; b < 16 - s; b++)
			CPU.GPR[rt]._u8[b] = temp._u8[b + s];
	}
	void SHLQBYBI(u32 rt, u32 ra, u32 rb)
	{
		const int s = (CPU.GPR[rb]._u32[3] >> 3) & 0x1f;
		const u128 temp = CPU.GPR[ra];
		CPU.GPR[rt].clear();
		for (int b = s; b < 16; b++)
			CPU.GPR[rt]._u8[b] = temp._u8[b - s];
	}
	void CBX(u32 rt, u32 ra, u32 rb)
	{
		const u32 t = (CPU.GPR[rb]._u32[3] + CPU.GPR[ra]._u32[3]) & 0xF;

		if (ra == 1 && (CPU.GPR[ra]._u32[3] & 0xF))
		{
			LOG_ERROR(SPU, "%s(): SP = 0x%x", __FUNCTION__, CPU.GPR[ra]._u32[3]);
			Emu.Pause();
		}

		CPU.GPR[rt]._u64[0] = (u64)0x18191A1B1C1D1E1F;
		CPU.GPR[rt]._u64[1] = (u64)0x1011121314151617;
		CPU.GPR[rt]._u8[15 - t] = 0x03;
	}
	void CHX(u32 rt, u32 ra, u32 rb)
	{
		const u32 t = (CPU.GPR[rb]._u32[3] + CPU.GPR[ra]._u32[3]) & 0xE;

		if (ra == 1 && (CPU.GPR[ra]._u32[3] & 0xF))
		{
			LOG_ERROR(SPU, "%s(): SP = 0x%x", __FUNCTION__, CPU.GPR[ra]._u32[3]);
			Emu.Pause();
		}

		CPU.GPR[rt]._u64[0] = (u64)0x18191A1B1C1D1E1F;
		CPU.GPR[rt]._u64[1] = (u64)0x1011121314151617;
		CPU.GPR[rt]._u16[7 - (t >> 1)] = 0x0203;
	}
	void CWX(u32 rt, u32 ra, u32 rb)
	{
		const u32 t = (CPU.GPR[ra]._u32[3] + CPU.GPR[rb]._u32[3]) & 0xC;

		if (ra == 1 && (CPU.GPR[ra]._u32[3] & 0xF))
		{
			LOG_ERROR(SPU, "%s(): SP = 0x%x", __FUNCTION__, CPU.GPR[ra]._u32[3]);
			Emu.Pause();
		}
		
		CPU.GPR[rt]._u64[0] = (u64)0x18191A1B1C1D1E1F;
		CPU.GPR[rt]._u64[1] = (u64)0x1011121314151617;
		CPU.GPR[rt]._u32[3 - (t >> 2)] = 0x00010203;
	}
	void CDX(u32 rt, u32 ra, u32 rb)
	{
		const u32 t = (CPU.GPR[rb]._u32[3] + CPU.GPR[ra]._u32[3]) & 0x8;

		if (ra == 1 && (CPU.GPR[ra]._u32[3] & 0xF))
		{
			LOG_ERROR(SPU, "%s(): SP = 0x%x", __FUNCTION__, CPU.GPR[ra]._u32[3]);
			Emu.Pause();
		}

		CPU.GPR[rt]._u64[0] = (u64)0x18191A1B1C1D1E1F;
		CPU.GPR[rt]._u64[1] = (u64)0x1011121314151617;
		CPU.GPR[rt]._u64[1 - (t >> 3)] = (u64)0x0001020304050607;
	}
	void ROTQBI(u32 rt, u32 ra, u32 rb)
	{
		const int t = CPU.GPR[rb]._u32[3] & 0x7;
		if (t) // not an optimization, it fixes shifts
		{
			const u128 temp = CPU.GPR[ra];
			CPU.GPR[rt]._u32[0] = (temp._u32[0] << t) | (temp._u32[3] >> (32 - t));
			CPU.GPR[rt]._u32[1] = (temp._u32[1] << t) | (temp._u32[0] >> (32 - t));
			CPU.GPR[rt]._u32[2] = (temp._u32[2] << t) | (temp._u32[1] >> (32 - t));
			CPU.GPR[rt]._u32[3] = (temp._u32[3] << t) | (temp._u32[2] >> (32 - t));
		}
		else
		{
			CPU.GPR[rt] = CPU.GPR[ra];
		}
	}
	void ROTQMBI(u32 rt, u32 ra, u32 rb)
	{
		const int t = (0 - CPU.GPR[rb]._u32[3]) & 0x7;
		if (t) // not an optimization, it fixes shifts
		{
			const u128 temp = CPU.GPR[ra];
			CPU.GPR[rt]._u32[0] = (temp._u32[0] >> t) | (temp._u32[1] << (32 - t));
			CPU.GPR[rt]._u32[1] = (temp._u32[1] >> t) | (temp._u32[2] << (32 - t));
			CPU.GPR[rt]._u32[2] = (temp._u32[2] >> t) | (temp._u32[3] << (32 - t));
			CPU.GPR[rt]._u32[3] = (temp._u32[3] >> t);
		}
		else
		{
			CPU.GPR[rt] = CPU.GPR[ra];
		}
	}
	void SHLQBI(u32 rt, u32 ra, u32 rb)
	{
		const int t = CPU.GPR[rb]._u32[3] & 0x7;
		if (t) // not an optimization, it fixes shifts
		{
			const u128 temp = CPU.GPR[ra];
			CPU.GPR[rt]._u32[0] = (temp._u32[0] << t);
			CPU.GPR[rt]._u32[1] = (temp._u32[1] << t) | (temp._u32[0] >> (32 - t));
			CPU.GPR[rt]._u32[2] = (temp._u32[2] << t) | (temp._u32[1] >> (32 - t));
			CPU.GPR[rt]._u32[3] = (temp._u32[3] << t) | (temp._u32[2] >> (32 - t));
		}
		else
		{
			CPU.GPR[rt] = CPU.GPR[ra];
		}
	}
	void ROTQBY(u32 rt, u32 ra, u32 rb)
	{
		const int s = CPU.GPR[rb]._u32[3] & 0xf;
		const u128 temp = CPU.GPR[ra];
		for (int b = 0; b < 16; ++b)
			CPU.GPR[rt]._u8[b] = temp._u8[(b - s) & 0xf];
	}
	void ROTQMBY(u32 rt, u32 ra, u32 rb)
	{
		const int s = (0 - CPU.GPR[rb]._u32[3]) & 0x1f;
		const u128 temp = CPU.GPR[ra];
		CPU.GPR[rt].clear();
		for (int b = 0; b < 16 - s; b++)
			CPU.GPR[rt]._u8[b] = temp._u8[b + s];
	}
	void SHLQBY(u32 rt, u32 ra, u32 rb)
	{
		const int s = CPU.GPR[rb]._u32[3] & 0x1f;
		const u128 temp = CPU.GPR[ra];
		CPU.GPR[rt].clear();
		for (int b = s; b < 16; b++)
			CPU.GPR[rt]._u8[b] = temp._u8[b - s];
	}
	void ORX(u32 rt, u32 ra)
	{
		CPU.GPR[rt]._u32[3] = CPU.GPR[ra]._u32[0] | CPU.GPR[ra]._u32[1] | CPU.GPR[ra]._u32[2] | CPU.GPR[ra]._u32[3];
		CPU.GPR[rt]._u32[2] = 0;
		CPU.GPR[rt]._u64[0] = 0;
	}
	void CBD(u32 rt, u32 ra, s32 i7)
	{
		const int t = (CPU.GPR[ra]._u32[3] + i7) & 0xF;

		if (ra == 1 && (CPU.GPR[ra]._u32[3] & 0xF))
		{
			LOG_ERROR(SPU, "%s(): SP = 0x%x", __FUNCTION__, CPU.GPR[ra]._u32[3]);
			Emu.Pause();
		}

		CPU.GPR[rt]._u64[0] = (u64)0x18191A1B1C1D1E1F;
		CPU.GPR[rt]._u64[1] = (u64)0x1011121314151617;
		CPU.GPR[rt]._u8[15 - t] = 0x03;
	}
	void CHD(u32 rt, u32 ra, s32 i7)
	{
		const int t = (CPU.GPR[ra]._u32[3] + i7) & 0xE;

		if (ra == 1 && (CPU.GPR[ra]._u32[3] & 0xF))
		{
			LOG_ERROR(SPU, "%s(): SP = 0x%x", __FUNCTION__, CPU.GPR[ra]._u32[3]);
			Emu.Pause();
		}

		CPU.GPR[rt]._u64[0] = (u64)0x18191A1B1C1D1E1F;
		CPU.GPR[rt]._u64[1] = (u64)0x1011121314151617;
		CPU.GPR[rt]._u16[7 - (t >> 1)] = 0x0203;
	}
	void CWD(u32 rt, u32 ra, s32 i7)
	{
		const int t = (CPU.GPR[ra]._u32[3] + i7) & 0xC;

		if (ra == 1 && (CPU.GPR[ra]._u32[3] & 0xF))
		{
			LOG_ERROR(SPU, "%s(): SP = 0x%x", __FUNCTION__, CPU.GPR[ra]._u32[3]);
			Emu.Pause();
		}

		CPU.GPR[rt]._u64[0] = (u64)0x18191A1B1C1D1E1F;
		CPU.GPR[rt]._u64[1] = (u64)0x1011121314151617;
		CPU.GPR[rt]._u32[3 - (t >> 2)] = 0x00010203;
	}
	void CDD(u32 rt, u32 ra, s32 i7)
	{
		const int t = (CPU.GPR[ra]._u32[3] + i7) & 0x8;

		if (ra == 1 && (CPU.GPR[ra]._u32[3] & 0xF))
		{
			LOG_ERROR(SPU, "%s(): SP = 0x%x", __FUNCTION__, CPU.GPR[ra]._u32[3]);
			Emu.Pause();
		}

		CPU.GPR[rt]._u64[0] = (u64)0x18191A1B1C1D1E1F;
		CPU.GPR[rt]._u64[1] = (u64)0x1011121314151617;
		CPU.GPR[rt]._u64[1 - (t >> 3)] = (u64)0x0001020304050607;
	}
	void ROTQBII(u32 rt, u32 ra, s32 i7)
	{
		const int s = i7 & 0x7;
		if (s) // not an optimization, it fixes shifts
		{
			const u128 temp = CPU.GPR[ra];
			CPU.GPR[rt]._u32[0] = (temp._u32[0] << s) | (temp._u32[3] >> (32 - s));
			CPU.GPR[rt]._u32[1] = (temp._u32[1] << s) | (temp._u32[0] >> (32 - s));
			CPU.GPR[rt]._u32[2] = (temp._u32[2] << s) | (temp._u32[1] >> (32 - s));
			CPU.GPR[rt]._u32[3] = (temp._u32[3] << s) | (temp._u32[2] >> (32 - s));
		}
		else
		{
			CPU.GPR[rt] = CPU.GPR[ra];
		}
	}
	void ROTQMBII(u32 rt, u32 ra, s32 i7)
	{
		const int s = (0 - i7) & 0x7;
		if (s) // not an optimization, it fixes shifts
		{
			const u128 temp = CPU.GPR[ra];
			CPU.GPR[rt]._u32[0] = (temp._u32[0] >> s) | (temp._u32[1] << (32 - s));
			CPU.GPR[rt]._u32[1] = (temp._u32[1] >> s) | (temp._u32[2] << (32 - s));
			CPU.GPR[rt]._u32[2] = (temp._u32[2] >> s) | (temp._u32[3] << (32 - s));
			CPU.GPR[rt]._u32[3] = (temp._u32[3] >> s);
		}
		else
		{
			CPU.GPR[rt] = CPU.GPR[ra];
		}
	}
	void SHLQBII(u32 rt, u32 ra, s32 i7)
	{
		const int s = i7 & 0x7;
		if (s) // not an optimization, it fixes shifts
		{
			const u128 temp = CPU.GPR[ra];
			CPU.GPR[rt]._u32[0] = (temp._u32[0] << s);
			CPU.GPR[rt]._u32[1] = (temp._u32[1] << s) | (temp._u32[0] >> (32 - s));
			CPU.GPR[rt]._u32[2] = (temp._u32[2] << s) | (temp._u32[1] >> (32 - s));
			CPU.GPR[rt]._u32[3] = (temp._u32[3] << s) | (temp._u32[2] >> (32 - s));
		}
		else
		{
			CPU.GPR[rt] = CPU.GPR[ra];
		}
	}
	void ROTQBYI(u32 rt, u32 ra, s32 i7)
	{
		const int s = i7 & 0xf;
		const u128 temp = CPU.GPR[ra];
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = temp._u8[(b - s) & 0xf];
	}
	void ROTQMBYI(u32 rt, u32 ra, s32 i7)
	{
		const int s = (0 - i7) & 0x1f;
		const u128 temp = CPU.GPR[ra];
		CPU.GPR[rt].clear();
		for (int b = 0; b < 16 - s; b++)
			CPU.GPR[rt]._u8[b] = temp._u8[b + s];
	}
	void SHLQBYI(u32 rt, u32 ra, s32 i7)
	{
		const int s = i7 & 0x1f;
		const u128 temp = CPU.GPR[ra];
		CPU.GPR[rt].clear();
		for (int b = s; b < 16; b++)
			CPU.GPR[rt]._u8[b] = temp._u8[b - s];
	}
	void NOP(u32 rt)
	{
	}
	void CGT(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._s32[w] > CPU.GPR[rb]._s32[w] ? 0xffffffff : 0;
	}
	void XOR(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._u32[w] ^ CPU.GPR[rb]._u32[w];
	}
	void CGTH(u32 rt, u32 ra, u32 rb)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = CPU.GPR[ra]._s16[h] > CPU.GPR[rb]._s16[h] ? 0xffff : 0;
	}
	void EQV(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._u32[w] ^ (~CPU.GPR[rb]._u32[w]);
	}
	void CGTB(u32 rt, u32 ra, u32 rb)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._s8[b] > CPU.GPR[rb]._s8[b] ? 0xff : 0;
	}
	void SUMB(u32 rt, u32 ra, u32 rb)
	{
		const u128 _a = CPU.GPR[ra];
		const u128 _b = CPU.GPR[rb];
		for (int w = 0; w < 4; w++)
		{
			CPU.GPR[rt]._u16[w*2] = _a._u8[w*4] + _a._u8[w*4 + 1] + _a._u8[w*4 + 2] + _a._u8[w*4 + 3];
			CPU.GPR[rt]._u16[w*2 + 1] = _b._u8[w*4] + _b._u8[w*4 + 1] + _b._u8[w*4 + 2] + _b._u8[w*4 + 3];
		}
	}
	//HGT uses signed values.  HLGT uses unsigned values
	void HGT(u32 rt, s32 ra, s32 rb)
	{
		if (CPU.GPR[ra]._s32[3] > CPU.GPR[rb]._s32[3])
		{
			CPU.SPU.Status.SetValue(SPU_STATUS_STOPPED_BY_HALT);
			CPU.Stop();
		}
	}
	void CLZ(u32 rt, u32 ra)
	{
		for (int w = 0; w < 4; w++)
		{
			int nPos;

			for (nPos = 0; nPos < 32; nPos++)
				if (CPU.GPR[ra]._u32[w] & (1 << (31 - nPos)))
					break;

			CPU.GPR[rt]._u32[w] = nPos;
		}
	}
	void XSWD(u32 rt, u32 ra)
	{
		CPU.GPR[rt]._s64[0] = (s64)CPU.GPR[ra]._s32[0];
		CPU.GPR[rt]._s64[1] = (s64)CPU.GPR[ra]._s32[2];
	}
	void XSHW(u32 rt, u32 ra)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._s32[w] = (s32)CPU.GPR[ra]._s16[w*2];
	}
	void CNTB(u32 rt, u32 ra)
	{
		const u128 temp = CPU.GPR[ra];
		CPU.GPR[rt].clear();
		for (int b = 0; b < 16; b++)
			for (int i = 0; i < 8; i++)
				CPU.GPR[rt]._u8[b] += (temp._u8[b] & (1 << i)) ? 1 : 0;
	}
	void XSBH(u32 rt, u32 ra)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._s16[h] = (s16)CPU.GPR[ra]._s8[h*2];
	}
	void CLGT(u32 rt, u32 ra, u32 rb)
	{
		for(u32 i = 0; i < 4; ++i)
		{
			CPU.GPR[rt]._u32[i] = (CPU.GPR[ra]._u32[i] > CPU.GPR[rb]._u32[i]) ? 0xffffffff : 0x00000000;
		}
	}
	void ANDC(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._u32[w] & (~CPU.GPR[rb]._u32[w]);
	}
	void FCGT(u32 rt, u32 ra, u32 rb)
	{
		for (int i = 0; i < 4; i++)
		{
			const u32 a = CPU.GPR[ra]._u32[i];
			const u32 b = CPU.GPR[rb]._u32[i];
			const u32 abs_a = a & 0x7FFFFFFF;
			const u32 abs_b = b & 0x7FFFFFFF;
			const bool a_zero = (abs_a < 0x00800000);
			const bool b_zero = (abs_b < 0x00800000);
			bool pass;
			if (a_zero)
				pass = b >= 0x80800000;
			else if (b_zero)
				pass = (s32)a >= 0x00800000;
			else if (a >= 0x80000000)
				pass = (b >= 0x80000000 && a < b);
			else
				pass = (b >= 0x80000000 || a > b);
			CPU.GPR[rt]._u32[i] = pass ? 0xFFFFFFFF : 0;
		}
	}
	void DFCGT(u32 rt, u32 ra, u32 rb)
	{
		UNIMPLEMENTED(); // cannot be used
	}
	void FA_FS(u32 rt, u32 ra, u32 rb, bool sub)
	{
		SetHostRoundingMode(FPSCR_RN_ZERO);
		for (int w = 0; w < 4; w++)
		{
			const float a = CPU.GPR[ra]._f[w];
			const float b = sub ? -CPU.GPR[rb]._f[w] : CPU.GPR[rb]._f[w];
			float result;
			if (isdenormal(a))
			{
				CPU.FPSCR.setSinglePrecisionExceptionFlags(w, FPSCR_SDIFF);
				if (b == 0.0f || isdenormal(b))
					result = +0.0f;
				else
					result = b;
			}
			else if (isdenormal(b))
			{
				CPU.FPSCR.setSinglePrecisionExceptionFlags(w, FPSCR_SDIFF);
				if (a == 0.0f)
					result = +0.0f;
				else
					result = a;
			}
			else if (isextended(a) || isextended(b))
			{
				CPU.FPSCR.setSinglePrecisionExceptionFlags(w, FPSCR_SDIFF);
				if (isextended(a) && fexpf(b) < 255-32)
				{
					if (std::signbit(a) != std::signbit(b))
					{
						const u32 bits = (u32&)a - 1;
						result = (float&)bits;
					}
					else

						result = a;
				}
				else if (isextended(b) && fexpf(a) < 255-32)
				{
					if (std::signbit(a) != std::signbit(b))
					{
						const u32 bits = (u32&)b - 1;
						result = (float&)bits;
					}
					else
						result = b;
				}
				else
				{
					feclearexcept(FE_ALL_EXCEPT);
					result = ldexpf_extended(a, -1) + ldexpf_extended(b, -1);
					result = ldexpf_extended(result, 1);
					if (fetestexcept(FE_OVERFLOW))
					{
						CPU.FPSCR.setSinglePrecisionExceptionFlags(w, FPSCR_SOVF);
						result = extended(std::signbit(result), 0x7FFFFF);
					}
				}
			}
			else
			{
				result = a + b;
				if (result == copysignf(FLOAT_MAX_NORMAL, result))
				{
					result = ldexpf_extended(ldexpf(a,-1) + ldexpf(b,-1), 1);
					if (isextended(result))
						CPU.FPSCR.setSinglePrecisionExceptionFlags(w, FPSCR_SDIFF);
				}
				else if (isdenormal(result))
				{
					CPU.FPSCR.setSinglePrecisionExceptionFlags(w, FPSCR_SUNF | FPSCR_SDIFF);
					result = +0.0f;
				}
				else if (result == 0.0f)
				{
					if (fabsf(a) != fabsf(b))
						CPU.FPSCR.setSinglePrecisionExceptionFlags(w, FPSCR_SUNF | FPSCR_SDIFF);
					result = +0.0f;
				}
			}
			CPU.GPR[rt]._f[w] = result;
		}
	}
	void FA(u32 rt, u32 ra, u32 rb) {FA_FS(rt, ra, rb, false);}
	void FS(u32 rt, u32 ra, u32 rb) {FA_FS(rt, ra, rb, true);}
	void FM(u32 rt, u32 ra, u32 rb)
	{
		SetHostRoundingMode(FPSCR_RN_ZERO);
		for (int w = 0; w < 4; w++)
		{
			const float a = CPU.GPR[ra]._f[w];
			const float b = CPU.GPR[rb]._f[w];
			float result;
			if (isdenormal(a) || isdenormal(b))
			{
				CPU.FPSCR.setSinglePrecisionExceptionFlags(w, FPSCR_SDIFF);
				result = +0.0f;
			}
			else if (isextended(a) || isextended(b))
			{
				CPU.FPSCR.setSinglePrecisionExceptionFlags(w, FPSCR_SDIFF);
				const bool sign = std::signbit(a) ^ std::signbit(b);
				if (a == 0.0f || b == 0.0f)
				{
					result = +0.0f;
				}
				else if ((fexpf(a)-127) + (fexpf(b)-127) >= 129)
				{
					CPU.FPSCR.setSinglePrecisionExceptionFlags(w, FPSCR_SOVF);
					result = extended(sign, 0x7FFFFF);
				}
				else
				{
					if (isextended(a))
						result = ldexpf_extended(a, -1) * b;
					else
						result = a * ldexpf_extended(b, -1);
					if (result == copysignf(FLOAT_MAX_NORMAL, result))
					{
						CPU.FPSCR.setSinglePrecisionExceptionFlags(w, FPSCR_SOVF);
						result = extended(sign, 0x7FFFFF);
					}
					else
						result = ldexpf_extended(result, 1);
				}
			}
			else
			{
				result = a * b;
				if (result == copysignf(FLOAT_MAX_NORMAL, result))
				{
					feclearexcept(FE_ALL_EXCEPT);
					if (fexpf(a) > fexpf(b))
						result = ldexpf(a, -1) * b;
					else
						result = a * ldexpf(b, -1);
					result = ldexpf_extended(result, 1);
					if (isextended(result))
						CPU.FPSCR.setSinglePrecisionExceptionFlags(w, FPSCR_SDIFF);
					if (fetestexcept(FE_OVERFLOW))
						CPU.FPSCR.setSinglePrecisionExceptionFlags(w, FPSCR_SOVF);
				}
				else if (isdenormal(result))
				{
					CPU.FPSCR.setSinglePrecisionExceptionFlags(w, FPSCR_SUNF | FPSCR_SDIFF);
					result = +0.0f;
				}
				else if (result == 0.0f)
				{
					if (a != 0.0f & b != 0.0f)
						CPU.FPSCR.setSinglePrecisionExceptionFlags(w, FPSCR_SUNF | FPSCR_SDIFF);
					result = +0.0f;
				}
			}
			CPU.GPR[rt]._f[w] = result;
		}
	}
	void CLGTH(u32 rt, u32 ra, u32 rb)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = CPU.GPR[ra]._u16[h] > CPU.GPR[rb]._u16[h] ? 0xffff : 0;
	}
	void ORC(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._u32[w] | (~CPU.GPR[rb]._u32[w]);
	}
	void FCMGT(u32 rt, u32 ra, u32 rb)
	{
		for (int i = 0; i < 4; i++)
		{
			const u32 a = CPU.GPR[ra]._u32[i];
			const u32 b = CPU.GPR[rb]._u32[i];
			const u32 abs_a = a & 0x7FFFFFFF;
			const u32 abs_b = b & 0x7FFFFFFF;
			const bool a_zero = (abs_a < 0x00800000);
			const bool b_zero = (abs_b < 0x00800000);
			bool pass;
			if (a_zero)
				pass = false;
			else if (b_zero)
				pass = !a_zero;
			else
				pass = abs_a > abs_b;
			CPU.GPR[rt]._u32[i] = pass ? 0xFFFFFFFF : 0;
		}
	}
	void DFCMGT(u32 rt, u32 ra, u32 rb)
	{
		UNIMPLEMENTED(); // cannot be used
	}
	enum DoubleOp {DFASM_A, DFASM_S, DFASM_M};
	void DFASM(u32 rt, u32 ra, u32 rb, DoubleOp op)
	{
		for (int i = 0; i < 2; i++)
		{
			double a = CPU.GPR[ra]._d[i];
			double b = CPU.GPR[rb]._d[i];
			if (isdenormal(a))
			{
				CPU.FPSCR.setDoublePrecisionExceptionFlags(i, FPSCR_DDENORM);
				a = copysign(0.0, a);
			}
			if (isdenormal(b))
			{
				CPU.FPSCR.setDoublePrecisionExceptionFlags(i, FPSCR_DDENORM);
				b = copysign(0.0, b);
			}
			double result;
			if (isnan(a) || isnan(b))
			{
				CPU.FPSCR.setDoublePrecisionExceptionFlags(i, FPSCR_DNAN);
				if (issnan(a) || issnan(b))
					CPU.FPSCR.setDoublePrecisionExceptionFlags(i, FPSCR_DINV);
				result = DOUBLE_NAN;
			}
			else
			{
				SetHostRoundingMode(CPU.FPSCR.checkSliceRounding(i));
				feclearexcept(FE_ALL_EXCEPT);
				switch (op)
				{
				case DFASM_A: result = a + b; break;
				case DFASM_S: result = a - b; break;
				case DFASM_M: result = a * b; break;
				}
				if (fetestexcept(FE_INVALID))
				{
					CPU.FPSCR.setDoublePrecisionExceptionFlags(i, FPSCR_DINV);
					result = DOUBLE_NAN;
				}
				else
				{
					if (fetestexcept(FE_OVERFLOW))
						CPU.FPSCR.setDoublePrecisionExceptionFlags(i, FPSCR_DOVF);
					if (fetestexcept(FE_UNDERFLOW))
						CPU.FPSCR.setDoublePrecisionExceptionFlags(i, FPSCR_DUNF);
					if (fetestexcept(FE_INEXACT))
						CPU.FPSCR.setDoublePrecisionExceptionFlags(i, FPSCR_DINX);
				}
			}
			CPU.GPR[rt]._d[i] = result;
		}
	}
	void DFA(u32 rt, u32 ra, u32 rb) {DFASM(rt, ra, rb, DFASM_A);}
	void DFS(u32 rt, u32 ra, u32 rb) {DFASM(rt, ra, rb, DFASM_S);}
	void DFM(u32 rt, u32 ra, u32 rb) {DFASM(rt, ra, rb, DFASM_M);}
	void CLGTB(u32 rt, u32 ra, u32 rb)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b] > CPU.GPR[rb]._u8[b] ? 0xff : 0;
	}
	void HLGT(u32 rt, u32 ra, u32 rb)
	{
		if (CPU.GPR[ra]._u32[3] > CPU.GPR[rb]._u32[3])
		{
			CPU.SPU.Status.SetValue(SPU_STATUS_STOPPED_BY_HALT);
			CPU.Stop();
		}
	}
	void DFMA(u32 rt, u32 ra, u32 rb, bool neg, bool sub)
	{
		for (int i = 0; i < 2; i++)
		{
			double a = CPU.GPR[ra]._d[i];
			double b = CPU.GPR[rb]._d[i];
			double c = CPU.GPR[rt]._d[i];
			if (isdenormal(a))
			{
				CPU.FPSCR.setDoublePrecisionExceptionFlags(i, FPSCR_DDENORM);
				a = copysign(0.0, a);
			}
			if (isdenormal(b))
			{
				CPU.FPSCR.setDoublePrecisionExceptionFlags(i, FPSCR_DDENORM);
				b = copysign(0.0, b);
			}
			if (isdenormal(c))
			{
				CPU.FPSCR.setDoublePrecisionExceptionFlags(i, FPSCR_DDENORM);
				c = copysign(0.0, c);
			}
			double result;
			if (isnan(a) || isnan(b) || isnan(c))
			{
				CPU.FPSCR.setDoublePrecisionExceptionFlags(i, FPSCR_DNAN);
				if (issnan(a) || issnan(b) || issnan(c) || (isinf(a) && b == 0.0f) || (a == 0.0f && isinf(b)))
					CPU.FPSCR.setDoublePrecisionExceptionFlags(i, FPSCR_DINV);
				result = DOUBLE_NAN;
			}
			else
			{
				SetHostRoundingMode(CPU.FPSCR.checkSliceRounding(i));
				feclearexcept(FE_ALL_EXCEPT);
				result = fma(a, b, sub ? -c : c);
				if (fetestexcept(FE_INVALID))
				{
					CPU.FPSCR.setDoublePrecisionExceptionFlags(i, FPSCR_DINV);
					result = DOUBLE_NAN;
				}
				else
				{
					if (fetestexcept(FE_OVERFLOW))
						CPU.FPSCR.setDoublePrecisionExceptionFlags(i, FPSCR_DOVF);
					if (fetestexcept(FE_UNDERFLOW))
						CPU.FPSCR.setDoublePrecisionExceptionFlags(i, FPSCR_DUNF);
					if (fetestexcept(FE_INEXACT))
						CPU.FPSCR.setDoublePrecisionExceptionFlags(i, FPSCR_DINX);
					if (neg) result = -result;
				}
			}
			CPU.GPR[rt]._d[i] = result;
		}
	}
	void DFMA(u32 rt, u32 ra, u32 rb) {DFMA(rt, ra, rb, false, false);}
	void DFMS(u32 rt, u32 ra, u32 rb) {DFMA(rt, ra, rb, false, true);}
	void DFNMS(u32 rt, u32 ra, u32 rb) {DFMA(rt, ra, rb, true, true);}
	void DFNMA(u32 rt, u32 ra, u32 rb) {DFMA(rt, ra, rb, true, false);}
	void CEQ(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._s32[w] == CPU.GPR[rb]._s32[w] ? 0xffffffff : 0;
	}
	void MPYHHU(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._u16[w*2+1] * CPU.GPR[rb]._u16[w*2+1];
	}
	void ADDX(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._u32[w] + CPU.GPR[rb]._u32[w] + (CPU.GPR[rt]._u32[w] & 1);
	}
	void SFX(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[rb]._u32[w] - CPU.GPR[ra]._u32[w] - (1 - (CPU.GPR[rt]._u32[w] & 1));
	}
	void CGX(u32 rt, u32 ra, u32 rb)
	{
		// rarely used
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = ((u64)CPU.GPR[ra]._u32[w] + (u64)CPU.GPR[rb]._u32[w] + (u64)(CPU.GPR[rt]._u32[w] & 1)) >> 32;
	}
	void BGX(u32 rt, u32 ra, u32 rb)
	{
		// rarely used
		s64 nResult;
		
		for (int w = 0; w < 4; w++)
		{
			nResult = (u64)CPU.GPR[rb]._u32[w] - (u64)CPU.GPR[ra]._u32[w] - (u64)(1 - (CPU.GPR[rt]._u32[w] & 1));
			CPU.GPR[rt]._u32[w] = nResult < 0 ? 0 : 1;
		}
	}
	void MPYHHA(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._s32[w] += CPU.GPR[ra]._s16[w*2+1] * CPU.GPR[rb]._s16[w*2+1];
	}
	void MPYHHAU(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] += CPU.GPR[ra]._u16[w*2+1] * CPU.GPR[rb]._u16[w*2+1];
	}
	//Forced bits to 0, hence the shift:
	
	void FSCRRD(u32 rt)
	{
		CPU.GPR[rt]._u32[3] = CPU.FPSCR._u32[3];
		CPU.GPR[rt]._u32[2] = CPU.FPSCR._u32[2];
		CPU.GPR[rt]._u32[1] = CPU.FPSCR._u32[1];
		CPU.GPR[rt]._u32[0] = CPU.FPSCR._u32[0];
	}
	void FESD(u32 rt, u32 ra)
	{
		for (int i = 0; i < 2; i++)
		{
			const float a = CPU.GPR[ra]._f[i*2+1];
			if (isnan(a))
			{
				CPU.FPSCR.setDoublePrecisionExceptionFlags(i, FPSCR_DNAN);
				if (issnan(a))
					CPU.FPSCR.setDoublePrecisionExceptionFlags(i, FPSCR_DINV);
				CPU.GPR[rt]._d[i] = DOUBLE_NAN;
			}
			else if (isdenormal(a))
			{
				CPU.FPSCR.setDoublePrecisionExceptionFlags(i, FPSCR_DDENORM);
				CPU.GPR[rt]._d[i] = 0.0;
			}
			else
			{
				CPU.GPR[rt]._d[i] = (double)a;
			}
		}
	}
	void FRDS(u32 rt, u32 ra)
	{
		for (int i = 0; i < 2; i++)
		{
			SetHostRoundingMode(CPU.FPSCR.checkSliceRounding(i));
			const double a = CPU.GPR[ra]._d[i];
			if (isnan(a))
			{
				CPU.FPSCR.setDoublePrecisionExceptionFlags(i, FPSCR_DNAN);
				if (issnan(a))
					CPU.FPSCR.setDoublePrecisionExceptionFlags(i, FPSCR_DINV);
				CPU.GPR[rt]._f[i*2+1] = FLOAT_NAN;
			}
			else
			{
				feclearexcept(FE_ALL_EXCEPT);
				CPU.GPR[rt]._f[i*2+1] = (float)a;
				if (fetestexcept(FE_OVERFLOW))
					CPU.FPSCR.setDoublePrecisionExceptionFlags(i, FPSCR_DOVF);
				if (fetestexcept(FE_UNDERFLOW))
					CPU.FPSCR.setDoublePrecisionExceptionFlags(i, FPSCR_DUNF);
				if (fetestexcept(FE_INEXACT))
					CPU.FPSCR.setDoublePrecisionExceptionFlags(i, FPSCR_DINX);
			}
			CPU.GPR[rt]._u32[i*2] = 0;
		}
	}
	void FSCRWR(u32 rt, u32 ra)
	{
		CPU.FPSCR._u32[3] = CPU.GPR[ra]._u32[3] & 0x00000F07;
		CPU.FPSCR._u32[2] = CPU.GPR[ra]._u32[2] & 0x00003F07;
		CPU.FPSCR._u32[1] = CPU.GPR[ra]._u32[1] & 0x00003F07;
		CPU.FPSCR._u32[0] = CPU.GPR[ra]._u32[0] & 0x00000F07;
	}
	void DFTSV(u32 rt, u32 ra, s32 i7)
	{
		UNIMPLEMENTED(); // cannot be used
	}
	void FCEQ(u32 rt, u32 ra, u32 rb)
	{
		for (int i = 0; i < 4; i++)
		{
			const u32 a = CPU.GPR[ra]._u32[i];
			const u32 b = CPU.GPR[rb]._u32[i];
			const u32 abs_a = a & 0x7FFFFFFF;
			const u32 abs_b = b & 0x7FFFFFFF;
			const bool a_zero = (abs_a < 0x00800000);
			const bool b_zero = (abs_b < 0x00800000);
			const bool pass = a == b || (a_zero && b_zero);
			CPU.GPR[rt]._u32[i] = pass ? 0xFFFFFFFF : 0;
		}
	}
	void DFCEQ(u32 rt, u32 ra, u32 rb)
	{
		UNIMPLEMENTED(); // cannot be used
	}
	void MPY(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._s32[w] = CPU.GPR[ra]._s16[w*2] * CPU.GPR[rb]._s16[w*2];
	}
	void MPYH(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._s32[w] = (CPU.GPR[ra]._s16[w*2+1] * CPU.GPR[rb]._s16[w*2]) << 16;
	}
	void MPYHH(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._s32[w] = CPU.GPR[ra]._s16[w*2+1] * CPU.GPR[rb]._s16[w*2+1];
	}
	void MPYS(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._s32[w] = (CPU.GPR[ra]._s16[w*2] * CPU.GPR[rb]._s16[w*2]) >> 16;
	}
	void CEQH(u32 rt, u32 ra, u32 rb)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = CPU.GPR[ra]._u16[h] == CPU.GPR[rb]._u16[h] ? 0xffff : 0;
	}
	void FCMEQ(u32 rt, u32 ra, u32 rb)
	{
		for (int i = 0; i < 4; i++)
		{
			const u32 a = CPU.GPR[ra]._u32[i];
			const u32 b = CPU.GPR[rb]._u32[i];
			const u32 abs_a = a & 0x7FFFFFFF;
			const u32 abs_b = b & 0x7FFFFFFF;
			const bool a_zero = (abs_a < 0x00800000);
			const bool b_zero = (abs_b < 0x00800000);
			const bool pass = abs_a == abs_b || (a_zero && b_zero);
			CPU.GPR[rt]._u32[i] = pass ? 0xFFFFFFFF : 0;
		}
	}
	void DFCMEQ(u32 rt, u32 ra, u32 rb)
	{
		UNIMPLEMENTED(); // cannot be used
	}
	void MPYU(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._u16[w*2] * CPU.GPR[rb]._u16[w*2];
	}
	void CEQB(u32 rt, u32 ra, u32 rb)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b] == CPU.GPR[rb]._u8[b] ? 0xff : 0;
	}
	void FI(u32 rt, u32 ra, u32 rb)
	{
		// TODO: Floating Interpolation: ra will be ignored.
		// It should work correctly if result of preceding FREST or FRSQEST is sufficiently exact
		CPU.GPR[rt] = CPU.GPR[rb];
	}
	void HEQ(u32 rt, u32 ra, u32 rb)
	{
		if (CPU.GPR[ra]._s32[3] == CPU.GPR[rb]._s32[3])
		{
			CPU.SPU.Status.SetValue(SPU_STATUS_STOPPED_BY_HALT);
			CPU.Stop();
		}
	}

	//0 - 9
	void CFLTS(u32 rt, u32 ra, s32 i8)
	{
		const int scale = 173 - (i8 & 0xff); //unsigned immediate
		for (int i = 0; i < 4; i++)
		{
			const float a = CPU.GPR[ra]._f[i];
			float scaled;
			if ((fexpf(a)-127) + scale >= 32)
				scaled = copysignf(4294967296.0f, a);
			else
				scaled = ldexpf(a, scale);
			s32 result;
			if (scaled >= 2147483648.0f)
				result = 0x7FFFFFFF;
			else if (scaled < -2147483648.0f)
				result = 0x80000000;
			else
				result = (s32)scaled;
			CPU.GPR[rt]._s32[i] = result;
		}
	}
	void CFLTU(u32 rt, u32 ra, s32 i8)
	{
		const int scale = 173 - (i8 & 0xff); //unsigned immediate
		for (int i = 0; i < 4; i++)
		{
			const float a = CPU.GPR[ra]._f[i];
			float scaled;
			if ((fexpf(a)-127) + scale >= 32)
				scaled = copysignf(4294967296.0f, a);
			else
				scaled = ldexpf(a, scale);
			u32 result;
			if (scaled >= 4294967296.0f)
				result = 0xFFFFFFFF;
			else if (scaled < 0.0f)
				result = 0;
			else
				result = (u32)scaled;
			CPU.GPR[rt]._u32[i] = result;
		}
	}
	void CSFLT(u32 rt, u32 ra, s32 i8)
	{
		SetHostRoundingMode(FPSCR_RN_ZERO);
		const int scale = 155 - (i8 & 0xff); //unsigned immediate
		for (int i = 0; i < 4; i++)
		{
			const s32 a = CPU.GPR[ra]._s32[i];
			CPU.GPR[rt]._f[i] = (float)a;

			u32 exp = ((CPU.GPR[rt]._u32[i] >> 23) & 0xff) - scale;

			if (exp > 255) //< 0
				exp = 0;

			CPU.GPR[rt]._u32[i] = (CPU.GPR[rt]._u32[i] & 0x807fffff) | (exp << 23);
			if (isdenormal(CPU.GPR[rt]._f[i]) || (CPU.GPR[rt]._f[i] == 0.0f && a != 0))
			{
				CPU.FPSCR.setSinglePrecisionExceptionFlags(i, FPSCR_SUNF | FPSCR_SDIFF);
				CPU.GPR[rt]._f[i] = 0.0f;
			}
		}
	}
	void CUFLT(u32 rt, u32 ra, s32 i8)
	{
		SetHostRoundingMode(FPSCR_RN_ZERO);
		const int scale = 155 - (i8 & 0xff); //unsigned immediate
		for (int i = 0; i < 4; i++)
		{
			const u32 a = CPU.GPR[ra]._u32[i];
			CPU.GPR[rt]._f[i] = (float)a;

			u32 exp = ((CPU.GPR[rt]._u32[i] >> 23) & 0xff) - scale;

			if (exp > 255) //< 0
				exp = 0;

			CPU.GPR[rt]._u32[i] = (CPU.GPR[rt]._u32[i] & 0x807fffff) | (exp << 23);
			if (isdenormal(CPU.GPR[rt]._f[i]) || (CPU.GPR[rt]._f[i] == 0.0f && a != 0))
			{
				CPU.FPSCR.setSinglePrecisionExceptionFlags(i, FPSCR_SUNF | FPSCR_SDIFF);
				CPU.GPR[rt]._f[i] = 0.0f;
			}
		}
	}

	//0 - 8
	void BRZ(u32 rt, s32 i16)
	{
		u32 target = branchTarget(CPU.PC, i16);
		if (CPU.GPR[rt]._u32[3] == 0)
		{
			LOG5_OPCODE("taken (0x%x)", target);
			CPU.SetBranch(target);
		}
		else
		{
			LOG5_OPCODE("not taken (0x%x)", target);
		}
	}
	void STQA(u32 rt, s32 i16)
	{
		u32 lsa = (i16 << 2) & 0x3fff0;

		CPU.WriteLS128(lsa, CPU.GPR[rt]);
	}
	void BRNZ(u32 rt, s32 i16)
	{
		u32 target = branchTarget(CPU.PC, i16);
		if (CPU.GPR[rt]._u32[3] != 0)
		{
			LOG5_OPCODE("taken (0x%x)", target);
			CPU.SetBranch(target);
		}
		else
		{
			LOG5_OPCODE("not taken (0x%x)", target);
		}
	}
	void BRHZ(u32 rt, s32 i16)
	{
		u32 target = branchTarget(CPU.PC, i16);
		if (CPU.GPR[rt]._u16[6] == 0)
		{
			LOG5_OPCODE("taken (0x%x)", target);
			CPU.SetBranch(target);
		}
		else
		{
			LOG5_OPCODE("not taken (0x%x)", target);
		}
	}
	void BRHNZ(u32 rt, s32 i16)
	{
		u32 target = branchTarget(CPU.PC, i16);
		if (CPU.GPR[rt]._u16[6] != 0)
		{
			LOG5_OPCODE("taken (0x%x)", target);
			CPU.SetBranch(target);
		}
		else
		{
			LOG5_OPCODE("not taken (0x%x)", target);
		}
	}
	void STQR(u32 rt, s32 i16)
	{
		u32 lsa = branchTarget(CPU.PC, i16) & 0x3fff0; 

		CPU.WriteLS128(lsa, CPU.GPR[rt]);
	}
	void BRA(s32 i16)
	{
		u32 target = branchTarget(0, i16);
		LOG5_OPCODE("branch (0x%x)", target);
		CPU.SetBranch(target);
	}
	void LQA(u32 rt, s32 i16)
	{
		u32 lsa = (i16 << 2) & 0x3fff0;

		CPU.GPR[rt] = CPU.ReadLS128(lsa);
	}
	void BRASL(u32 rt, s32 i16)
	{
		u32 target = branchTarget(0, i16);
		CPU.GPR[rt].clear();
		CPU.GPR[rt]._u32[3] = CPU.PC + 4;
		LOG5_OPCODE("branch (0x%x)", target);
		CPU.SetBranch(target);
	}
	void BR(s32 i16)
	{
		u32 target = branchTarget(CPU.PC, i16);
		LOG5_OPCODE("branch (0x%x)", target);
		CPU.SetBranch(target);
	}
	void FSMBI(u32 rt, s32 i16)
	{
		const u32 s = i16;

		for(u32 j = 0; j < 16; ++j)
		{
			if((s >> j) & 0x1)
			{
				CPU.GPR[rt]._u8[j] = 0xFF;
			}
			else
			{
				CPU.GPR[rt]._u8[j] = 0x00;
			}
		}
	}
	void BRSL(u32 rt, s32 i16)
	{
		u32 target = branchTarget(CPU.PC, i16);
		CPU.GPR[rt].clear();
		CPU.GPR[rt]._u32[3] = CPU.PC + 4;
		LOG5_OPCODE("branch (0x%x)", target);
		CPU.SetBranch(target);
	}
	void LQR(u32 rt, s32 i16)
	{
		u32 lsa = branchTarget(CPU.PC, i16) & 0x3fff0;

		CPU.GPR[rt] = CPU.ReadLS128(lsa);
	}
	void IL(u32 rt, s32 i16)
	{
		CPU.GPR[rt]._s32[0] =
			CPU.GPR[rt]._s32[1] =
			CPU.GPR[rt]._s32[2] =
			CPU.GPR[rt]._s32[3] = i16;
	}
	void ILHU(u32 rt, s32 i16)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._s32[w] = i16 << 16;
	}
	void ILH(u32 rt, s32 i16)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._s16[h] = i16;
	}
	void IOHL(u32 rt, s32 i16)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._s32[w] |= (i16 & 0xFFFF);
	}
	

	//0 - 7
	void ORI(u32 rt, u32 ra, s32 i10)
	{
		for(u32 i = 0; i < 4; ++i)
			CPU.GPR[rt]._s32[i] = CPU.GPR[ra]._s32[i] | i10;
	}
	void ORHI(u32 rt, u32 ra, s32 i10)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._s16[h] = CPU.GPR[ra]._s16[h] | i10;
	}
	void ORBI(u32 rt, u32 ra, s32 i10)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._s8[b] = CPU.GPR[ra]._s8[b] | i10;
	}
	void SFI(u32 rt, u32 ra, s32 i10)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._s32[w] = i10 - CPU.GPR[ra]._s32[w];
	}
	void SFHI(u32 rt, u32 ra, s32 i10)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._s16[h] = i10 - CPU.GPR[ra]._s16[h];
	}
	void ANDI(u32 rt, u32 ra, s32 i10)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._s32[w] = CPU.GPR[ra]._s32[w] & i10;
	}
	void ANDHI(u32 rt, u32 ra, s32 i10)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._s16[h] = CPU.GPR[ra]._s16[h] & i10;
	}
	void ANDBI(u32 rt, u32 ra, s32 i10)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._s8[b] = CPU.GPR[ra]._s8[b] & i10;
	}
	void AI(u32 rt, u32 ra, s32 i10)
	{
		CPU.GPR[rt]._s32[0] = CPU.GPR[ra]._s32[0] + i10;
		CPU.GPR[rt]._s32[1] = CPU.GPR[ra]._s32[1] + i10;
		CPU.GPR[rt]._s32[2] = CPU.GPR[ra]._s32[2] + i10;
		CPU.GPR[rt]._s32[3] = CPU.GPR[ra]._s32[3] + i10;
	}
	void AHI(u32 rt, u32 ra, s32 i10)
	{
		for(u32 h = 0; h < 8; ++h)
			CPU.GPR[rt]._s16[h] = CPU.GPR[ra]._s16[h] + i10;
	}
	void STQD(u32 rt, s32 i10, u32 ra) //i10 is shifted left by 4 while decoding
	{
		const u32 lsa = (CPU.GPR[ra]._s32[3] + i10) & 0x3fff0;

		CPU.WriteLS128(lsa, CPU.GPR[rt]);
	}
	void LQD(u32 rt, s32 i10, u32 ra) //i10 is shifted left by 4 while decoding
	{
		const u32 lsa = (CPU.GPR[ra]._s32[3] + i10) & 0x3fff0;

		CPU.GPR[rt] = CPU.ReadLS128(lsa);
	}
	void XORI(u32 rt, u32 ra, s32 i10)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._s32[w] = CPU.GPR[ra]._s32[w] ^ i10;
	}
	void XORHI(u32 rt, u32 ra, s32 i10)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._s16[h] = CPU.GPR[ra]._s16[h] ^ i10;
	}
	void XORBI(u32 rt, u32 ra, s32 i10)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._s8[b] = CPU.GPR[ra]._s8[b] ^ i10;
	}
	void CGTI(u32 rt, u32 ra, s32 i10)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._s32[w] > i10 ? 0xffffffff : 0;
	}
	void CGTHI(u32 rt, u32 ra, s32 i10)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = CPU.GPR[ra]._s16[h] > i10 ? 0xffff : 0;
	}
	void CGTBI(u32 rt, u32 ra, s32 i10)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._s8[b] > (s8)(i10 & 0xff) ? 0xff : 0;
	}
	void HGTI(u32 rt, u32 ra, s32 i10)
	{
		if (CPU.GPR[ra]._s32[3] > i10)
		{
			CPU.SPU.Status.SetValue(SPU_STATUS_STOPPED_BY_HALT);
			CPU.Stop();
		}
	}
	void CLGTI(u32 rt, u32 ra, s32 i10)
	{
		for(u32 i = 0; i < 4; ++i)
		{
			CPU.GPR[rt]._u32[i] = (CPU.GPR[ra]._u32[i] > (u32)i10) ? 0xffffffff : 0x00000000;
		}
	}
	void CLGTHI(u32 rt, u32 ra, s32 i10)
	{
		for(u32 i = 0; i < 8; ++i)
		{
			CPU.GPR[rt]._u16[i] = (CPU.GPR[ra]._u16[i] > (u16)i10) ? 0xffff : 0x0000;
		}
	}
	void CLGTBI(u32 rt, u32 ra, s32 i10)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b] > (u8)(i10 & 0xff) ? 0xff : 0;
	}
	void HLGTI(u32 rt, u32 ra, s32 i10)
	{
		if (CPU.GPR[ra]._u32[3] > (u32)i10)
		{
			CPU.SPU.Status.SetValue(SPU_STATUS_STOPPED_BY_HALT);
			CPU.Stop();
		}
	}
	void MPYI(u32 rt, u32 ra, s32 i10)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._s32[w] = CPU.GPR[ra]._s16[w*2] * i10;
	}
	void MPYUI(u32 rt, u32 ra, s32 i10)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._u16[w*2] * (u16)(i10 & 0xffff);
	}
	void CEQI(u32 rt, u32 ra, s32 i10)
	{
		for(u32 i = 0; i < 4; ++i)
			CPU.GPR[rt]._u32[i] = (CPU.GPR[ra]._s32[i] == i10) ? 0xffffffff : 0x00000000;
	}
	void CEQHI(u32 rt, u32 ra, s32 i10)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = (CPU.GPR[ra]._s16[h] == (s16)i10) ? 0xffff : 0;
	}
	void CEQBI(u32 rt, u32 ra, s32 i10)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._s8[b] = (CPU.GPR[ra]._s8[b] == (s8)(i10 & 0xff)) ? 0xff : 0;
	}
	void HEQI(u32 rt, u32 ra, s32 i10)
	{
		if (CPU.GPR[ra]._s32[3] == i10)
		{
			CPU.SPU.Status.SetValue(SPU_STATUS_STOPPED_BY_HALT);
			CPU.Stop();
		}
	}


	//0 - 6
	void HBRA(s32 ro, s32 i16)
	{ //i16 is shifted left by 2 while decoding
	}
	void HBRR(s32 ro, s32 i16)
	{
	}
	void ILA(u32 rt, u32 i18)
	{
		CPU.GPR[rt]._u32[0] = 
			CPU.GPR[rt]._u32[1] = 
			CPU.GPR[rt]._u32[2] = 
			CPU.GPR[rt]._u32[3] = i18 & 0x3FFFF;
	}

	//0 - 3
	void SELB(u32 rt, u32 ra, u32 rb, u32 rc)
	{
		for(u64 i = 0; i < 2; ++i)
		{
			CPU.GPR[rt]._u64[i] =
				( CPU.GPR[rc]._u64[i] & CPU.GPR[rb]._u64[i]) |
				(~CPU.GPR[rc]._u64[i] & CPU.GPR[ra]._u64[i]);
		}
	}
	void SHUFB(u32 rt, u32 ra, u32 rb, u32 rc)
	{
		const u128 _a = CPU.GPR[ra];
		const u128 _b = CPU.GPR[rb];
		for (int i = 0; i < 16; i++)
		{
			u8 b = CPU.GPR[rc]._u8[i];
			if(b & 0x80)
			{
				if(b & 0x40)
				{
					if(b & 0x20)
						CPU.GPR[rt]._u8[i] = 0x80;
					else
						CPU.GPR[rt]._u8[i] = 0xFF;
				}
				else
					CPU.GPR[rt]._u8[i] = 0x00;
			}
			else
			{
				if(b & 0x10)
					CPU.GPR[rt]._u8[i] = _b._u8[15 - (b & 0x0F)];
				else
					CPU.GPR[rt]._u8[i] = _a._u8[15 - (b & 0x0F)];
			}
		}
	}
	void MPYA(u32 rt, u32 ra, u32 rb, u32 rc)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._s32[w] = CPU.GPR[ra]._s16[w*2] * CPU.GPR[rb]._s16[w*2] + CPU.GPR[rc]._s32[w];
	}
	void FNMS(u32 rt, u32 ra, u32 rb, u32 rc) {FMA(rt, ra, rb, rc, true, true);}
	void FMA(u32 rt, u32 ra, u32 rb, u32 rc) {FMA(rt, ra, rb, rc, false, false);}
	void FMS(u32 rt, u32 ra, u32 rb, u32 rc) {FMA(rt, ra, rb, rc, false, true);}
	void FMA(u32 rt, u32 ra, u32 rb, u32 rc, bool neg, bool sub)
	{
		SetHostRoundingMode(FPSCR_RN_ZERO);
		for (int w = 0; w < 4; w++)
		{
			float a = CPU.GPR[ra]._f[w];
			float b = neg ? -CPU.GPR[rb]._f[w] : CPU.GPR[rb]._f[w];
			float c = (neg != sub) ? -CPU.GPR[rc]._f[w] : CPU.GPR[rc]._f[w];
			if (isdenormal(a))
			{
				CPU.FPSCR.setSinglePrecisionExceptionFlags(w, FPSCR_SDIFF);
				a = 0.0f;
			}
			if (isdenormal(b))
			{
				CPU.FPSCR.setSinglePrecisionExceptionFlags(w, FPSCR_SDIFF);
				b = 0.0f;
			}
			if (isdenormal(c))
			{
				CPU.FPSCR.setSinglePrecisionExceptionFlags(w, FPSCR_SDIFF);
				c = 0.0f;
			}

			const bool sign = std::signbit(a) ^ std::signbit(b);
			float result;

			if (isextended(a) || isextended(b))
			{
				CPU.FPSCR.setSinglePrecisionExceptionFlags(w, FPSCR_SDIFF);
				if (a == 0.0f || b == 0.0f)
				{
					result = c;
				}
				else if ((fexpf(a)-127) + (fexpf(b)-127) >= 130)
				{
					CPU.FPSCR.setSinglePrecisionExceptionFlags(w, FPSCR_SOVF);
					result = extended(sign, 0x7FFFFF);
				}
				else
				{
					float new_a, new_b;
					if (isextended(a))
					{
						new_a = ldexpf_extended(a, -2);
						new_b = b;
					}
					else
					{
						new_a = a;
						new_b = ldexpf_extended(b, -2);
					}
					if (fexpf(c) < 3)
					{
						result = new_a * new_b;
						if (c != 0.0f && std::signbit(c) != sign)
						{
							u32 bits = (u32&)result - 1;
							result = (float&)bits;
						}
					}
					else
					{
						result = fmaf(new_a, new_b, ldexpf_extended(c, -2));
					}
					if (fabsf(result) >= ldexpf(1.0f, 127))
					{
						CPU.FPSCR.setSinglePrecisionExceptionFlags(w, FPSCR_SOVF);
						result = extended(sign, 0x7FFFFF);
					}
					else
					{
						result = ldexpf_extended(result, 2);
					}
				}
			}
			else if (isextended(c))
			{
				CPU.FPSCR.setSinglePrecisionExceptionFlags(w, FPSCR_SDIFF);
				if (a == 0.0f || b == 0.0f)
				{
					result = c;
				}
				else if ((fexpf(a)-127) + (fexpf(b)-127) < 96)
				{
					result = c;
					if (sign != std::signbit(c))
					{
					    u32 bits = (u32&)result - 1;
					    result = (float&)bits;
					}
				}
				else
				{
					result = fmaf(ldexpf(a,-1), ldexpf(b,-1), ldexpf_extended(c,-2));
					if (fabsf(result) >= ldexpf(1.0f, 127))
					{
						CPU.FPSCR.setSinglePrecisionExceptionFlags(w, FPSCR_SOVF);
						result = extended(sign, 0x7FFFFF);
					}
					else
					{
						result = ldexpf_extended(result, 2);
					}
				}
			}
			else
			{
				feclearexcept(FE_ALL_EXCEPT);
				result = fmaf(a, b, c);
				if (fetestexcept(FE_OVERFLOW))
				{
					CPU.FPSCR.setSinglePrecisionExceptionFlags(w, FPSCR_SDIFF);
					if (fexpf(a) > fexpf(b))
						result = fmaf(ldexpf(a,-2), b, ldexpf(c,-2));
					else
						result = fmaf(a, ldexpf(b,-2), ldexpf(c,-2));
					if (fabsf(result) >= ldexpf(1.0f, 127))
					{
						CPU.FPSCR.setSinglePrecisionExceptionFlags(w, FPSCR_SOVF);
						result = extended(sign, 0x7FFFFF);
					}
					else
					{
						result = ldexpf_extended(result, 2);
					}
				}
				else if (fetestexcept(FE_UNDERFLOW))
				{
					CPU.FPSCR.setSinglePrecisionExceptionFlags(w, FPSCR_SUNF | FPSCR_SDIFF);
				}
			}
			if (isdenormal(result))
			{
				CPU.FPSCR.setSinglePrecisionExceptionFlags(w, FPSCR_SUNF | FPSCR_SDIFF);
				result = 0.0f;
			}
			else if (result == 0.0f)
			{
				result = +0.0f;
			}
			CPU.GPR[rt]._f[w] = result;
		}
	}

	void UNK(u32 code, u32 opcode, u32 gcode)
	{
		UNK(fmt::Format("Unknown/Illegal opcode! (0x%08x, 0x%x, 0x%x)", code, opcode, gcode));
	}

	void UNK(const std::string& err)
	{
		LOG_ERROR(Log::SPU, "%s #pc: 0x%x", err.c_str(), CPU.PC);
		Emu.Pause();
		for(uint i=0; i<128; ++i) LOG_NOTICE(Log::SPU, "r%d = 0x%s", i, CPU.GPR[i].to_hex().c_str());
	}
};
