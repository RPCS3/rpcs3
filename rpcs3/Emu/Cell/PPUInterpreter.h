#pragma once

#include "Emu/Cell/PPUOpcodes.h"
#include "Emu/Memory/Memory.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/SysCalls/SysCalls.h"
#include "rpcs3.h"
#include <stdint.h>
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#define _rotl64(x,r) (((u64)x << r) | ((u64)x >> (64 - r)))
#endif

#define UNIMPLEMENTED() UNK(__FUNCTION__)

#if 0//def _DEBUG
#define HLE_CALL_DEBUG
#endif

static u64 rotate_mask[64][64];
void InitRotateMask()
{
	static bool inited = false;
	if(inited) return;

	for(u32 mb=0; mb<64; mb++) for(u32 me=0; me<64; me++)
	{
		const u64 mask = ((u64)-1 >> mb) ^ ((me >= 63) ? 0 : (u64)-1 >> (me + 1));	
		rotate_mask[mb][me] = mb > me ? ~mask : mask;
	}

	inited = true;
}

u8 rotl8(const u8 x, const u8 n) { return (x << n) | (x >> (8 - n)); }
u8 rotr8(const u8 x, const u8 n) { return (x >> n) | (x << (8 - n)); }

u16 rotl16(const u16 x, const u8 n) { return (x << n) | (x >> (16 - n)); }
u16 rotr16(const u16 x, const u8 n) { return (x >> n) | (x << (16 - n)); }
/*
u32 rotl32(const u32 x, const u8 n) { return (x << n) | (x >> (32 - n)); }
u32 rotr32(const u32 x, const u8 n) { return (x >> n) | (x << (32 - n)); }
u64 rotl64(const u64 x, const u8 n) { return (x << n) | (x >> (64 - n)); }
u64 rotr64(const u64 x, const u8 n) { return (x >> n) | (x << (64 - n)); }
*/

#define rotl32(x, n) _rotl64((u64)(u32)x | ((u64)(u32)x << 32), n)
#define rotr32(x, n) _rotr64((u64)(u32)x | ((u64)(u32)x << 32), n)
#define rotl64 _rotl64
#define rotr64 _rotr64

class PPUInterpreter : public PPUOpcodes
{
private:
	PPUThread& CPU;

public:
	PPUInterpreter(PPUThread& cpu) : CPU(cpu)
	{
		InitRotateMask();
	}

private:
	void Exit() {}

	void SysCall()
	{
		SysCalls::DoSyscall(CPU.GPR[11]);

		if(Ini.HLELogging.GetValue())
		{
			ConLog.Warning("SysCall[0x%llx] done with code [0x%llx]! #pc: 0x%llx", CPU.GPR[11], CPU.GPR[3], CPU.PC);
			if(CPU.GPR[11] > 1024)
				SysCalls::DoFunc(CPU.GPR[11]);
		}
		/*else if ((s64)CPU.GPR[3] < 0) // probably, error code
		{
			ConLog.Error("SysCall[0x%llx] done with code [0x%llx]! #pc: 0x%llx", CPU.GPR[11], CPU.GPR[3], CPU.PC);
			if(CPU.GPR[11] > 1024)
				SysCalls::DoFunc(CPU.GPR[11]);
		}*/
#ifdef HLE_CALL_DEBUG
		ConLog.Write("SysCall[%lld] done with code [0x%llx]! #pc: 0x%llx", CPU.GPR[11], CPU.GPR[3], CPU.PC);
#endif
	}

	void NULL_OP()
	{
		UNK("null");
	}

	void NOP()
	{
		//__asm nop
	}

	float CheckVSCR_NJ(const float v) const
	{
		if(!CPU.VSCR.NJ) return v;

		const int fpc = _fpclass(v);
#ifdef __GNUG__
		if(fpc == FP_SUBNORMAL)
			return std::signbit(v) ? -0.0f : 0.0f;
#else
		if(fpc & _FPCLASS_ND) return -0.0f;
		if(fpc & _FPCLASS_PD) return  0.0f;
#endif

		return v;
	}

	bool CheckCondition(u32 bo, u32 bi)
	{
		const u8 bo0 = (bo & 0x10) ? 1 : 0;
		const u8 bo1 = (bo & 0x08) ? 1 : 0;
		const u8 bo2 = (bo & 0x04) ? 1 : 0;
		const u8 bo3 = (bo & 0x02) ? 1 : 0;

		if(!bo2) --CPU.CTR;

		const u8 ctr_ok = bo2 | ((CPU.CTR != 0) ^ bo3);
		const u8 cond_ok = bo0 | (CPU.IsCR(bi) ^ (~bo1 & 0x1));

		return ctr_ok && cond_ok;
	}

	u64& GetRegBySPR(u32 spr)
	{
		const u32 n = (spr >> 5) | ((spr & 0x1f) << 5);

		switch(n)
		{
		case 0x001: return CPU.XER.XER;
		case 0x008: return CPU.LR;
		case 0x009: return CPU.CTR;
		case 0x100: return CPU.USPRG0;
		}

		UNK(fmt::Format("GetRegBySPR error: Unknown SPR 0x%x!", n));
		return CPU.XER.XER;
	}
	
	void TDI(u32 to, u32 ra, s32 simm16)
	{
		s64 a = CPU.GPR[ra];

		if( (a < (s64)simm16  && (to & 0x10)) ||
			(a > (s64)simm16  && (to & 0x8))  ||
			(a == (s64)simm16 && (to & 0x4))  ||
			((u64)a < (u64)simm16 && (to & 0x2)) ||
			((u64)a > (u64)simm16 && (to & 0x1)) )
		{
			UNK(fmt::Format("Trap! (tdi %x, r%d, %x)", to, ra, simm16));
		}
	}

	void TWI(u32 to, u32 ra, s32 simm16)
	{
		s32 a = CPU.GPR[ra];

		if( (a < simm16  && (to & 0x10)) ||
			(a > simm16  && (to & 0x8))  ||
			(a == simm16 && (to & 0x4))  ||
			((u32)a < (u32)simm16 && (to & 0x2)) ||
			((u32)a > (u32)simm16 && (to & 0x1)) )
		{
			UNK(fmt::Format("Trap! (twi %x, r%d, %x)", to, ra, simm16));
		}
	}

	void MFVSCR(u32 vd)
	{
		CPU.VPR[vd].Clear();
		CPU.VPR[vd]._u32[0] = CPU.VSCR.VSCR;
	}
	void MTVSCR(u32 vb)
	{
		CPU.VSCR.VSCR = CPU.VPR[vb]._u32[0];
		CPU.VSCR.X = CPU.VSCR.Y = 0;
	}
	void VADDCUW(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u32[w] = ~CPU.VPR[va]._u32[w] < CPU.VPR[vb]._u32[w];
		}
	}
	void VADDFP(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._f[w] = CPU.VPR[va]._f[w] + CPU.VPR[vb]._f[w];
		}
	}
	void VADDSBS(u32 vd, u32 va, u32 vb)
	{
		for(u32 b=0; b<16; ++b)
		{
			s16 result = (s16)CPU.VPR[va]._s8[b] + (s16)CPU.VPR[vb]._s8[b];

			if (result > 0x7f)
			{
				CPU.VPR[vd]._s8[b] = 0x7f;
				CPU.VSCR.SAT = 1;
			}
			else if (result < -0x80)
			{
				CPU.VPR[vd]._s8[b] = -0x80;
				CPU.VSCR.SAT = 1;
			}
			else
				CPU.VPR[vd]._s8[b] = result;
		}
	}
	void VADDSHS(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 8; h++)
		{
			s32 result = (s32)CPU.VPR[va]._s16[h] + (s32)CPU.VPR[vb]._s16[h];

			if (result > 0x7fff)
			{
				CPU.VPR[vd]._s16[h] = 0x7fff;
				CPU.VSCR.SAT = 1;
			}
			else if (result < -0x8000)
			{
				CPU.VPR[vd]._s16[h] = -0x8000;
				CPU.VSCR.SAT = 1;
			}
			else
				CPU.VPR[vd]._s16[h] = result;
		}
	}
	void VADDSWS(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			s64 result = (s64)CPU.VPR[va]._s32[w] + (s64)CPU.VPR[vb]._s32[w];

			if (result > 0x7fffffff)
			{
				CPU.VPR[vd]._s32[w] = 0x7fffffff;
				CPU.VSCR.SAT = 1;
			}
			else if (result < (s32)0x80000000)
			{
				CPU.VPR[vd]._s32[w] = 0x80000000;
				CPU.VSCR.SAT = 1;
			}
			else
				CPU.VPR[vd]._s32[w] = result;
		}
	}
	void VADDUBM(u32 vd, u32 va, u32 vb)
	{
		for (uint b = 0; b < 16; b++)
		{
			CPU.VPR[vd]._u8[b] = CPU.VPR[va]._u8[b] + CPU.VPR[vb]._u8[b];
		}
	}
	void VADDUBS(u32 vd, u32 va, u32 vb)
	{
		for (uint b = 0; b < 16; b++)
		{
			u16 result = (u16)CPU.VPR[va]._u8[b] + (u16)CPU.VPR[vb]._u8[b];

			if (result > 0xff)
			{
				CPU.VPR[vd]._u8[b] = 0xff;
				CPU.VSCR.SAT = 1;
			}
			else
				CPU.VPR[vd]._u8[b] = result;
		}
	}
	void VADDUHM(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._u16[h] = CPU.VPR[va]._u16[h] + CPU.VPR[vb]._u16[h];
		}
	}
	void VADDUHS(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 8; h++)
		{
			u32 result = (u32)CPU.VPR[va]._u16[h] + (u32)CPU.VPR[vb]._u16[h];

			if (result > 0xffff)
			{
				CPU.VPR[vd]._u16[h] = 0xffff;
				CPU.VSCR.SAT = 1;
			}
			else
				CPU.VPR[vd]._u16[h] = result;
		}
	}
	void VADDUWM(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u32[w] = CPU.VPR[va]._u32[w] + CPU.VPR[vb]._u32[w];
		}
	}
	void VADDUWS(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			u64 result = (u64)CPU.VPR[va]._u32[w] + (u64)CPU.VPR[vb]._u32[w];

			if (result > 0xffffffff)
			{
				CPU.VPR[vd]._u32[w] = 0xffffffff;
				CPU.VSCR.SAT = 1;
			}
			else
				CPU.VPR[vd]._u32[w] = result;
		}
	}
	void VAND(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u32[w] = CPU.VPR[va]._u32[w] & CPU.VPR[vb]._u32[w];
		}
	}
	void VANDC(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u32[w] = CPU.VPR[va]._u32[w] & (~CPU.VPR[vb]._u32[w]);
		}
	}
	void VAVGSB(u32 vd, u32 va, u32 vb)
	{
		for (uint b = 0; b < 16; b++)
		{
			CPU.VPR[vd]._s8[b] = (CPU.VPR[va]._s8[b] + CPU.VPR[vb]._s8[b] + 1) >> 1;
		}
	}
	void VAVGSH(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._s16[h] = (CPU.VPR[va]._s16[h] + CPU.VPR[vb]._s16[h] + 1) >> 1;
		}
	}
	void VAVGSW(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._s32[w] = ((s64)CPU.VPR[va]._s32[w] + (s64)CPU.VPR[vb]._s32[w] + 1) >> 1;
		}
	}
	void VAVGUB(u32 vd, u32 va, u32 vb)
	{
		for (uint b = 0; b < 16; b++)
			CPU.VPR[vd]._u8[b] = (CPU.VPR[va]._u8[b] + CPU.VPR[vb]._u8[b] + 1) >> 1;
	}
	void VAVGUH(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._u16[h] = (CPU.VPR[va]._u16[h] + CPU.VPR[vb]._u16[h] + 1) >> 1;
		}
	}
	void VAVGUW(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u32[w] = ((u64)CPU.VPR[va]._u32[w] + (u64)CPU.VPR[vb]._u32[w] + 1) >> 1;
		}
	}
	void VCFSX(u32 vd, u32 uimm5, u32 vb)
	{
		u32 scale = 1 << uimm5;
			
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._f[w] = ((float)CPU.VPR[vb]._s32[w]) / scale;
		}
	}
	void VCFUX(u32 vd, u32 uimm5, u32 vb)
	{
		u32 scale = 1 << uimm5;
			
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._f[w] = ((float)CPU.VPR[vb]._u32[w]) / scale;
		}
	}
	void VCMPBFP(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			u32 mask = 0;

			const float A = CheckVSCR_NJ(CPU.VPR[va]._f[w]);
			const float B = CheckVSCR_NJ(CPU.VPR[vb]._f[w]);

			if (A >  B) mask |= 1 << 31;
			if (A < -B) mask |= 1 << 30;

			CPU.VPR[vd]._u32[w] = mask;
		}
	}
	void VCMPBFP_(u32 vd, u32 va, u32 vb)
	{
		bool allInBounds = true;

		for (uint w = 0; w < 4; w++)
		{
			u32 mask = 0;

			const float A = CheckVSCR_NJ(CPU.VPR[va]._f[w]);
			const float B = CheckVSCR_NJ(CPU.VPR[vb]._f[w]);

			if (A >  B) mask |= 1 << 31;
			if (A < -B) mask |= 1 << 30;

			CPU.VPR[vd]._u32[w] = mask;

			if (mask)
				allInBounds = false;
		}

		// Bit n°2 of CR6
		CPU.SetCRBit(6, 0x2, allInBounds);
	}
	void VCMPEQFP(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u32[w] = CPU.VPR[va]._f[w] == CPU.VPR[vb]._f[w] ? 0xffffffff : 0;
		}
	}
	void VCMPEQFP_(u32 vd, u32 va, u32 vb)
	{
		int all_equal = 0x8;
		int none_equal = 0x2;

		for (uint w = 0; w < 4; w++)
		{
			if (CPU.VPR[va]._f[w] == CPU.VPR[vb]._f[w])
			{
				CPU.VPR[vd]._u32[w] = 0xffffffff;
				none_equal = 0;
			}
			else
			{
				CPU.VPR[vd]._u32[w] = 0;
				all_equal = 0;
			}
		}

		CPU.CR.cr6 = all_equal | none_equal;
	}
	void VCMPEQUB(u32 vd, u32 va, u32 vb)
	{
		for (uint b = 0; b < 16; b++)
		{
			CPU.VPR[vd]._u8[b] = CPU.VPR[va]._u8[b] == CPU.VPR[vb]._u8[b] ? 0xff : 0;
		}
	}
	void VCMPEQUB_(u32 vd, u32 va, u32 vb)
	{
		int all_equal = 0x8;
		int none_equal = 0x2;
			
		for (uint b = 0; b < 16; b++)
		{
			if (CPU.VPR[va]._u8[b] == CPU.VPR[vb]._u8[b])
			{
				CPU.VPR[vd]._u8[b] = 0xff;
				none_equal = 0;
			}
			else
			{
				CPU.VPR[vd]._u8[b] = 0;
				all_equal = 0;
			}
		}

		CPU.CR.cr6 = all_equal | none_equal;
	}
	void VCMPEQUH(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._u16[h] = CPU.VPR[va]._u16[h] == CPU.VPR[vb]._u16[h] ? 0xffff : 0;
		}
	}
	void VCMPEQUH_(u32 vd, u32 va, u32 vb)
	{
		int all_equal = 0x8;
		int none_equal = 0x2;
			
		for (uint h = 0; h < 8; h++)
		{
			if (CPU.VPR[va]._u16[h] == CPU.VPR[vb]._u16[h])
			{
				CPU.VPR[vd]._u16[h] = 0xffff;
				none_equal = 0;
			}
			else
			{
				CPU.VPR[vd]._u16[h] = 0;
				all_equal = 0;
			}
		}
			
		CPU.CR.cr6 = all_equal | none_equal;
	}
	void VCMPEQUW(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u32[w] = CPU.VPR[va]._u32[w] == CPU.VPR[vb]._u32[w] ? 0xffffffff : 0;
		}
	}
	void VCMPEQUW_(u32 vd, u32 va, u32 vb)
	{
		int all_equal = 0x8;
		int none_equal = 0x2;
			
		for (uint w = 0; w < 4; w++)
		{
			if (CPU.VPR[va]._u32[w] == CPU.VPR[vb]._u32[w])
			{
				CPU.VPR[vd]._u32[w] = 0xffffffff;
				none_equal = 0;
			}
			else
			{
				CPU.VPR[vd]._u32[w] = 0;
				all_equal = 0;
			}
		}

		CPU.CR.cr6 = all_equal | none_equal;
	}
	void VCMPGEFP(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u32[w] = CPU.VPR[va]._f[w] >= CPU.VPR[vb]._f[w] ? 0xffffffff : 0;
		}
	}
	void VCMPGEFP_(u32 vd, u32 va, u32 vb)
	{
		int all_ge = 0x8;
		int none_ge = 0x2;
			
		for (uint w = 0; w < 4; w++)
		{
			if (CPU.VPR[va]._f[w] >= CPU.VPR[vb]._f[w])
			{
				CPU.VPR[vd]._u32[w] = 0xffffffff;
				none_ge = 0;
			}
			else
			{
				CPU.VPR[vd]._u32[w] = 0;
				all_ge = 0;
			}
		}

		CPU.CR.cr6 = all_ge | none_ge;
	}
	void VCMPGTFP(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u32[w] = CPU.VPR[va]._f[w] > CPU.VPR[vb]._f[w] ? 0xffffffff : 0;
		}
	}
	void VCMPGTFP_(u32 vd, u32 va, u32 vb)
	{
		int all_ge = 0x8;
		int none_ge = 0x2;
			
		for (uint w = 0; w < 4; w++)
		{
			if (CPU.VPR[va]._f[w] > CPU.VPR[vb]._f[w])
			{
				CPU.VPR[vd]._u32[w] = 0xffffffff;
				none_ge = 0;
			}
			else
			{
				CPU.VPR[vd]._u32[w] = 0;
				all_ge = 0;
			}
		}

		CPU.CR.cr6 = all_ge | none_ge;
	}
	void VCMPGTSB(u32 vd, u32 va, u32 vb)
	{
		for (uint b = 0; b < 16; b++)
		{
			CPU.VPR[vd]._u8[b] = CPU.VPR[va]._s8[b] > CPU.VPR[vb]._s8[b] ? 0xff : 0;
		}
	}
	void VCMPGTSB_(u32 vd, u32 va, u32 vb)
	{
		int all_gt = 0x8;
		int none_gt = 0x2;
			
		for (uint b = 0; b < 16; b++)
		{
			if (CPU.VPR[va]._s8[b] > CPU.VPR[vb]._s8[b])
			{
				CPU.VPR[vd]._u8[b] = 0xff;
				none_gt = 0;
			}
			else
			{
				CPU.VPR[vd]._u8[b] = 0;
				all_gt = 0;
			}
		}

		CPU.CR.cr6 = all_gt | none_gt;
	}
	void VCMPGTSH(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._u16[h] = CPU.VPR[va]._s16[h] > CPU.VPR[vb]._s16[h] ? 0xffff : 0;
		}
	}
	void VCMPGTSH_(u32 vd, u32 va, u32 vb)
	{
		int all_gt = 0x8;
		int none_gt = 0x2;
			
		for (uint h = 0; h < 8; h++)
		{
			if (CPU.VPR[va]._s16[h] > CPU.VPR[vb]._s16[h])
			{
				CPU.VPR[vd]._u16[h] = 0xffff;
				none_gt = 0;
			}
			else
			{
				CPU.VPR[vd]._u16[h] = 0;
				all_gt = 0;
			}
		}

		CPU.CR.cr6 = all_gt | none_gt;
	}
	void VCMPGTSW(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u32[w] = CPU.VPR[va]._s32[w] > CPU.VPR[vb]._s32[w] ? 0xffffffff : 0;
		}
	}
	void VCMPGTSW_(u32 vd, u32 va, u32 vb)
	{
		int all_gt = 0x8;
		int none_gt = 0x2;
			
		for (uint w = 0; w < 4; w++)
		{
			if (CPU.VPR[va]._s32[w] > CPU.VPR[vb]._s32[w])
			{
				CPU.VPR[vd]._u32[w] = 0xffffffff;
				none_gt = 0;
			}
			else
			{
				CPU.VPR[vd]._u32[w] = 0;
				all_gt = 0;
			}
		}

		CPU.CR.cr6 = all_gt | none_gt;
	}
	void VCMPGTUB(u32 vd, u32 va, u32 vb)
	{
		for (uint b = 0; b < 16; b++)
		{
			CPU.VPR[vd]._u8[b] = CPU.VPR[va]._u8[b] > CPU.VPR[vb]._u8[b] ? 0xff : 0;
		}
	}
	void VCMPGTUB_(u32 vd, u32 va, u32 vb)
	{
		int all_gt = 0x8;
		int none_gt = 0x2;

		for (uint b = 0; b < 16; b++)
		{
			if (CPU.VPR[va]._u8[b] > CPU.VPR[vb]._u8[b])
			{
				CPU.VPR[vd]._u8[b] = 0xff;
				none_gt = 0;
			}
			else
			{
				CPU.VPR[vd]._u8[b] = 0;
				all_gt = 0;
			}
		}

		CPU.CR.cr6 = all_gt | none_gt;
	}
	void VCMPGTUH(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._u16[h] = CPU.VPR[va]._u16[h] > CPU.VPR[vb]._u16[h] ? 0xffff : 0;
		}
	}
	void VCMPGTUH_(u32 vd, u32 va, u32 vb)
	{
		int all_gt = 0x8;
		int none_gt = 0x2;
			
		for (uint h = 0; h < 8; h++)
		{
			if (CPU.VPR[va]._u16[h] > CPU.VPR[vb]._u16[h])
			{
				CPU.VPR[vd]._u16[h] = 0xffff;
				none_gt = 0;
			}
			else
			{
				CPU.VPR[vd]._u16[h] = 0;
				all_gt = 0;
			}
		}

		CPU.CR.cr6 = all_gt | none_gt;
	}
	void VCMPGTUW(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u32[w] = CPU.VPR[va]._u32[w] > CPU.VPR[vb]._u32[w] ? 0xffffffff : 0;
		}
	}
	void VCMPGTUW_(u32 vd, u32 va, u32 vb)
	{
		int all_gt = 0x8;
		int none_gt = 0x2;
			
		for (uint w = 0; w < 4; w++)
		{
			if (CPU.VPR[va]._u32[w] > CPU.VPR[vb]._u32[w])
			{
				CPU.VPR[vd]._u32[w] = 0xffffffff;
				none_gt = 0;
			}
			else
			{
				CPU.VPR[vd]._u32[w] = 0;
				all_gt = 0;
			}
		}

		CPU.CR.cr6 = all_gt | none_gt;
	}
	void VCTSXS(u32 vd, u32 uimm5, u32 vb)
	{
		int nScale = 1 << uimm5;
			
		for (uint w = 0; w < 4; w++)
		{		
			float result = CPU.VPR[vb]._f[w] * nScale;

			if (result > INT_MAX)
				CPU.VPR[vd]._s32[w] = (int)INT_MAX;
			else if (result < INT_MIN)
				CPU.VPR[vd]._s32[w] = (int)INT_MIN;
			else // C rounding = Round towards 0
				CPU.VPR[vd]._s32[w] = (int)result;
		}
	}
	void VCTUXS(u32 vd, u32 uimm5, u32 vb)
	{
		int nScale = 1 << uimm5;

		for (uint w = 0; w < 4; w++)
		{
			// C rounding = Round towards 0
			s64 result = (s64)(CPU.VPR[vb]._f[w] * nScale);

			if (result > UINT_MAX)
				CPU.VPR[vd]._u32[w] = (u32)UINT_MAX;
			else if (result < 0)
				CPU.VPR[vd]._u32[w] = 0;
			else
				CPU.VPR[vd]._u32[w] = (u32)result;
		}
	}
	void VEXPTEFP(u32 vd, u32 vb)
	{
		// vd = exp(vb * log(2))
		// ISA : Note that the value placed into the element of vD may vary between implementations
		// and between different executions on the same implementation.
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._f[w] = exp(CPU.VPR[vb]._f[w] * log(2.0f));
		}
	}
	void VLOGEFP(u32 vd, u32 vb)
	{
		// ISA : Note that the value placed into the element of vD may vary between implementations
		// and between different executions on the same implementation.
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._f[w] = log(CPU.VPR[vb]._f[w]) / log(2.0f);
		}
	}
	void VMADDFP(u32 vd, u32 va, u32 vc, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._f[w] = CPU.VPR[va]._f[w] * CPU.VPR[vc]._f[w] + CPU.VPR[vb]._f[w];
		}
	}
	void VMAXFP(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._f[w] = max(CPU.VPR[va]._f[w], CPU.VPR[vb]._f[w]);
		}
	}
	void VMAXSB(u32 vd, u32 va, u32 vb)
	{
		for (uint b = 0; b < 16; b++)
			CPU.VPR[vd]._s8[b] = max(CPU.VPR[va]._s8[b], CPU.VPR[vb]._s8[b]);
	}
	void VMAXSH(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._s16[h] = max(CPU.VPR[va]._s16[h], CPU.VPR[vb]._s16[h]);
		}
	}
	void VMAXSW(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._s32[w] = max(CPU.VPR[va]._s32[w], CPU.VPR[vb]._s32[w]);
		}
	}
	void VMAXUB(u32 vd, u32 va, u32 vb)
	{
		for (uint b = 0; b < 16; b++)
			CPU.VPR[vd]._u8[b] = max(CPU.VPR[va]._u8[b], CPU.VPR[vb]._u8[b]);
	}
	void VMAXUH(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._u16[h] = max(CPU.VPR[va]._u16[h], CPU.VPR[vb]._u16[h]);
		}
	}
	void VMAXUW(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u32[w] = max(CPU.VPR[va]._u32[w], CPU.VPR[vb]._u32[w]);
		}
	}
	void VMHADDSHS(u32 vd, u32 va, u32 vb, u32 vc)
	{
		for (uint h = 0; h < 8; h++)
		{
			s32 result = (s32)CPU.VPR[va]._s16[h] * (s32)CPU.VPR[vb]._s16[h] + (s32)CPU.VPR[vc]._s16[h];

			if (result > INT16_MAX)
			{
				CPU.VPR[vd]._s16[h] = (s16)INT16_MAX;
				CPU.VSCR.SAT = 1;
			}
			else if (result < INT16_MIN)
			{
				CPU.VPR[vd]._s16[h] = (s16)INT16_MIN;
				CPU.VSCR.SAT = 1;
			}
			else
				CPU.VPR[vd]._s16[h] = (s16)result;
		}
	}
	void VMHRADDSHS(u32 vd, u32 va, u32 vb, u32 vc)
	{
		for (uint h = 0; h < 8; h++)
		{
			s32 result = (s32)CPU.VPR[va]._s16[h] * (s32)CPU.VPR[vb]._s16[h] + (s32)CPU.VPR[vc]._s16[h] + 0x4000;

			if (result > INT16_MAX)
			{
				CPU.VPR[vd]._s16[h] = (s16)INT16_MAX;
				CPU.VSCR.SAT = 1;
			}
			else if (result < INT16_MIN)
			{
				CPU.VPR[vd]._s16[h] = (s16)INT16_MIN;
				CPU.VSCR.SAT = 1;
			}
			else
				CPU.VPR[vd]._s16[h] = (s16)result;
		}
	}
	void VMINFP(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._f[w] = min(CPU.VPR[va]._f[w], CPU.VPR[vb]._f[w]);
		}
	}
	void VMINSB(u32 vd, u32 va, u32 vb)
	{
		for (uint b = 0; b < 16; b++)
		{
			CPU.VPR[vd]._s8[b] = min(CPU.VPR[va]._s8[b], CPU.VPR[vb]._s8[b]);
		}
	}
	void VMINSH(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._s16[h] = min(CPU.VPR[va]._s16[h], CPU.VPR[vb]._s16[h]);
		}
	}
	void VMINSW(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._s32[w] = min(CPU.VPR[va]._s32[w], CPU.VPR[vb]._s32[w]);
		}
	}
	void VMINUB(u32 vd, u32 va, u32 vb)
	{
		for (uint b = 0; b < 16; b++)
		{
			CPU.VPR[vd]._u8[b] = min(CPU.VPR[va]._u8[b], CPU.VPR[vb]._u8[b]);
		}
	}
	void VMINUH(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._u16[h] = min(CPU.VPR[va]._u16[h], CPU.VPR[vb]._u16[h]);
		}
	}
	void VMINUW(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u32[w] = min(CPU.VPR[va]._u32[w], CPU.VPR[vb]._u32[w]);
		}
	}
	void VMLADDUHM(u32 vd, u32 va, u32 vb, u32 vc)
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._u16[h] = CPU.VPR[va]._u16[h] * CPU.VPR[vb]._u16[h] + CPU.VPR[vc]._u16[h];
		}
	}
	void VMRGHB(u32 vd, u32 va, u32 vb)
	{
		VPR_reg VA = CPU.VPR[va];
		VPR_reg VB = CPU.VPR[vb];
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._u8[15 - h*2] = VA._u8[15 - h];
			CPU.VPR[vd]._u8[15 - h*2 - 1] = VB._u8[15 - h];
		}
	}
	void VMRGHH(u32 vd, u32 va, u32 vb)
	{
		VPR_reg VA = CPU.VPR[va];
		VPR_reg VB = CPU.VPR[vb];
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u16[7 - w*2] = VA._u16[7 - w];
			CPU.VPR[vd]._u16[7 - w*2 - 1] = VB._u16[7 - w];
		}
	}
	void VMRGHW(u32 vd, u32 va, u32 vb)
	{
		VPR_reg VA = CPU.VPR[va];
		VPR_reg VB = CPU.VPR[vb];
		for (uint d = 0; d < 2; d++)
		{
			CPU.VPR[vd]._u32[3 - d*2] = VA._u32[3 - d];
			CPU.VPR[vd]._u32[3 - d*2 - 1] = VB._u32[3 - d];
		}
	}
	void VMRGLB(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._u8[15 - h*2] = CPU.VPR[va]._u8[7 - h];
			CPU.VPR[vd]._u8[15 - h*2 - 1] = CPU.VPR[vb]._u8[7 - h];
		}
	}
	void VMRGLH(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u16[7 - w*2] = CPU.VPR[va]._u16[3 - w];
			CPU.VPR[vd]._u16[7 - w*2 - 1] = CPU.VPR[vb]._u16[3 - w];
		}
	}
	void VMRGLW(u32 vd, u32 va, u32 vb)
	{
		for (uint d = 0; d < 2; d++)
		{
			CPU.VPR[vd]._u32[3 - d*2] = CPU.VPR[va]._u32[1 - d];
			CPU.VPR[vd]._u32[3 - d*2 - 1] = CPU.VPR[vb]._u32[1 - d];
		}
	}
	void VMSUMMBM(u32 vd, u32 va, u32 vb, u32 vc)
	{
		for (uint w = 0; w < 4; w++)
		{
			s32 result = 0;

			for (uint b = 0; b < 4; b++)
			{
				result += CPU.VPR[va]._s8[w*4 + b] * CPU.VPR[vb]._u8[w*4 + b];
			}

			result += CPU.VPR[vc]._s32[w];
			CPU.VPR[vd]._s32[w] = result;
		}
	}
	void VMSUMSHM(u32 vd, u32 va, u32 vb, u32 vc)
	{
		for (uint w = 0; w < 4; w++)
		{
			s32 result = 0;

			for (uint h = 0; h < 2; h++)
			{
				result += CPU.VPR[va]._s16[w*2 + h] * CPU.VPR[vb]._s16[w*2 + h];
			}

			result += CPU.VPR[vc]._s32[w];
			CPU.VPR[vd]._s32[w] = result;
		}
	}
	void VMSUMSHS(u32 vd, u32 va, u32 vb, u32 vc)
	{
		for (uint w = 0; w < 4; w++)
		{
			s64 result = 0;
			s32 saturated = 0;

			for (uint h = 0; h < 2; h++)
			{
				result += CPU.VPR[va]._s16[w*2 + h] * CPU.VPR[vb]._s16[w*2 + h];
			}

			result += CPU.VPR[vc]._s32[w];

			if (result > INT_MAX)
			{
				saturated = INT_MAX;
				CPU.VSCR.SAT = 1;
			}
			else if (result < INT_MIN)
			{
				saturated = INT_MIN;
				CPU.VSCR.SAT = 1;
			}
			else
				saturated = (s32)result;

			CPU.VPR[vd]._s32[w] = saturated;
		}
	}
	void VMSUMUBM(u32 vd, u32 va, u32 vb, u32 vc)
	{
		for (uint w = 0; w < 4; w++)
		{
			u32 result = 0;

			for (uint b = 0; b < 4; b++)
			{
				result += CPU.VPR[va]._u8[w*4 + b] * CPU.VPR[vb]._u8[w*4 + b];
			}

			result += CPU.VPR[vc]._u32[w];
			CPU.VPR[vd]._u32[w] = result;
		}
	}
	void VMSUMUHM(u32 vd, u32 va, u32 vb, u32 vc)
	{
		for (uint w = 0; w < 4; w++)
		{
			u32 result = 0;

			for (uint h = 0; h < 2; h++)
			{
				result += CPU.VPR[va]._u16[w*2 + h] * CPU.VPR[vb]._u16[w*2 + h];
			}

			result += CPU.VPR[vc]._u32[w];
			CPU.VPR[vd]._u32[w] = result;
		}
	}
	void VMSUMUHS(u32 vd, u32 va, u32 vb, u32 vc)
	{
		for (uint w = 0; w < 4; w++)
		{
			u64 result = 0;
			u32 saturated = 0;

			for (uint h = 0; h < 2; h++)
			{
				result += CPU.VPR[va]._u16[w*2 + h] * CPU.VPR[vb]._u16[w*2 + h];
			}

			result += CPU.VPR[vc]._u32[w];

			if (result > UINT_MAX)
			{
				saturated = UINT_MAX;
				CPU.VSCR.SAT = 1;
			}
			else
				saturated = (u32)result;

			CPU.VPR[vd]._u32[w] = saturated;
		}
	}
	void VMULESB(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._s16[h] = (s16)CPU.VPR[va]._s8[h*2+1] * (s16)CPU.VPR[vb]._s8[h*2+1];
		}
	}
	void VMULESH(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._s32[w] = (s32)CPU.VPR[va]._s16[w*2+1] * (s32)CPU.VPR[vb]._s16[w*2+1];
		}
	}
	void VMULEUB(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._u16[h] = (u16)CPU.VPR[va]._u8[h*2+1] * (u16)CPU.VPR[vb]._u8[h*2+1];
		}
	}
	void VMULEUH(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u32[w] = (u32)CPU.VPR[va]._u16[w*2+1] * (u32)CPU.VPR[vb]._u16[w*2+1];
		}
	}
	void VMULOSB(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._s16[h] = (s16)CPU.VPR[va]._s8[h*2] * (s16)CPU.VPR[vb]._s8[h*2];
		}
	}
	void VMULOSH(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._s32[w] = (s32)CPU.VPR[va]._s16[w*2] * (s32)CPU.VPR[vb]._s16[w*2];
		}
	}
	void VMULOUB(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._u16[h] = (u16)CPU.VPR[va]._u8[h*2] * (u16)CPU.VPR[vb]._u8[h*2];
		}
	}
	void VMULOUH(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u32[w] = (u32)CPU.VPR[va]._u16[w*2] * (u32)CPU.VPR[vb]._u16[w*2];
		}
	}
	void VNMSUBFP(u32 vd, u32 va, u32 vc, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._f[w] = -(CPU.VPR[va]._f[w] * CPU.VPR[vc]._f[w] - CPU.VPR[vb]._f[w]);
		}
	}
	void VNOR(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u32[w] = ~(CPU.VPR[va]._u32[w] | CPU.VPR[vb]._u32[w]);
		}
	}
	void VOR(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u32[w] = CPU.VPR[va]._u32[w] | CPU.VPR[vb]._u32[w];
		}
	}
	void VPERM(u32 vd, u32 va, u32 vb, u32 vc)
	{
		u8 tmpSRC[32];
		memcpy(tmpSRC, CPU.VPR[vb]._u8, 16);
		memcpy(tmpSRC + 16, CPU.VPR[va]._u8, 16);

		for (uint b = 0; b < 16; b++)
		{
			u8 index = CPU.VPR[vc]._u8[b] & 0x1f;
				
			CPU.VPR[vd]._u8[b] = tmpSRC[0x1f - index];
		}
	}
	void VPKPX(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 4; h++)
		{
			u16 bb7  = CPU.VPR[vb]._u8[15 - (h*4 + 0)] & 0x1;
			u16 bb8  = CPU.VPR[vb]._u8[15 - (h*4 + 1)] >> 3;
			u16 bb16 = CPU.VPR[vb]._u8[15 - (h*4 + 2)] >> 3;
			u16 bb24 = CPU.VPR[vb]._u8[15 - (h*4 + 3)] >> 3;
			u16 ab7  = CPU.VPR[va]._u8[15 - (h*4 + 0)] & 0x1;
			u16 ab8  = CPU.VPR[va]._u8[15 - (h*4 + 1)] >> 3;
			u16 ab16 = CPU.VPR[va]._u8[15 - (h*4 + 2)] >> 3;
			u16 ab24 = CPU.VPR[va]._u8[15 - (h*4 + 3)] >> 3;

			CPU.VPR[vd]._u16[3 - h]			= (bb7 << 15) | (bb8 << 10) | (bb16 << 5) | bb24;
			CPU.VPR[vd]._u16[4 + (3 - h)]	= (ab7 << 15) | (ab8 << 10) | (ab16 << 5) | ab24;
		}
	}
	void VPKSHSS(u32 vd, u32 va, u32 vb)
	{
		for (uint b = 0; b < 8; b++)
		{
			s16 result = CPU.VPR[va]._s16[b];

			if (result > INT8_MAX)
			{
				result = INT8_MAX;
				CPU.VSCR.SAT = 1;
			}
			else if (result < INT8_MIN)
			{
				result = INT8_MIN;
				CPU.VSCR.SAT = 1;
			}
				
			CPU.VPR[vd]._s8[b+8] = result;

			result = CPU.VPR[vb]._s16[b];

			if (result > INT8_MAX)
			{
				result = INT8_MAX;
				CPU.VSCR.SAT = 1;
			}
			else if (result < INT8_MIN)
			{
				result = INT8_MIN;
				CPU.VSCR.SAT = 1;
			}

			CPU.VPR[vd]._s8[b] = result;
		}
	}
	void VPKSHUS(u32 vd, u32 va, u32 vb)
	{
		for (uint b = 0; b < 8; b++)
		{
			s16 result = CPU.VPR[va]._s16[b];

			if (result > UINT8_MAX)
			{
				result = UINT8_MAX;
				CPU.VSCR.SAT = 1;
			}
			else if (result < 0)
			{
				result = 0;
				CPU.VSCR.SAT = 1;
			}
				
			CPU.VPR[vd]._u8[b+8] = result;

			result = CPU.VPR[vb]._s16[b];

			if (result > UINT8_MAX)
			{
				result = UINT8_MAX;
				CPU.VSCR.SAT = 1;
			}
			else if (result < 0)
			{
				result = 0;
				CPU.VSCR.SAT = 1;
			}
				
			CPU.VPR[vd]._u8[b] = result;
		}
	}
	void VPKSWSS(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 4; h++)
		{
			s32 result = CPU.VPR[va]._s32[h];

			if (result > INT16_MAX)
			{
				result = INT16_MAX;
				CPU.VSCR.SAT = 1;
			}
			else if (result < INT16_MIN)
			{
				result = INT16_MIN;
				CPU.VSCR.SAT = 1;
			}
				
			CPU.VPR[vd]._s16[h+4] = result;

			result = CPU.VPR[vb]._s32[h];

			if (result > INT16_MAX)
			{
				result = INT16_MAX;
				CPU.VSCR.SAT = 1;
			}
			else if (result < INT16_MIN)
			{
				result = INT16_MIN;
				CPU.VSCR.SAT = 1;
			}
				
			CPU.VPR[vd]._s16[h] = result;
		}
	}
	void VPKSWUS(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 4; h++)
		{
			s32 result = CPU.VPR[va]._s32[h];

			if (result > UINT16_MAX)
			{
				result = UINT16_MAX;
				CPU.VSCR.SAT = 1;
			}
			else if (result < 0)
			{
				result = 0;
				CPU.VSCR.SAT = 1;
			}
				
			CPU.VPR[vd]._u16[h+4] = result;

			result = CPU.VPR[vb]._s32[h];

			if (result > UINT16_MAX)
			{
				result = UINT16_MAX;
				CPU.VSCR.SAT = 1;
			}
			else if (result < 0)
			{
				result = 0;
				CPU.VSCR.SAT = 1;
			}
				
			CPU.VPR[vd]._u16[h] = result;
		}
	}
	void VPKUHUM(u32 vd, u32 va, u32 vb)
	{
		for (uint b = 0; b < 8; b++)
		{
			CPU.VPR[vd]._u8[b+8] = CPU.VPR[va]._u8[b*2];
			CPU.VPR[vd]._u8[b  ] = CPU.VPR[vb]._u8[b*2];
		}
	}
	void VPKUHUS(u32 vd, u32 va, u32 vb)
	{
		for (uint b = 0; b < 8; b++)
		{
			u16 result = CPU.VPR[va]._u16[b];

			if (result > UINT8_MAX)
			{
				result = UINT8_MAX;
				CPU.VSCR.SAT = 1;
			}
				
			CPU.VPR[vd]._u8[b+8] = result;

			result = CPU.VPR[vb]._u16[b];

			if (result > UINT8_MAX)
			{
				result = UINT8_MAX;
				CPU.VSCR.SAT = 1;
			}
				
			CPU.VPR[vd]._u8[b] = result;
		}
	}
	void VPKUWUM(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 4; h++)
		{
			CPU.VPR[vd]._u16[h+4] = CPU.VPR[va]._u16[h*2];
			CPU.VPR[vd]._u16[h  ] = CPU.VPR[vb]._u16[h*2];
		}
	}
	void VPKUWUS(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 4; h++)
		{
			u32 result = CPU.VPR[va]._u32[h];

			if (result > UINT16_MAX)
			{
				result = UINT16_MAX;
				CPU.VSCR.SAT = 1;
			}
				
			CPU.VPR[vd]._u16[h+4] = result;

			result = CPU.VPR[vb]._u32[h];

			if (result > UINT16_MAX)
			{
				result = UINT16_MAX;
				CPU.VSCR.SAT = 1;
			}
				
			CPU.VPR[vd]._u16[h] = result;
		}
	}
	void VREFP(u32 vd, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._f[w] = 1.0f / CPU.VPR[vb]._f[w];
		}
	}
	void VRFIM(u32 vd, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._f[w] = floor(CPU.VPR[vb]._f[w]);
		}
	}
	void VRFIN(u32 vd, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._f[w] = floor(CPU.VPR[vb]._f[w] + 0.5f);
		}
	}
	void VRFIP(u32 vd, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._f[w] = ceil(CPU.VPR[vb]._f[w]);
		}
	}
	void VRFIZ(u32 vd, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			float f;
			modff(CPU.VPR[vb]._f[w], &f);
			CPU.VPR[vd]._f[w] = f;
		}
	}
	void VRLB(u32 vd, u32 va, u32 vb)
	{
		for (uint b = 0; b < 16; b++)
		{
			int nRot = CPU.VPR[vb]._u8[b] & 0x7;

			CPU.VPR[vd]._u8[b] = (CPU.VPR[va]._u8[b] << nRot) | (CPU.VPR[va]._u8[b] >> (8 - nRot));
		}
	}
	void VRLH(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._u16[h] = rotl16(CPU.VPR[va]._u16[h], CPU.VPR[vb]._u8[h*2] & 0xf);
		}
	}
	void VRLW(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u32[w] = rotl32(CPU.VPR[va]._u32[w], CPU.VPR[vb]._u8[w*4] & 0x1f);
		}
	}
	void VRSQRTEFP(u32 vd, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			//TODO: accurate div
			CPU.VPR[vd]._f[w] = 1.0f / sqrtf(CPU.VPR[vb]._f[w]);
		}
	}
	void VSEL(u32 vd, u32 va, u32 vb, u32 vc)
	{
		for (uint b = 0; b < 16; b++)
		{
			CPU.VPR[vd]._u8[b] = (CPU.VPR[vb]._u8[b] & CPU.VPR[vc]._u8[b]) | (CPU.VPR[va]._u8[b] & (~CPU.VPR[vc]._u8[b]));
		}
	}
	void VSL(u32 vd, u32 va, u32 vb)
	{
		u8 sh = CPU.VPR[vb]._u8[0] & 0x7;

		u32 t = 1;

		for (uint b = 0; b < 16; b++)
		{
			t &= (CPU.VPR[vb]._u8[b] & 0x7) == sh;
		}

		if(t)
		{
			CPU.VPR[vd]._u8[0] = CPU.VPR[va]._u8[0] << sh;

			for (uint b = 1; b < 16; b++)
			{
				CPU.VPR[vd]._u8[b] = (CPU.VPR[va]._u8[b] << sh) | (CPU.VPR[va]._u8[b-1] >> (8 - sh));
			}
		}
		else
		{
			//undefined
			CPU.VPR[vd]._u32[0] = 0xCDCDCDCD;
			CPU.VPR[vd]._u32[1] = 0xCDCDCDCD;
			CPU.VPR[vd]._u32[2] = 0xCDCDCDCD;
			CPU.VPR[vd]._u32[3] = 0xCDCDCDCD;
		}
	}
	void VSLB(u32 vd, u32 va, u32 vb)
	{
		for (uint b = 0; b < 16; b++)
		{
			CPU.VPR[vd]._u8[b] = CPU.VPR[va]._u8[b] << (CPU.VPR[vb]._u8[b] & 0x7);
		}
	}
	void VSLDOI(u32 vd, u32 va, u32 vb, u32 sh)
	{
		u8 tmpSRC[32];
		memcpy(tmpSRC, CPU.VPR[vb]._u8, 16);
		memcpy(tmpSRC + 16, CPU.VPR[va]._u8, 16);

		for(uint b=0; b<16; b++)
		{
			CPU.VPR[vd]._u8[15 - b] = tmpSRC[31 - (b + sh)];
		}
	}
	void VSLH(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._u16[h] = CPU.VPR[va]._u16[h] << (CPU.VPR[vb]._u8[h*2] & 0xf);
		}
	}
	void VSLO(u32 vd, u32 va, u32 vb)
	{
		u8 nShift = (CPU.VPR[vb]._u8[0] >> 3) & 0xf;

		CPU.VPR[vd].Clear();

		for (u8 b = 0; b < 16 - nShift; b++)
		{
			CPU.VPR[vd]._u8[15 - b] = CPU.VPR[va]._u8[15 - (b + nShift)];
		}
	}
	void VSLW(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u32[w] = CPU.VPR[va]._u32[w] << (CPU.VPR[vb]._u32[w] & 0x1f);
		}
	}
	void VSPLTB(u32 vd, u32 uimm5, u32 vb)
	{
		u8 byte = CPU.VPR[vb]._u8[15 - uimm5];
			
		for (uint b = 0; b < 16; b++)
		{
			CPU.VPR[vd]._u8[b] = byte;
		}
	}
	void VSPLTH(u32 vd, u32 uimm5, u32 vb)
	{
		assert(uimm5 < 8);
			
		u16 hword = CPU.VPR[vb]._u16[7 - uimm5];

		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._u16[h] = hword;
		}
	}
	void VSPLTISB(u32 vd, s32 simm5)
	{
		for (uint b = 0; b < 16; b++)
		{
			CPU.VPR[vd]._u8[b] = simm5;
		}
	}
	void VSPLTISH(u32 vd, s32 simm5)
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._u16[h] = (s16)simm5;
		}
	}
	void VSPLTISW(u32 vd, s32 simm5)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u32[w] = (s32)simm5;
		}
	}
	void VSPLTW(u32 vd, u32 uimm5, u32 vb)
	{
		assert(uimm5 < 4);

		u32 word = CPU.VPR[vb]._u32[3 - uimm5];
			
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u32[w] = word;
		}
	}
	void VSR(u32 vd, u32 va, u32 vb)
	{
		u8 sh = CPU.VPR[vb]._u8[0] & 0x7;
		u32 t = 1;

		for (uint b = 0; b < 16; b++)
		{
			t &= (CPU.VPR[vb]._u8[b] & 0x7) == sh;
		}

		if(t)
		{
			CPU.VPR[vd]._u8[15] = CPU.VPR[va]._u8[15] >> sh;

			for (uint b = 14; ~b; b--)
			{
				CPU.VPR[vd]._u8[b] = (CPU.VPR[va]._u8[b] >> sh) | (CPU.VPR[va]._u8[b+1] << (8 - sh));
			}
		}
		else
		{
			//undefined
			CPU.VPR[vd]._u32[0] = 0xCDCDCDCD;
			CPU.VPR[vd]._u32[1] = 0xCDCDCDCD;
			CPU.VPR[vd]._u32[2] = 0xCDCDCDCD;
			CPU.VPR[vd]._u32[3] = 0xCDCDCDCD;
		}
	}
	void VSRAB(u32 vd, u32 va, u32 vb)
	{
		for (uint b = 0; b < 16; b++)
		{
			CPU.VPR[vd]._s8[b] = CPU.VPR[va]._s8[b] >> (CPU.VPR[vb]._u8[b] & 0x7);
		}
	}
	void VSRAH(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._s16[h] = CPU.VPR[va]._s16[h] >> (CPU.VPR[vb]._u8[h*2] & 0xf);
		}
	}
	void VSRAW(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._s32[w] = CPU.VPR[va]._s32[w] >> (CPU.VPR[vb]._u8[w*4] & 0x1f);
		}
	}
	void VSRB(u32 vd, u32 va, u32 vb)
	{
		for (uint b = 0; b < 16; b++)
		{
			CPU.VPR[vd]._u8[b] = CPU.VPR[va]._u8[b] >> (CPU.VPR[vb]._u8[b] & 0x7);
		}
	}
	void VSRH(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._u16[h] = CPU.VPR[va]._u16[h] >> (CPU.VPR[vb]._u8[h*2] & 0xf);
		}
	}
	void VSRO(u32 vd, u32 va, u32 vb)
	{
		u8 nShift = (CPU.VPR[vb]._u8[0] >> 3) & 0xf;

		CPU.VPR[vd].Clear();

		for (u8 b = 0; b < 16 - nShift; b++)
		{
			CPU.VPR[vd]._u8[b] = CPU.VPR[va]._u8[b + nShift];
		}
	}
	void VSRW(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u32[w] = CPU.VPR[va]._u32[w] >> (CPU.VPR[vb]._u8[w*4] & 0x1f);
		}
	}
	void VSUBCUW(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u32[w] = CPU.VPR[va]._u32[w] < CPU.VPR[vb]._u32[w] ? 0 : 1;
		}
	}
	void VSUBFP(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._f[w] = CPU.VPR[va]._f[w] - CPU.VPR[vb]._f[w];
		}
	}
	void VSUBSBS(u32 vd, u32 va, u32 vb)
	{
		for (uint b = 0; b < 16; b++)
		{
			s16 result = (s16)CPU.VPR[va]._s8[b] - (s16)CPU.VPR[vb]._s8[b];

			if (result < INT8_MIN)
			{
				CPU.VPR[vd]._s8[b] = INT8_MIN;
				CPU.VSCR.SAT = 1;
			}
			else if (result > INT8_MAX)
			{
				CPU.VPR[vd]._s8[b] = INT8_MAX;
				CPU.VSCR.SAT = 1;
			}
			else
				CPU.VPR[vd]._s8[b] = (s8)result;
		}
	}
	void VSUBSHS(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 8; h++)
		{
			s32 result = (s32)CPU.VPR[va]._s16[h] - (s32)CPU.VPR[vb]._s16[h];

			if (result < INT16_MIN)
			{
				CPU.VPR[vd]._s16[h] = (s16)INT16_MIN;
				CPU.VSCR.SAT = 1;
			}
			else if (result > INT16_MAX)
			{
				CPU.VPR[vd]._s16[h] = (s16)INT16_MAX;
				CPU.VSCR.SAT = 1;
			}
			else
				CPU.VPR[vd]._s16[h] = (s16)result;
		}
	}
	void VSUBSWS(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			s64 result = (s64)CPU.VPR[va]._s32[w] - (s64)CPU.VPR[vb]._s32[w];

			if (result < INT32_MIN)
			{
				CPU.VPR[vd]._s32[w] = (s32)INT32_MIN;
				CPU.VSCR.SAT = 1;
			}
			else if (result > INT32_MAX)
			{
				CPU.VPR[vd]._s32[w] = (s32)INT32_MAX;
				CPU.VSCR.SAT = 1;
			}
			else
				CPU.VPR[vd]._s32[w] = (s32)result;
		}
	}
	void VSUBUBM(u32 vd, u32 va, u32 vb)
	{
		for (uint b = 0; b < 16; b++)
		{
			CPU.VPR[vd]._u8[b] = (u8)((CPU.VPR[va]._u8[b] - CPU.VPR[vb]._u8[b]) & 0xff);
		}
	}
	void VSUBUBS(u32 vd, u32 va, u32 vb)
	{
		for (uint b = 0; b < 16; b++)
		{
				s16 result = (s16)CPU.VPR[va]._u8[b] - (s16)CPU.VPR[vb]._u8[b];

				if (result < 0)
				{
				CPU.VPR[vd]._u8[b] = 0;
				CPU.VSCR.SAT = 1;
				}
				else
					CPU.VPR[vd]._u8[b] = (u8)result;
		}
	}
	void VSUBUHM(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._u16[h] = CPU.VPR[va]._u16[h] - CPU.VPR[vb]._u16[h];
		}
	}
	void VSUBUHS(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 8; h++)
		{
			s32 result = (s32)CPU.VPR[va]._u16[h] - (s32)CPU.VPR[vb]._u16[h];

			if (result < 0)
			{
				CPU.VPR[vd]._u16[h] = 0;
				CPU.VSCR.SAT = 1;
			}
			else
				CPU.VPR[vd]._u16[h] = (u16)result;
		}
	}
	void VSUBUWM(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u32[w] = CPU.VPR[va]._u32[w] - CPU.VPR[vb]._u32[w];
		}
	}
	void VSUBUWS(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			s64 result = (s64)CPU.VPR[va]._u32[w] - (s64)CPU.VPR[vb]._u32[w];

			if (result < 0)
			{
				CPU.VPR[vd]._u32[w] = 0;
				CPU.VSCR.SAT = 1;
			}
			else
				CPU.VPR[vd]._u32[w] = (u32)result;
		}
	}
	void VSUMSWS(u32 vd, u32 va, u32 vb)
	{
		CPU.VPR[vd].Clear();
			
		s64 sum = CPU.VPR[vb]._s32[3];

		for (uint w = 0; w < 4; w++)
		{
			sum += CPU.VPR[va]._s32[w];
		}

		if (sum > INT32_MAX)
		{
			CPU.VPR[vd]._s32[0] = (s32)INT32_MAX;
			CPU.VSCR.SAT = 1;
		}
		else if (sum < INT32_MIN)
		{
			CPU.VPR[vd]._s32[0] = (s32)INT32_MIN;
			CPU.VSCR.SAT = 1;
		}
		else
			CPU.VPR[vd]._s32[0] = (s32)sum;
	}
	void VSUM2SWS(u32 vd, u32 va, u32 vb)
	{
		for (uint n = 0; n < 2; n++)
		{
			s64 sum = (s64)CPU.VPR[va]._s32[n*2] + CPU.VPR[va]._s32[n*2 + 1] + CPU.VPR[vb]._s32[n*2];

			if (sum > INT32_MAX)
			{
				CPU.VPR[vd]._s32[n*2] = (s32)INT32_MAX;
				CPU.VSCR.SAT = 1;
			}
			else if (sum < INT32_MIN)
			{
				CPU.VPR[vd]._s32[n*2] = (s32)INT32_MIN;
				CPU.VSCR.SAT = 1;
			}
			else
				CPU.VPR[vd]._s32[n*2] = (s32)sum;
		}
		CPU.VPR[vd]._s32[1] = 0;
		CPU.VPR[vd]._s32[3] = 0;
	}
	void VSUM4SBS(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			s64 sum = CPU.VPR[vb]._s32[w];

			for (uint b = 0; b < 4; b++)
			{
				sum += CPU.VPR[va]._s8[w*4 + b];
			}

			if (sum > INT32_MAX)
			{
				CPU.VPR[vd]._s32[w] = (s32)INT32_MAX;
				CPU.VSCR.SAT = 1;
			}
			else if (sum < INT32_MIN)
			{
				CPU.VPR[vd]._s32[w] = (s32)INT32_MIN;
				CPU.VSCR.SAT = 1;
			}
			else
				CPU.VPR[vd]._s32[w] = (s32)sum;
		}
	}
	void VSUM4SHS(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			s64 sum = CPU.VPR[vb]._s32[w];

			for (uint h = 0; h < 2; h++)
			{
				sum += CPU.VPR[va]._s16[w*2 + h];
			}

			if (sum > INT32_MAX)
			{
				CPU.VPR[vd]._s32[w] = (s32)INT32_MAX;
				CPU.VSCR.SAT = 1;
			}
			else if (sum < INT32_MIN)
			{
				CPU.VPR[vd]._s32[w] = (s32)INT32_MIN;
				CPU.VSCR.SAT = 1;
			}
			else
				CPU.VPR[vd]._s32[w] = (s32)sum;
		}
	}
	void VSUM4UBS(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			u64 sum = CPU.VPR[vb]._u32[w];

			for (uint b = 0; b < 4; b++)
			{
				sum += CPU.VPR[va]._u8[w*4 + b];
			}

			if (sum > UINT32_MAX)
			{
				CPU.VPR[vd]._u32[w] = (u32)UINT32_MAX;
				CPU.VSCR.SAT = 1;
			}
			else
				CPU.VPR[vd]._u32[w] = (u32)sum;
		}
	}
	void VUPKHPX(u32 vd, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._s8[(3 - w)*4 + 3] = CPU.VPR[vb]._s8[w*2 + 0] >> 7;  // signed shift sign extends
			CPU.VPR[vd]._u8[(3 - w)*4 + 2] = (CPU.VPR[vb]._u8[w*2 + 0] >> 2) & 0x1f;
			CPU.VPR[vd]._u8[(3 - w)*4 + 1] = ((CPU.VPR[vb]._u8[w*2 + 0] & 0x3) << 3) | ((CPU.VPR[vb]._u8[w*2 + 1] >> 5) & 0x7);
			CPU.VPR[vd]._u8[(3 - w)*4 + 0] = CPU.VPR[vb]._u8[w*2 + 1] & 0x1f;
		}
	}
	void VUPKHSB(u32 vd, u32 vb)
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._s16[h] = CPU.VPR[vb]._s8[h];
		}
	}
	void VUPKHSH(u32 vd, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._s32[w] = CPU.VPR[vb]._s16[w];
		}
	}
	void VUPKLPX(u32 vd, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._s8[(3 - w)*4 + 3] = CPU.VPR[vb]._s8[8 + w*2 + 0] >> 7;  // signed shift sign extends
			CPU.VPR[vd]._u8[(3 - w)*4 + 2] = (CPU.VPR[vb]._u8[8 + w*2 + 0] >> 2) & 0x1f;
			CPU.VPR[vd]._u8[(3 - w)*4 + 1] = ((CPU.VPR[vb]._u8[8 + w*2 + 0] & 0x3) << 3) | ((CPU.VPR[vb]._u8[8 + w*2 + 1] >> 5) & 0x7);
			CPU.VPR[vd]._u8[(3 - w)*4 + 0] = CPU.VPR[vb]._u8[8 + w*2 + 1] & 0x1f;
		}
	}
	void VUPKLSB(u32 vd, u32 vb)
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._s16[h] = CPU.VPR[vb]._s8[8 + h];
		}
	}
	void VUPKLSH(u32 vd, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._s32[w] = CPU.VPR[vb]._s16[4 + w];
		}
	}
	void VXOR(u32 vd, u32 va, u32 vb)
	{
		CPU.VPR[vd]._u32[0] = CPU.VPR[va]._u32[0] ^ CPU.VPR[vb]._u32[0];
		CPU.VPR[vd]._u32[1] = CPU.VPR[va]._u32[1] ^ CPU.VPR[vb]._u32[1];
		CPU.VPR[vd]._u32[2] = CPU.VPR[va]._u32[2] ^ CPU.VPR[vb]._u32[2];
		CPU.VPR[vd]._u32[3] = CPU.VPR[va]._u32[3] ^ CPU.VPR[vb]._u32[3];
	}
	void MULLI(u32 rd, u32 ra, s32 simm16)
	{
		CPU.GPR[rd] = (s64)CPU.GPR[ra] * simm16;
	}
	void SUBFIC(u32 rd, u32 ra, s32 simm16)
	{
		const u64 RA = CPU.GPR[ra];
		const u64 IMM = (u64)(s64)simm16;
		CPU.GPR[rd] = IMM - RA;
		CPU.XER.CA = RA > IMM;
	}
	void CMPLI(u32 crfd, u32 l, u32 ra, u32 uimm16)
	{
		CPU.UpdateCRnU(l, crfd, CPU.GPR[ra], uimm16);
	}
	void CMPI(u32 crfd, u32 l, u32 ra, s32 simm16)
	{
		CPU.UpdateCRnS(l, crfd, CPU.GPR[ra], simm16);
	}
	void ADDIC(u32 rd, u32 ra, s32 simm16)
	{
		const u64 RA = CPU.GPR[ra];
		CPU.GPR[rd] = RA + simm16;
		CPU.XER.CA = CPU.IsCarry(RA, simm16);
	}
	void ADDIC_(u32 rd, u32 ra, s32 simm16)
	{
		const u64 RA = CPU.GPR[ra];
		CPU.GPR[rd] = RA + simm16;
		CPU.XER.CA = CPU.IsCarry(RA, simm16);
		CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void ADDI(u32 rd, u32 ra, s32 simm16)
	{
		CPU.GPR[rd] = ra ? ((s64)CPU.GPR[ra] + simm16) : simm16;
	}
	void ADDIS(u32 rd, u32 ra, s32 simm16)
	{
		CPU.GPR[rd] = ra ? ((s64)CPU.GPR[ra] + (simm16 << 16)) : (simm16 << 16);
	}
	void BC(u32 bo, u32 bi, s32 bd, u32 aa, u32 lk)
	{
		if (CheckCondition(bo, bi))
		{
			CPU.SetBranch(branchTarget((aa ? 0 : CPU.PC), bd), lk);
		}
		if(lk) CPU.LR = CPU.PC + 4;
	}
	void SC(s32 sc_code)
	{
		switch(sc_code)
		{
		case 0x1: UNK(fmt::Format("HyperCall %d", CPU.GPR[0])); break;
		case 0x2: SysCall(); break;
		case 0x3:
			StaticExecute(CPU.GPR[11]);
			if (Ini.HLELogging.GetValue())
			{
				ConLog.Write("'%s' done with code[0x%llx]! #pc: 0x%llx",
					g_static_funcs_list[CPU.GPR[11]].name, CPU.GPR[3], CPU.PC);
			}
			break;
		case 0x22: UNK("HyperCall LV1"); break;
		default: UNK(fmt::Format("Unknown sc: %x", sc_code));
		}
	}
	void B(s32 ll, u32 aa, u32 lk)
	{
		CPU.SetBranch(branchTarget(aa ? 0 : CPU.PC, ll), lk);
		if(lk) CPU.LR = CPU.PC + 4;
	}
	void MCRF(u32 crfd, u32 crfs)
	{
		CPU.SetCR(crfd, CPU.GetCR(crfs));
	}
	void BCLR(u32 bo, u32 bi, u32 bh, u32 lk)
	{
		if (CheckCondition(bo, bi))
		{
			CPU.SetBranch(branchTarget(0, CPU.LR), true);
		}
		if(lk) CPU.LR = CPU.PC + 4;
	}
	void CRNOR(u32 crbd, u32 crba, u32 crbb)
	{
		const u8 v = 1 ^ (CPU.IsCR(crba) | CPU.IsCR(crbb));
		CPU.SetCRBit2(crbd, v & 0x1);
	}
	void CRANDC(u32 crbd, u32 crba, u32 crbb)
	{
		const u8 v = CPU.IsCR(crba) & (1 ^ CPU.IsCR(crbb));
		CPU.SetCRBit2(crbd, v & 0x1);
	}
	void ISYNC()
	{
		_mm_mfence();
	}
	void CRXOR(u32 crbd, u32 crba, u32 crbb)
	{
		const u8 v = CPU.IsCR(crba) ^ CPU.IsCR(crbb);
		CPU.SetCRBit2(crbd, v & 0x1);
	}
	void CRNAND(u32 crbd, u32 crba, u32 crbb)
	{
		const u8 v = 1 ^ (CPU.IsCR(crba) & CPU.IsCR(crbb));
		CPU.SetCRBit2(crbd, v & 0x1);
	}
	void CRAND(u32 crbd, u32 crba, u32 crbb)
	{
		const u8 v = CPU.IsCR(crba) & CPU.IsCR(crbb);
		CPU.SetCRBit2(crbd, v & 0x1);
	}
	void CREQV(u32 crbd, u32 crba, u32 crbb)
	{
		const u8 v = 1 ^ (CPU.IsCR(crba) ^ CPU.IsCR(crbb));
		CPU.SetCRBit2(crbd, v & 0x1);
	}
	void CRORC(u32 crbd, u32 crba, u32 crbb)
	{
		const u8 v = CPU.IsCR(crba) | (1 ^ CPU.IsCR(crbb));
		CPU.SetCRBit2(crbd, v & 0x1);
	}
	void CROR(u32 crbd, u32 crba, u32 crbb)
	{
		const u8 v = CPU.IsCR(crba) | CPU.IsCR(crbb);
		CPU.SetCRBit2(crbd, v & 0x1);
	}
	void BCCTR(u32 bo, u32 bi, u32 bh, u32 lk)
	{
		if(bo & 0x10 || CPU.IsCR(bi) == (bo & 0x8))
		{
			CPU.SetBranch(branchTarget(0, CPU.CTR), true);
		}
		if(lk) CPU.LR = CPU.PC + 4;
	}	
	void RLWIMI(u32 ra, u32 rs, u32 sh, u32 mb, u32 me, bool rc)
	{
		const u64 mask = rotate_mask[32 + mb][32 + me];
		CPU.GPR[ra] = (CPU.GPR[ra] & ~mask) | (rotl32(CPU.GPR[rs], sh) & mask);
		if(rc) CPU.UpdateCR0<s32>(CPU.GPR[ra]);
	}
	void RLWINM(u32 ra, u32 rs, u32 sh, u32 mb, u32 me, bool rc)
	{
		CPU.GPR[ra] = rotl32(CPU.GPR[rs], sh) & rotate_mask[32 + mb][32 + me];
		if(rc) CPU.UpdateCR0<s32>(CPU.GPR[ra]);
	}
	void RLWNM(u32 ra, u32 rs, u32 rb, u32 mb, u32 me, bool rc)
	{
		CPU.GPR[ra] = rotl32(CPU.GPR[rs], CPU.GPR[rb] & 0x1f) & rotate_mask[32 + mb][32 + me];
		if(rc) CPU.UpdateCR0<s32>(CPU.GPR[ra]);
	}
	void ORI(u32 ra, u32 rs, u32 uimm16)
	{
		CPU.GPR[ra] = CPU.GPR[rs] | uimm16;
	}
	void ORIS(u32 ra, u32 rs, u32 uimm16)
	{
		CPU.GPR[ra] = CPU.GPR[rs] | (uimm16 << 16);
	}
	void XORI(u32 ra, u32 rs, u32 uimm16)
	{
		CPU.GPR[ra] = CPU.GPR[rs] ^ uimm16;
	}
	void XORIS(u32 ra, u32 rs, u32 uimm16)
	{
		CPU.GPR[ra] = CPU.GPR[rs] ^ (uimm16 << 16);
	}
	void ANDI_(u32 ra, u32 rs, u32 uimm16)
	{
		CPU.GPR[ra] = CPU.GPR[rs] & uimm16;
		CPU.UpdateCR0<s16>(CPU.GPR[ra]);
	}
	void ANDIS_(u32 ra, u32 rs, u32 uimm16)
	{
		CPU.GPR[ra] = CPU.GPR[rs] & (uimm16 << 16);
		CPU.UpdateCR0<s32>(CPU.GPR[ra]);
	}
	void RLDICL(u32 ra, u32 rs, u32 sh, u32 mb, bool rc)
	{
		CPU.GPR[ra] = rotl64(CPU.GPR[rs], sh) & rotate_mask[mb][63];
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void RLDICR(u32 ra, u32 rs, u32 sh, u32 me, bool rc)
	{
		CPU.GPR[ra] = rotl64(CPU.GPR[rs], sh) & rotate_mask[0][me];
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void RLDIC(u32 ra, u32 rs, u32 sh, u32 mb, bool rc)
	{
		CPU.GPR[ra] = rotl64(CPU.GPR[rs], sh) & rotate_mask[mb][63-sh];
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void RLDIMI(u32 ra, u32 rs, u32 sh, u32 mb, bool rc)
	{
		const u64 mask = rotate_mask[mb][63-sh];
		CPU.GPR[ra] = (CPU.GPR[ra] & ~mask) | (rotl64(CPU.GPR[rs], sh) & mask);
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void RLDC_LR(u32 ra, u32 rs, u32 rb, u32 m_eb, bool is_r, bool rc)
	{
		if (is_r) // rldcr
		{
			RLDICR(ra, rs, CPU.GPR[rb], m_eb, rc);
		}
		else // rldcl
		{
			RLDICL(ra, rs, CPU.GPR[rb], m_eb, rc);
		}
	}
	void CMP(u32 crfd, u32 l, u32 ra, u32 rb)
	{
		CPU.UpdateCRnS(l, crfd, CPU.GPR[ra], CPU.GPR[rb]);
	}
	void TW(u32 to, u32 ra, u32 rb)
	{
		s32 a = CPU.GPR[ra];
		s32 b = CPU.GPR[rb];

		if( (a < b  && (to & 0x10)) ||
			(a > b  && (to & 0x8))  ||
			(a == b && (to & 0x4))  ||
			((u32)a < (u32)b && (to & 0x2)) ||
			((u32)a > (u32)b && (to & 0x1)) )
		{
			UNK(fmt::Format("Trap! (tw %x, r%d, r%d)", to, ra, rb));
		}
	}
	void LVSL(u32 vd, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];

		static const u64 lvsl_values[0x10][2] =
		{
			{0x08090A0B0C0D0E0F, 0x0001020304050607},
			{0x090A0B0C0D0E0F10, 0x0102030405060708},
			{0x0A0B0C0D0E0F1011, 0x0203040506070809},
			{0x0B0C0D0E0F101112, 0x030405060708090A},
			{0x0C0D0E0F10111213, 0x0405060708090A0B},
			{0x0D0E0F1011121314, 0x05060708090A0B0C},
			{0x0E0F101112131415, 0x060708090A0B0C0D},
			{0x0F10111213141516, 0x0708090A0B0C0D0E},
			{0x1011121314151617, 0x08090A0B0C0D0E0F},
			{0x1112131415161718, 0x090A0B0C0D0E0F10},
			{0x1213141516171819, 0x0A0B0C0D0E0F1011},
			{0x131415161718191A, 0x0B0C0D0E0F101112},
			{0x1415161718191A1B, 0x0C0D0E0F10111213},
			{0x15161718191A1B1C, 0x0D0E0F1011121314},
			{0x161718191A1B1C1D, 0x0E0F101112131415},
			{0x1718191A1B1C1D1E, 0x0F10111213141516},
		};

		CPU.VPR[vd]._u64[0] = lvsl_values[addr & 0xf][0];
		CPU.VPR[vd]._u64[1] = lvsl_values[addr & 0xf][1];
	}
	void LVEBX(u32 vd, u32 ra, u32 rb)
	{
		//const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		//CPU.VPR[vd].Clear();
		//CPU.VPR[vd]._u8[addr & 0xf] = Memory.Read8(addr);
		CPU.VPR[vd]._u128 = Memory.Read128((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~0xfULL);
	}
	void SUBFC(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		const s64 RA = CPU.GPR[ra];
		const s64 RB = CPU.GPR[rb];
		CPU.GPR[rd] = ~RA + RB + 1;
		CPU.XER.CA = CPU.IsCarry(RA, RB);
		if(oe) UNK("subfco");
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void ADDC(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		const s64 RA = CPU.GPR[ra];
		const s64 RB = CPU.GPR[rb];
		CPU.GPR[rd] = RA + RB;
		CPU.XER.CA = RA <= RB;
		if(oe) UNK("addco");
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void MULHDU(u32 rd, u32 ra, u32 rb, bool rc)
	{
#ifdef _M_X64
		CPU.GPR[rd] = __umulh(CPU.GPR[ra], CPU.GPR[rb]);
#else
		//ConLog.Warning("MULHDU");
		const u64 RA = CPU.GPR[ra];
		const u64 RB = CPU.GPR[rb];

		u128 RD;

		u64& lo =  (u64&)((u32*)&RD)[0];
		u64& mid = (u64&)((u32*)&RD)[1];
		u64& hi =  (u64&)((u32*)&RD)[2];

		const u64 a0 = ((u32*)&RA)[0];
		const u64 a1 = ((u32*)&RA)[1];
		const u64 b0 = ((u32*)&RB)[0];
		const u64 b1 = ((u32*)&RB)[1];

		lo = a0 * b0;
		hi = a1 * b1;

		mid += (a0 + a1) * (b0 + b1) - (lo + hi);

		CPU.GPR[rd] = RD._u64[1];
#endif

		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void MULHWU(u32 rd, u32 ra, u32 rb, bool rc)
	{
		u32 a = CPU.GPR[ra];
		u32 b = CPU.GPR[rb];
		CPU.GPR[rd] = ((u64)a * (u64)b) >> 32;
		if(rc) CPU.UpdateCR0<s32>(CPU.GPR[rd]);
	}
	void MFOCRF(u32 a, u32 rd, u32 crm)
	{
		/*
		if(a)
		{
			u32 n = 0, count = 0;
			for(u32 i = 0; i < 8; ++i)
			{
				if(crm & (1 << i))
				{
					n = i;
					count++;
				}
			}

			if(count == 1)
			{
				//RD[32+4*n : 32+4*n+3] = CR[4*n : 4*n+3];
				u8 offset = n * 4;
				CPU.GPR[rd] = (CPU.GPR[rd] & ~(0xf << offset)) | ((u32)CPU.GetCR(7 - n) << offset);
			}
			else
				CPU.GPR[rd] = 0;
		}
		else
		{
		*/
		CPU.GPR[rd] = CPU.CR.CR;
		//}
	}
	void LWARX(u32 rd, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];

		SMutexLocker lock(reservation.mutex);
		reservation.owner = lock.tid;
		reservation.addr = addr;
		reservation.size = 4;
		reservation.data32 = CPU.GPR[rd] = Memory.Read32(addr);
	}
	void LDX(u32 rd, u32 ra, u32 rb)
	{
		CPU.GPR[rd] = Memory.Read64(ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]);
	}
	void LWZX(u32 rd, u32 ra, u32 rb)
	{
		CPU.GPR[rd] = Memory.Read32(ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]);
	}
	void SLW(u32 ra, u32 rs, u32 rb, bool rc)
	{
		u32 n = CPU.GPR[rb] & 0x1f;
		u32 r = rotl32((u32)CPU.GPR[rs], n);
		u32 m = (CPU.GPR[rb] & 0x20) ? 0 : rotate_mask[32][63 - n];

		CPU.GPR[ra] = r & m;

		if(rc) CPU.UpdateCR0<s32>(CPU.GPR[ra]);
	}
	void CNTLZW(u32 ra, u32 rs, bool rc)
	{
		u32 i;
		for(i=0; i < 32; i++)
		{
			if(CPU.GPR[rs] & (1ULL << (31 - i))) break;
		}

		CPU.GPR[ra] = i;

		if(rc) CPU.SetCRBit(CR_LT, false);
	}
	void SLD(u32 ra, u32 rs, u32 rb, bool rc)
	{
		u32 n = CPU.GPR[rb] & 0x3f;
		u64 r = rotl64(CPU.GPR[rs], n);
		u64 m = (CPU.GPR[rb] & 0x40) ? 0 : rotate_mask[0][63 - n];

		CPU.GPR[ra] = r & m;

		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void AND(u32 ra, u32 rs, u32 rb, bool rc)
	{
		CPU.GPR[ra] = CPU.GPR[rs] & CPU.GPR[rb];
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void CMPL(u32 crfd, u32 l, u32 ra, u32 rb)
	{
		CPU.UpdateCRnU(l, crfd, CPU.GPR[ra], CPU.GPR[rb]);
	}
	void LVSR(u32 vd, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];

		static const u64 lvsr_values[0x10][2] =
		{
			{0x18191A1B1C1D1E1F, 0x1011121314151617},
			{0x1718191A1B1C1D1E, 0x0F10111213141516},
			{0x161718191A1B1C1D, 0x0E0F101112131415},
			{0x15161718191A1B1C, 0x0D0E0F1011121314},
			{0x1415161718191A1B, 0x0C0D0E0F10111213},
			{0x131415161718191A, 0x0B0C0D0E0F101112},
			{0x1213141516171819, 0x0A0B0C0D0E0F1011},
			{0x1112131415161718, 0x090A0B0C0D0E0F10},
			{0x1011121314151617, 0x08090A0B0C0D0E0F},
			{0x0F10111213141516, 0x0708090A0B0C0D0E},
			{0x0E0F101112131415, 0x060708090A0B0C0D},
			{0x0D0E0F1011121314, 0x05060708090A0B0C},
			{0x0C0D0E0F10111213, 0x0405060708090A0B},
			{0x0B0C0D0E0F101112, 0x030405060708090A},
			{0x0A0B0C0D0E0F1011, 0x0203040506070809},
			{0x090A0B0C0D0E0F10, 0x0102030405060708},
		};

		CPU.VPR[vd]._u64[0] = lvsr_values[addr & 0xf][0];
		CPU.VPR[vd]._u64[1] = lvsr_values[addr & 0xf][1];
	}
	void LVEHX(u32 vd, u32 ra, u32 rb)
	{
		//const u64 addr = (ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~1ULL;
		//CPU.VPR[vd].Clear();
		//(u16&)CPU.VPR[vd]._u8[addr & 0xf] = Memory.Read16(addr);
		CPU.VPR[vd]._u128 = Memory.Read128((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~0xfULL);
	}
	void SUBF(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		CPU.GPR[rd] = CPU.GPR[rb] - CPU.GPR[ra];
		if(oe) UNK("subfo");
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void LDUX(u32 rd, u32 ra, u32 rb)
	{
		const u64 addr = CPU.GPR[ra] + CPU.GPR[rb];
		CPU.GPR[rd] = Memory.Read64(addr);
		CPU.GPR[ra] = addr;
	}
	void DCBST(u32 ra, u32 rb)
	{
		//UNK("dcbst", false);
		_mm_mfence();
	}
	void LWZUX(u32 rd, u32 ra, u32 rb)
	{
		const u64 addr = CPU.GPR[ra] + CPU.GPR[rb];
		CPU.GPR[rd] = Memory.Read32(addr);
		CPU.GPR[ra] = addr;
	}
	void CNTLZD(u32 ra, u32 rs, bool rc)
	{
		u32 i;
		for(i=0; i < 64; i++)
		{
			if(CPU.GPR[rs] & (1ULL << (63 - i))) break;
		}

		CPU.GPR[ra] = i;
		if(rc) CPU.SetCRBit(CR_LT, false);
	}
	void ANDC(u32 ra, u32 rs, u32 rb, bool rc)
	{
		CPU.GPR[ra] = CPU.GPR[rs] & ~CPU.GPR[rb];
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void LVEWX(u32 vd, u32 ra, u32 rb)
	{
		//const u64 addr = (ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~3ULL;
		//CPU.VPR[vd].Clear();
		//(u32&)CPU.VPR[vd]._u8[addr & 0xf] = Memory.Read32(addr);
		CPU.VPR[vd]._u128 = Memory.Read128((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~0xfULL);
	}
	void MULHD(u32 rd, u32 ra, u32 rb, bool rc)
	{
#ifdef _M_X64
		CPU.GPR[rd] = __mulh(CPU.GPR[ra], CPU.GPR[rb]);
#else
		//ConLog.Warning("MULHD");
		const s64 RA = CPU.GPR[ra];
		const s64 RB = CPU.GPR[rb];

		u128 RT;

		s64& lo =  (s64&)((s32*)&RT)[0];
		s64& mid = (s64&)((s32*)&RT)[1];
		s64& hi =  (s64&)((s32*)&RT)[2];

		const s64 a0 = ((s32*)&RA)[0];
		const s64 a1 = ((s32*)&RA)[1];
		const s64 b0 = ((s32*)&RB)[0];
		const s64 b1 = ((s32*)&RB)[1];

		lo = a0 * b0;
		hi = a1 * b1;

		mid += (a0 + a1) * (b0 + b1) - (lo + hi);

		CPU.GPR[rd] = RT._u64[1];
#endif

		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void MULHW(u32 rd, u32 ra, u32 rb, bool rc)
	{
		s32 a = CPU.GPR[ra];
		s32 b = CPU.GPR[rb];
		CPU.GPR[rd] = ((s64)a * (s64)b) >> 32;
		if(rc) CPU.UpdateCR0<s32>(CPU.GPR[rd]);
	}
	void LDARX(u32 rd, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];

		SMutexLocker lock(reservation.mutex);
		reservation.owner = lock.tid;
		reservation.addr = addr;
		reservation.size = 8;
		reservation.data64 = CPU.GPR[rd] = Memory.Read64(addr);
	}
	void DCBF(u32 ra, u32 rb)
	{
		//UNK("dcbf", false);
		_mm_mfence();
	}
	void LBZX(u32 rd, u32 ra, u32 rb)
	{
		CPU.GPR[rd] = Memory.Read8(ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]);
	}
	void LVX(u32 vd, u32 ra, u32 rb)
	{
		CPU.VPR[vd]._u128 = Memory.Read128((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~0xfULL);
	}
	void NEG(u32 rd, u32 ra, u32 oe, bool rc)
	{
		CPU.GPR[rd] = 0-CPU.GPR[ra];
		if(oe) UNK("nego");
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void LBZUX(u32 rd, u32 ra, u32 rb)
	{
		//if(ra == 0 || ra == rd) throw "Bad instruction [LBZUX]";

		const u64 addr = CPU.GPR[ra] + CPU.GPR[rb];
		CPU.GPR[rd] = Memory.Read8(addr);
		CPU.GPR[ra] = addr;
	}
	void NOR(u32 ra, u32 rs, u32 rb, bool rc)
	{
		CPU.GPR[ra] = ~(CPU.GPR[rs] | CPU.GPR[rb]);
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void STVEBX(u32 vs, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		const u8 eb = addr & 0xf;
		Memory.Write8(addr, CPU.VPR[vs]._u8[15 - eb]);
	}
	void SUBFE(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		const u64 RA = CPU.GPR[ra];
		const u64 RB = CPU.GPR[rb];
		CPU.GPR[rd] = ~RA + RB + CPU.XER.CA;
		CPU.XER.CA = (~RA + CPU.XER.CA > ~RB) | ((RA == 0) & CPU.XER.CA);
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
		if(oe) UNK("subfeo");
	}
	void ADDE(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		const u64 RA = CPU.GPR[ra];
		const u64 RB = CPU.GPR[rb];
		if (CPU.XER.CA)
		{
			if (RA == ~0ULL) //-1
			{
				CPU.GPR[rd] = RB;
				CPU.XER.CA = 1;
			}
			else
			{
				CPU.GPR[rd] = RA + 1 + RB;
				CPU.XER.CA = CPU.IsCarry(RA + 1, RB);
			}
		}
		else
		{
			CPU.GPR[rd] = RA + RB;
			CPU.XER.CA = CPU.IsCarry(RA, RB);
		}
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
		if(oe) UNK("addeo");
	}
	void MTOCRF(u32 l, u32 crm, u32 rs)
	{
		if(l)
		{
			u32 n = 0, count = 0;
			for(u32 i=0; i<8; ++i)
			{
				if(crm & (1 << i))
				{
					n = i;
					count++;
				}
			}

			if(count == 1)
			{
				//CR[4*n : 4*n+3] = RS[32+4*n : 32+4*n+3];
				CPU.SetCR(7 - n, (CPU.GPR[rs] >> (4*n)) & 0xf);
			}
			else
				CPU.CR.CR = 0;
		}
		else
		{
			for(u32 i=0; i<8; ++i)
			{
				if(crm & (1 << i))
				{
					CPU.SetCR(7 - i, CPU.GPR[rs] & (0xf << (i * 4)));
				}
			}
		}
	}
	void STDX(u32 rs, u32 ra, u32 rb)
	{
		Memory.Write64((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]), CPU.GPR[rs]);
	}
	void STWCX_(u32 rs, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];

		SMutexLocker lock(reservation.mutex);
		if (lock.tid == reservation.owner && reservation.addr == addr && reservation.size == 4)
		{
			// Memory.Write32(addr, CPU.GPR[rs]);
			CPU.SetCR_EQ(0, InterlockedCompareExchange((volatile long*) (Memory + addr), re((u32) CPU.GPR[rs]), re(reservation.data32)) == re(reservation.data32));
			reservation.clear();
		}
		else
		{
			CPU.SetCR_EQ(0, false);
			if (lock.tid == reservation.owner) reservation.clear();
		}
	}
	void STWX(u32 rs, u32 ra, u32 rb)
	{
		Memory.Write32((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]), CPU.GPR[rs]);
	}
	void STVEHX(u32 vs, u32 ra, u32 rb)
	{
		const u64 addr = (ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~1ULL;
		const u8 eb = (addr & 0xf) >> 1;
		Memory.Write16(addr, CPU.VPR[vs]._u16[7 - eb]);
	}
	void STDUX(u32 rs, u32 ra, u32 rb)
	{
		const u64 addr = CPU.GPR[ra] + CPU.GPR[rb];
		Memory.Write64(addr, CPU.GPR[rs]);
		CPU.GPR[ra] = addr;
	}
	void STWUX(u32 rs, u32 ra, u32 rb)
	{
		const u64 addr = CPU.GPR[ra] + CPU.GPR[rb];
		Memory.Write32(addr, CPU.GPR[rs]);
		CPU.GPR[ra] = addr;
	}
	void STVEWX(u32 vs, u32 ra, u32 rb)
	{
		const u64 addr = (ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~3ULL;
		const u8 eb = (addr & 0xf) >> 2;
		Memory.Write32(addr, CPU.VPR[vs]._u32[3 - eb]);
	}
	void ADDZE(u32 rd, u32 ra, u32 oe, bool rc)
	{
		const u64 RA = CPU.GPR[ra];
		CPU.GPR[rd] = RA + CPU.XER.CA;
		CPU.XER.CA = CPU.IsCarry(RA, CPU.XER.CA);
		if(oe) ConLog.Warning("addzeo");
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void SUBFZE(u32 rd, u32 ra, u32 oe, bool rc)
	{
		const u64 RA = CPU.GPR[ra];
		CPU.GPR[rd] = ~RA + CPU.XER.CA;
		CPU.XER.CA = ((RA == 0) & CPU.XER.CA);
		if (oe) ConLog.Warning("subfzeo");
		if (rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void STDCX_(u32 rs, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];

		SMutexLocker lock(reservation.mutex);
		if (lock.tid == reservation.owner && reservation.addr == addr && reservation.size == 8)
		{
			// Memory.Write64(addr, CPU.GPR[rs]);
			CPU.SetCR_EQ(0, InterlockedCompareExchange64((volatile long long*)(Memory + addr), re(CPU.GPR[rs]), re(reservation.data64)) == re(reservation.data64));
			reservation.clear();
		}
		else
		{
			CPU.SetCR_EQ(0, false);
			if (lock.tid == reservation.owner) reservation.clear();
		}
	}
	void STBX(u32 rs, u32 ra, u32 rb)
	{
		Memory.Write8((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]), CPU.GPR[rs]);
	}
	void STVX(u32 vs, u32 ra, u32 rb)
	{
		Memory.Write128((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~0xfULL, CPU.VPR[vs]._u128);
	}
	void SUBFME(u32 rd, u32 ra, u32 oe, bool rc)
	{
		const u64 RA = CPU.GPR[ra];
		CPU.GPR[rd] = ~RA + CPU.XER.CA + 0xFFFFFFFFFFFFFFFF;
		CPU.XER.CA = (~RA + CPU.XER.CA > ~0xFFFFFFFFFFFFFFFF) | ((RA == 0) & CPU.XER.CA);
		if (oe) ConLog.Warning("subfmeo");
		if (rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void MULLD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		CPU.GPR[rd] = (s64)((s64)CPU.GPR[ra] * (s64)CPU.GPR[rb]);
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
		if(oe) UNK("mulldo");
	}
	void ADDME(u32 rd, u32 ra, u32 oe, bool rc)
	{
		const s64 RA = CPU.GPR[ra];
		CPU.GPR[rd] = RA + CPU.XER.CA - 1;
		CPU.XER.CA |= RA != 0;

		if(oe) UNK("addmeo");
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void MULLW(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		CPU.GPR[rd] = (s64)((s64)(s32)CPU.GPR[ra] * (s64)(s32)CPU.GPR[rb]);
		if(rc) CPU.UpdateCR0<s32>(CPU.GPR[rd]);
		if(oe) UNK("mullwo");
	}
	void DCBTST(u32 th, u32 ra, u32 rb)
	{
		//UNK("dcbtst", false);
		_mm_mfence();
	}
	void STBUX(u32 rs, u32 ra, u32 rb)
	{
		const u64 addr = CPU.GPR[ra] + CPU.GPR[rb];
		Memory.Write8(addr, CPU.GPR[rs]);
		CPU.GPR[ra] = addr;
	}
	void ADD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		const u64 RA = CPU.GPR[ra];
		const u64 RB = CPU.GPR[rb];
		CPU.GPR[rd] = RA + RB;
		CPU.XER.CA = CPU.IsCarry(RA, RB);
		if(oe) UNK("addo");
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void DCBT(u32 ra, u32 rb, u32 th)
	{
		//UNK("dcbt", false);
		_mm_mfence();
	}
	void LHZX(u32 rd, u32 ra, u32 rb)
	{
		CPU.GPR[rd] = Memory.Read16(ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]);
	}
	void EQV(u32 ra, u32 rs, u32 rb, bool rc)
	{
		CPU.GPR[ra] = ~(CPU.GPR[rs] ^ CPU.GPR[rb]);
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void ECIWX(u32 rd, u32 ra, u32 rb)
	{
		//HACK!
		CPU.GPR[rd] = Memory.Read32(ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]);
	}
	void LHZUX(u32 rd, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		CPU.GPR[rd] = Memory.Read16(addr);
		CPU.GPR[ra] = addr;
	}
	void XOR(u32 ra, u32 rs, u32 rb, bool rc)
	{
		CPU.GPR[ra] = CPU.GPR[rs] ^ CPU.GPR[rb];
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void MFSPR(u32 rd, u32 spr)
	{
		CPU.GPR[rd] = GetRegBySPR(spr);
	}
	void LWAX(u32 rd, u32 ra, u32 rb)
	{
		CPU.GPR[rd] = (s64)(s32)Memory.Read32(ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]);
	}
	void DST(u32 ra, u32 rb, u32 strm, u32 t)
	{
		_mm_mfence();
	}
	void LHAX(u32 rd, u32 ra, u32 rb)
	{
		CPU.GPR[rd] = (s64)(s16)Memory.Read16(ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]);
	}
	void LVXL(u32 vd, u32 ra, u32 rb)
	{
		CPU.VPR[vd]._u128 = Memory.Read128((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~0xfULL);
	}
	void MFTB(u32 rd, u32 spr)
	{
		const u32 n = (spr >> 5) | ((spr & 0x1f) << 5);

		switch(n)
		{
		case 0x10C: CPU.GPR[rd] = CPU.TB; break;
		case 0x10D: CPU.GPR[rd] = CPU.TBH; break;
		default: UNK(fmt::Format("mftb r%d, %d", rd, spr)); break;
		}
	}
	void LWAUX(u32 rd, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		CPU.GPR[rd] = (s64)(s32)Memory.Read32(addr);
		CPU.GPR[ra] = addr;
	}
	void DSTST(u32 ra, u32 rb, u32 strm, u32 t)
	{
		_mm_mfence();
	}
	void LHAUX(u32 rd, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		CPU.GPR[rd] = (s64)(s16)Memory.Read16(addr);
		CPU.GPR[ra] = addr;
	}
	void STHX(u32 rs, u32 ra, u32 rb)
	{
		Memory.Write16(ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb], CPU.GPR[rs]);
	}
	void ORC(u32 ra, u32 rs, u32 rb, bool rc)
	{
		CPU.GPR[ra] = CPU.GPR[rs] | ~CPU.GPR[rb];
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void ECOWX(u32 rs, u32 ra, u32 rb)
	{
		//HACK!
		Memory.Write32((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]), CPU.GPR[rs]);
	}
	void STHUX(u32 rs, u32 ra, u32 rb)
	{
		const u64 addr = CPU.GPR[ra] + CPU.GPR[rb];
		Memory.Write16(addr, CPU.GPR[rs]);
		CPU.GPR[ra] = addr;
	}
	void OR(u32 ra, u32 rs, u32 rb, bool rc)
	{
		CPU.GPR[ra] = CPU.GPR[rs] | CPU.GPR[rb];
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void DIVDU(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		const u64 RA = CPU.GPR[ra];
		const u64 RB = CPU.GPR[rb];

		if(RB == 0)
		{
			if(oe) UNK("divduo");
			CPU.GPR[rd] = 0;
		}
		else
		{
			CPU.GPR[rd] = RA / RB;
		}

		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void DIVWU(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		const u32 RA = CPU.GPR[ra];
		const u32 RB = CPU.GPR[rb];

		if(RB == 0)
		{
			if(oe) UNK("divwuo");
			CPU.GPR[rd] = 0;
		}
		else
		{
			CPU.GPR[rd] = RA / RB;
		}

		if(rc) CPU.UpdateCR0<s32>(CPU.GPR[rd]);
	}
	void MTSPR(u32 spr, u32 rs)
	{
		GetRegBySPR(spr) = CPU.GPR[rs];
	}
	/*0x1d6*///DCBI
	void NAND(u32 ra, u32 rs, u32 rb, bool rc)
	{
		CPU.GPR[ra] = ~(CPU.GPR[rs] & CPU.GPR[rb]);

		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void STVXL(u32 vs, u32 ra, u32 rb)
	{
		Memory.Write128((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~0xfULL, CPU.VPR[vs]._u128);
	}
	void DIVD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		const s64 RA = CPU.GPR[ra];
		const s64 RB = CPU.GPR[rb];

		if (RB == 0 || ((u64)RA == (1ULL << 63) && RB == -1))
		{
			if(oe) UNK("divdo");
			CPU.GPR[rd] = /*(((u64)RA & (1ULL << 63)) && RB == 0) ? -1 :*/ 0;
		}
		else
		{
			CPU.GPR[rd] = RA / RB;
		}

		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void DIVW(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		const s32 RA = CPU.GPR[ra];
		const s32 RB = CPU.GPR[rb];

		if (RB == 0 || ((u32)RA == (1 << 31) && RB == -1))
		{
			if(oe) UNK("divwo");
			CPU.GPR[rd] = /*(((u32)RA & (1 << 31)) && RB == 0) ? -1 :*/ 0;
		}
		else
		{
			CPU.GPR[rd] = (u32)(RA / RB);
		}

		if(rc) CPU.UpdateCR0<s32>(CPU.GPR[rd]);
	}
	void LVLX(u32 vd, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		const u8 eb = addr & 0xf;

		Memory.ReadLeft(CPU.VPR[vd]._u8 + eb, addr, 16 - eb);
	}
	void LDBRX(u32 rd, u32 ra, u32 rb)
	{
		CPU.GPR[rd] = (u64&)Memory[ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]];
	}
	void LWBRX(u32 rd, u32 ra, u32 rb)
	{
		CPU.GPR[rd] = (u32&)Memory[ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]];
	}
	void LFSX(u32 frd, u32 ra, u32 rb)
	{
		(u32&)CPU.FPR[frd] = Memory.Read32(ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]);
		CPU.FPR[frd] = (float&)CPU.FPR[frd];
	}
	void SRW(u32 ra, u32 rs, u32 rb, bool rc)
	{
		u32 n = CPU.GPR[rb] & 0x1f;
		u32 r = rotl32((u32)CPU.GPR[rs], 64 - n);
		u32 m = (CPU.GPR[rb] & 0x20) ? 0 : rotate_mask[32 + n][63];
		CPU.GPR[ra] = r & m;
		
		if(rc) CPU.UpdateCR0<s32>(CPU.GPR[ra]);
	}
	void SRD(u32 ra, u32 rs, u32 rb, bool rc)
	{
		u32 n = CPU.GPR[rb] & 0x3f;
		u64 r = rotl64(CPU.GPR[rs], 64 - n);
		u64 m = (CPU.GPR[rb] & 0x40) ? 0 : rotate_mask[n][63];
		CPU.GPR[ra] = r & m;
		
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void LVRX(u32 vd, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		const u8 eb = addr & 0xf;

		Memory.ReadRight(CPU.VPR[vd]._u8, addr & ~0xf, eb);
	}
	void LSWI(u32 rd, u32 ra, u32 nb)
	{
		u64 EA = ra ? CPU.GPR[ra] : 0;
		u64 N = nb ? nb : 32;
		u8 reg = CPU.GPR[rd];

		while (N > 0)
		{
			if (N > 3)
			{
				CPU.GPR[reg] = Memory.Read32(EA);
				EA += 4;
				N -= 4;
			}
			else
			{
				u32 buf = 0;
				while (N > 0)
				{
					N = N - 1;
					buf |= Memory.Read8(EA) <<(N*8) ;
					EA = EA + 1;
				}
				CPU.GPR[reg] = buf;
			}
			reg = (reg + 1) % 32;
		}
	}
	void LFSUX(u32 frd, u32 ra, u32 rb)
	{
		const u64 addr = CPU.GPR[ra] + CPU.GPR[rb];
		(u64&)CPU.FPR[frd] = Memory.Read32(addr);
		CPU.FPR[frd] = (float&)CPU.FPR[frd];
		CPU.GPR[ra] = addr;
	}
	void SYNC(u32 l)
	{
		_mm_mfence();
	}
	void LFDX(u32 frd, u32 ra, u32 rb)
	{
		(u64&)CPU.FPR[frd] = Memory.Read64(ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]);
	}
	void LFDUX(u32 frd, u32 ra, u32 rb)
	{
		const u64 addr = CPU.GPR[ra] + CPU.GPR[rb];
		(u64&)CPU.FPR[frd] = Memory.Read64(addr);
		CPU.GPR[ra] = addr;
	}
	void STVLX(u32 vs, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		const u8 eb = addr & 0xf;

		Memory.WriteLeft(addr, 16 - eb, CPU.VPR[vs]._u8 + eb);
	}
	void STWBRX(u32 rs, u32 ra, u32 rb)
	{
		(u32&)Memory[ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]] = CPU.GPR[rs];
	}
	void STFSX(u32 frs, u32 ra, u32 rb)
	{
		Memory.Write32((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]), CPU.FPR[frs].To32());
	}
	void STVRX(u32 vs, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		const u8 eb = addr & 0xf;

		Memory.WriteRight(addr - eb, eb, CPU.VPR[vs]._u8);
	}
	void STSWI(u32 rd, u32 ra, u32 nb)
	{
			u64 EA = ra ? CPU.GPR[ra] : 0;
			u64 N = nb ? nb : 32;
			u8 reg = CPU.GPR[rd];

			while (N > 0)
			{
				if (N > 3)
				{
					Memory.Write32(EA, CPU.GPR[reg]);
					EA += 4;
					N -= 4;
				}
				else
				{
					u32 buf = CPU.GPR[reg];
					while (N > 0)
					{
						N = N - 1;
						Memory.Write8(EA, (0xFF000000 & buf) >> 24);
						buf <<= 8;
						EA = EA + 1;
					}
				}
				reg = (reg + 1) % 32;
			}
	}
	void STFDX(u32 frs, u32 ra, u32 rb)
	{
		Memory.Write64((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]), (u64&)CPU.FPR[frs]);
	}
	void LVLXL(u32 vd, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		const u8 eb = addr & 0xf;

		Memory.ReadLeft(CPU.VPR[vd]._u8 + eb, addr, 16 - eb);
	}
	void LHBRX(u32 rd, u32 ra, u32 rb)
	{
		CPU.GPR[rd] = (u16&)Memory[ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]];
	}
	void SRAW(u32 ra, u32 rs, u32 rb, bool rc)
	{
		s32 RS = CPU.GPR[rs];
		u8 shift = CPU.GPR[rb] & 63;
		if (shift > 31)
		{
			CPU.GPR[ra] = 0 - (RS < 0);
			CPU.XER.CA = (RS < 0);
		}
		else
		{
			CPU.GPR[ra] = RS >> shift;
			CPU.XER.CA = (RS < 0) & ((CPU.GPR[ra] << shift) != RS);
		}

		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void SRAD(u32 ra, u32 rs, u32 rb, bool rc)
	{
		s64 RS = CPU.GPR[rs];
		u8 shift = CPU.GPR[rb] & 127;
		if (shift > 63)
		{
			CPU.GPR[ra] = 0 - (RS < 0);
			CPU.XER.CA = (RS < 0);
		}
		else
		{
			CPU.GPR[ra] = RS >> shift;
			CPU.XER.CA = (RS < 0) & ((CPU.GPR[ra] << shift) != RS);
		}

		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void LVRXL(u32 vd, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		const u8 eb = addr & 0xf;

		Memory.ReadRight(CPU.VPR[vd]._u8, addr & ~0xf, eb);
	}
	void DSS(u32 strm, u32 a)
	{
		_mm_mfence();
	}
	void SRAWI(u32 ra, u32 rs, u32 sh, bool rc)
	{
		s32 RS = (u32)CPU.GPR[rs];
		CPU.GPR[ra] = RS >> sh;
		CPU.XER.CA = (RS < 0) & ((u32)(CPU.GPR[ra] << sh) != RS);

		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void SRADI1(u32 ra, u32 rs, u32 sh, bool rc)
	{
		s64 RS = CPU.GPR[rs];
		CPU.GPR[ra] = RS >> sh;
		CPU.XER.CA = (RS < 0) & ((CPU.GPR[ra] << sh) != RS);

		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void SRADI2(u32 ra, u32 rs, u32 sh, bool rc)
	{
		SRADI1(ra, rs, sh, rc);
	}
	void EIEIO()
	{
		_mm_mfence();
	}
	void STVLXL(u32 vs, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		const u8 eb = addr & 0xf;

		Memory.WriteLeft(addr, 16 - eb, CPU.VPR[vs]._u8 + eb);
	}
	void STHBRX(u32 rs, u32 ra, u32 rb)
	{
		(u16&)Memory[ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]] = CPU.GPR[rs];
	}
	void EXTSH(u32 ra, u32 rs, bool rc)
	{
		CPU.GPR[ra] = (s64)(s16)CPU.GPR[rs];
		if(rc) CPU.UpdateCR0<s16>(CPU.GPR[ra]);
	}
	void STVRXL(u32 vs, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		const u8 eb = addr & 0xf;

		Memory.WriteRight(addr - eb, eb, CPU.VPR[vs]._u8);
	}
	void EXTSB(u32 ra, u32 rs, bool rc)
	{
		CPU.GPR[ra] = (s64)(s8)CPU.GPR[rs];
		if(rc) CPU.UpdateCR0<s8>(CPU.GPR[ra]);
	}
	void STFIWX(u32 frs, u32 ra, u32 rb)
	{
		Memory.Write32(ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb], (u32&)CPU.FPR[frs]);
	}
	void EXTSW(u32 ra, u32 rs, bool rc)
	{
		CPU.GPR[ra] = (s64)(s32)CPU.GPR[rs];
		//CPU.XER.CA = ((s64)CPU.GPR[ra] < 0); // ???
		if(rc) CPU.UpdateCR0<s32>(CPU.GPR[ra]);
	}
	/*0x3d6*///ICBI
	void DCBZ(u32 ra, u32 rs)
	{
		//UNK("dcbz", false);
		_mm_mfence();
	}
	void LWZ(u32 rd, u32 ra, s32 d)
	{
		CPU.GPR[rd] = Memory.Read32(ra ? CPU.GPR[ra] + d : d);
	}
	void LWZU(u32 rd, u32 ra, s32 d)
	{
		const u64 addr = CPU.GPR[ra] + d;
		CPU.GPR[rd] = Memory.Read32(addr);
		CPU.GPR[ra] = addr;
	}
	void LBZ(u32 rd, u32 ra, s32 d)
	{
		CPU.GPR[rd] = Memory.Read8(ra ? CPU.GPR[ra] + d : d);
	}
	void LBZU(u32 rd, u32 ra, s32 d)
	{
		const u64 addr = CPU.GPR[ra] + d;
		CPU.GPR[rd] = Memory.Read8(addr);
		CPU.GPR[ra] = addr;
	}
	void STW(u32 rs, u32 ra, s32 d)
	{
		Memory.Write32(ra ? CPU.GPR[ra] + d : d, CPU.GPR[rs]);
	}
	void STWU(u32 rs, u32 ra, s32 d)
	{
		const u64 addr = CPU.GPR[ra] + d;
		Memory.Write32(addr, CPU.GPR[rs]);
		CPU.GPR[ra] = addr;
	}
	void STB(u32 rs, u32 ra, s32 d)
	{
		Memory.Write8(ra ? CPU.GPR[ra] + d : d, CPU.GPR[rs]);
	}
	void STBU(u32 rs, u32 ra, s32 d)
	{
		const u64 addr = CPU.GPR[ra] + d;
		Memory.Write8(addr, CPU.GPR[rs]);
		CPU.GPR[ra] = addr;
	}
	void LHZ(u32 rd, u32 ra, s32 d)
	{
		CPU.GPR[rd] = Memory.Read16(ra ? CPU.GPR[ra] + d : d);
	}
	void LHZU(u32 rd, u32 ra, s32 d)
	{
		const u64 addr = CPU.GPR[ra] + d;
		CPU.GPR[rd] = Memory.Read16(addr);
		CPU.GPR[ra] = addr;
	}
	void LHA(u32 rd, u32 ra, s32 d)
	{
		CPU.GPR[rd] = (s64)(s16)Memory.Read16(ra ? CPU.GPR[ra] + d : d);
	}
	void LHAU(u32 rd, u32 ra, s32 d)
	{
		const u64 addr = CPU.GPR[ra] + d;
		CPU.GPR[rd] = (s64)(s16)Memory.Read16(addr);
		CPU.GPR[ra] = addr;
	}
	void STH(u32 rs, u32 ra, s32 d)
	{
		Memory.Write16(ra ? CPU.GPR[ra] + d : d, CPU.GPR[rs]);
	}
	void STHU(u32 rs, u32 ra, s32 d)
	{
		const u64 addr = CPU.GPR[ra] + d;
		Memory.Write16(addr, CPU.GPR[rs]);
		CPU.GPR[ra] = addr;
	}
	void LMW(u32 rd, u32 ra, s32 d)
	{
		u64 addr = ra ? CPU.GPR[ra] + d : d;
		for(u32 i=rd; i<32; ++i, addr += 4)
		{
			CPU.GPR[i] = Memory.Read32(addr);
		}
	}
	void STMW(u32 rs, u32 ra, s32 d)
	{
		u64 addr = ra ? CPU.GPR[ra] + d : d;
		for(u32 i=rs; i<32; ++i, addr += 4)
		{
			Memory.Write32(addr, CPU.GPR[i]);
		}
	}
	void LFS(u32 frd, u32 ra, s32 d)
	{
		const u32 v = Memory.Read32(ra ? CPU.GPR[ra] + d : d);
		CPU.FPR[frd] = (float&)v;
	}
	void LFSU(u32 frd, u32 ra, s32 ds)
	{
		const u64 addr = CPU.GPR[ra] + ds;
		const u32 v = Memory.Read32(addr);
		CPU.FPR[frd] = (float&)v;
		CPU.GPR[ra] = addr;
	}
	void LFD(u32 frd, u32 ra, s32 d)
	{
		(u64&)CPU.FPR[frd] = Memory.Read64(ra ? CPU.GPR[ra] + d : d);
	}
	void LFDU(u32 frd, u32 ra, s32 ds)
	{
		const u64 addr = CPU.GPR[ra] + ds;
		(u64&)CPU.FPR[frd] = Memory.Read64(addr);
		CPU.GPR[ra] = addr;
	}
	void STFS(u32 frs, u32 ra, s32 d)
	{
		Memory.Write32(ra ? CPU.GPR[ra] + d : d, CPU.FPR[frs].To32());
	}
	void STFSU(u32 frs, u32 ra, s32 d)
	{
		const u64 addr = CPU.GPR[ra] + d;
		Memory.Write32(addr, CPU.FPR[frs].To32());
		CPU.GPR[ra] = addr;
	}
	void STFD(u32 frs, u32 ra, s32 d)
	{
		Memory.Write64(ra ? CPU.GPR[ra] + d : d, (u64&)CPU.FPR[frs]);
	}
	void STFDU(u32 frs, u32 ra, s32 d)
	{
		const u64 addr = CPU.GPR[ra] + d;
		Memory.Write64(addr, (u64&)CPU.FPR[frs]);
		CPU.GPR[ra] = addr;
	}
	void LD(u32 rd, u32 ra, s32 ds)
	{
		CPU.GPR[rd] = Memory.Read64(ra ? CPU.GPR[ra] + ds : ds);
	}
	void LDU(u32 rd, u32 ra, s32 ds)
	{
		//if(ra == 0 || rt == ra) return;
		const u64 addr = CPU.GPR[ra] + ds;
		CPU.GPR[rd] = Memory.Read64(addr);
		CPU.GPR[ra] = addr;
	}
	void LWA(u32 rd, u32 ra, s32 ds)
	{
		CPU.GPR[rd] = (s64)(s32)Memory.Read32(ra ? CPU.GPR[ra] + ds : ds);
	}
	void FDIVS(u32 frd, u32 fra, u32 frb, bool rc)
	{
		if(FPRdouble::IsNaN(CPU.FPR[fra]))
		{
			CPU.FPR[frd] = CPU.FPR[fra];
		}
		else if(FPRdouble::IsNaN(CPU.FPR[frb]))
		{
			CPU.FPR[frd] = CPU.FPR[frb];
		}
		else
		{
			if(CPU.FPR[frb] == 0.0)
			{
				if(CPU.FPR[fra] == 0.0)
				{
					CPU.FPSCR.VXZDZ = true;
					CPU.FPR[frd] = FPR_NAN;
				}
				else
				{
					CPU.FPR[frd] = (float)(CPU.FPR[fra] / CPU.FPR[frb]);
				}

				CPU.FPSCR.ZX = true;
			}
			else if(FPRdouble::IsINF(CPU.FPR[fra]) && FPRdouble::IsINF(CPU.FPR[frb]))
			{
				CPU.FPSCR.VXIDI = true;
				CPU.FPR[frd] = FPR_NAN;
			}
			else
			{
				CPU.FPR[frd] = (float)(CPU.FPR[fra] / CPU.FPR[frb]);
			}
		}

		CPU.FPSCR.FPRF = CPU.FPR[frd].GetType();
		if(rc) UNK("fdivs.");//CPU.UpdateCR1(CPU.FPR[frd]);
	}
	void FSUBS(u32 frd, u32 fra, u32 frb, bool rc)
	{
		CPU.FPR[frd] = static_cast<float>(CPU.FPR[fra] - CPU.FPR[frb]);
		CPU.FPSCR.FPRF = CPU.FPR[frd].GetType();
		if(rc) UNK("fsubs.");//CPU.UpdateCR1(CPU.FPR[frd]);
	}
	void FADDS(u32 frd, u32 fra, u32 frb, bool rc)
	{
		CPU.FPR[frd] = static_cast<float>(CPU.FPR[fra] + CPU.FPR[frb]);
		CPU.FPSCR.FPRF = CPU.FPR[frd].GetType();
		if(rc) UNK("fadds.");//CPU.UpdateCR1(CPU.FPR[frd]);
	}
	void FSQRTS(u32 frd, u32 frb, bool rc)
	{
		CPU.FPR[frd] = static_cast<float>(sqrt(CPU.FPR[frb]));
		CPU.FPSCR.FPRF = CPU.FPR[frd].GetType();
		if(rc) UNK("fsqrts.");//CPU.UpdateCR1(CPU.FPR[frd]);
	}
	void FRES(u32 frd, u32 frb, bool rc)
	{
		if(CPU.FPR[frb] == 0.0)
		{
			CPU.SetFPSCRException(FPSCR_ZX);
		}
		CPU.FPR[frd] = static_cast<float>(1.0 / CPU.FPR[frb]);
		if(rc) UNK("fres.");//CPU.UpdateCR1(CPU.FPR[frd]);
	}
	void FMULS(u32 frd, u32 fra, u32 frc, bool rc)
	{
		CPU.FPR[frd] = static_cast<float>(CPU.FPR[fra] * CPU.FPR[frc]);
		CPU.FPSCR.FI = 0;
		CPU.FPSCR.FR = 0;
		CPU.FPSCR.FPRF = CPU.FPR[frd].GetType();
		if(rc) UNK("fmuls.");//CPU.UpdateCR1(CPU.FPR[frd]);
	}
	void FMADDS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc)
	{
		CPU.FPR[frd] = static_cast<float>(CPU.FPR[fra] * CPU.FPR[frc] + CPU.FPR[frb]);
		CPU.FPSCR.FPRF = CPU.FPR[frd].GetType();
		if(rc) UNK("fmadds.");//CPU.UpdateCR1(CPU.FPR[frd]);
	}
	void FMSUBS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc)
	{
		CPU.FPR[frd] = static_cast<float>(CPU.FPR[fra] * CPU.FPR[frc] - CPU.FPR[frb]);
		CPU.FPSCR.FPRF = CPU.FPR[frd].GetType();
		if(rc) UNK("fmsubs.");//CPU.UpdateCR1(CPU.FPR[frd]);
	}
	void FNMSUBS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc)
	{
		CPU.FPR[frd] = static_cast<float>(-(CPU.FPR[fra] * CPU.FPR[frc] - CPU.FPR[frb]));
		CPU.FPSCR.FPRF = CPU.FPR[frd].GetType();
		if(rc) UNK("fnmsubs.");////CPU.UpdateCR1(CPU.FPR[frd]);
	}
	void FNMADDS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc)
	{
		CPU.FPR[frd] = static_cast<float>(-(CPU.FPR[fra] * CPU.FPR[frc] + CPU.FPR[frb]));
		CPU.FPSCR.FPRF = CPU.FPR[frd].GetType();
		if(rc) UNK("fnmadds.");//CPU.UpdateCR1(CPU.FPR[frd]);
	}
	void STD(u32 rs, u32 ra, s32 d)
	{
		Memory.Write64(ra ? CPU.GPR[ra] + d : d, CPU.GPR[rs]);
	}
	void STDU(u32 rs, u32 ra, s32 ds)
	{
		//if(ra == 0 || rs == ra) return;
		const u64 addr = CPU.GPR[ra] + ds;
		Memory.Write64(addr, CPU.GPR[rs]);
		CPU.GPR[ra] = addr;
	}
	void MTFSB1(u32 crbd, bool rc)
	{
		u64 mask = (1ULL << crbd);
		if ((crbd == 29) && !CPU.FPSCR.NI) ConLog.Warning("Non-IEEE mode enabled");
		CPU.FPSCR.FPSCR |= mask;

		if(rc) UNIMPLEMENTED();
	}
	void MCRFS(u32 crbd, u32 crbs)
	{
		u64 mask = (1ULL << crbd);
		CPU.CR.CR &= ~mask;
		CPU.CR.CR |= CPU.FPSCR.FPSCR & mask;
	}
	void MTFSB0(u32 crbd, bool rc)
	{
		u64 mask = (1ULL << crbd);
		if ((crbd == 29) && !CPU.FPSCR.NI) ConLog.Warning("Non-IEEE mode disabled");
		CPU.FPSCR.FPSCR &= ~mask;

		if(rc) UNIMPLEMENTED();
	}
	void MTFSFI(u32 crfd, u32 i, bool rc)
	{
		u64 mask = (0x1ULL << crfd);

		if(i)
		{
			if ((crfd == 29) && !CPU.FPSCR.NI) ConLog.Warning("Non-IEEE mode enabled");
			CPU.FPSCR.FPSCR |= mask;
		}
		else
		{
			if ((crfd == 29) && CPU.FPSCR.NI) ConLog.Warning("Non-IEEE mode disabled");
			CPU.FPSCR.FPSCR &= ~mask;
		}

		if(rc) UNIMPLEMENTED();
	}
	void MFFS(u32 frd, bool rc)
	{
		(u64&)CPU.FPR[frd] = CPU.FPSCR.FPSCR;
		if(rc) UNIMPLEMENTED();
	}
	void MTFSF(u32 flm, u32 frb, bool rc)
	{
		u32 mask = 0;
		for(u32 i=0; i<8; ++i)
		{
			if(flm & (1 << i)) mask |= 0xf << (i * 4);
		}

		const u32 oldNI = CPU.FPSCR.NI;
		CPU.FPSCR.FPSCR = (CPU.FPSCR.FPSCR & ~mask) | ((u32&)CPU.FPR[frb] & mask);
		if (CPU.FPSCR.NI != oldNI)
		{
			if (oldNI)
				ConLog.Warning("Non-IEEE mode disabled");
			else
				ConLog.Warning("Non-IEEE mode enabled");
		}
		if(rc) UNK("mtfsf.");
	}
	void FCMPU(u32 crfd, u32 fra, u32 frb)
	{
		int cmp_res = FPRdouble::Cmp(CPU.FPR[fra], CPU.FPR[frb]);

		if(cmp_res == CR_SO)
		{
			if(FPRdouble::IsSNaN(CPU.FPR[fra]) || FPRdouble::IsSNaN(CPU.FPR[frb]))
			{
				CPU.SetFPSCRException(FPSCR_VXSNAN);
			}
		}

		CPU.FPSCR.FPRF = cmp_res;
		CPU.SetCR(crfd, cmp_res);
	}
	void FRSP(u32 frd, u32 frb, bool rc)
	{
		const double b = CPU.FPR[frb];
		double b0 = b;
		if(CPU.FPSCR.NI)
		{
			if (((u64&)b0 & DOUBLE_EXP) < 0x3800000000000000ULL) (u64&)b0 &= DOUBLE_SIGN;
		}
		const double r = static_cast<float>(b0);
		CPU.FPSCR.FR = fabs(r) > fabs(b);
		CPU.SetFPSCR_FI(b != r);
		CPU.FPSCR.FPRF = PPCdouble(r).GetType();
		CPU.FPR[frd] = r;
	}
	void FCTIW(u32 frd, u32 frb, bool rc)
	{
		const double b = CPU.FPR[frb];
		u32 r;
		if(b > (double)0x7fffffff)
		{
			r = 0x7fffffff;
			CPU.SetFPSCRException(FPSCR_VXCVI);
			CPU.FPSCR.FI = 0;
			CPU.FPSCR.FR = 0;
		}
		else if (b < -(double)0x80000000)
		{
			r = 0x80000000;
			CPU.SetFPSCRException(FPSCR_VXCVI);
			CPU.FPSCR.FI = 0;
			CPU.FPSCR.FR = 0;
		}
		else
		{
			s32 i = 0;
			switch(CPU.FPSCR.RN)
			{
			case FPSCR_RN_NEAR:
				{
					double t = b + 0.5;
					i = (s32)t;
					if (t - i < 0 || (t - i == 0 && b > 0)) i--;
					break;
				}
			case FPSCR_RN_ZERO:
				i = (s32)b;
				break;
			case FPSCR_RN_PINF:
				i = (s32)b;
				if (b - i > 0) i++;
				break;
			case FPSCR_RN_MINF:
				i = (s32)b;
				if (b - i < 0) i--;
				break;
			}
			r = (u32)i;
			double di = i;
			if (di == b)
			{
				CPU.SetFPSCR_FI(0);
				CPU.FPSCR.FR = 0;
			}
			else
			{
				CPU.SetFPSCR_FI(1);
				CPU.FPSCR.FR = fabs(di) > fabs(b);
			}
		}

		(u64&)CPU.FPR[frd] = 0xfff8000000000000ull | r;
		if(r == 0 && ( (u64&)b & DOUBLE_SIGN )) (u64&)CPU.FPR[frd] |= 0x100000000ull;

		if(rc) UNK("fctiw.");
	}
	void FCTIWZ(u32 frd, u32 frb, bool rc)
	{
		const double b = CPU.FPR[frb];
		u32 value;
		if (b > (double)0x7fffffff)
		{
			value = 0x7fffffff;
			CPU.SetFPSCRException(FPSCR_VXCVI);
			CPU.FPSCR.FI = 0;
			CPU.FPSCR.FR = 0;
		}
		else if (b < -(double)0x80000000)
		{
			value = 0x80000000;
			CPU.SetFPSCRException(FPSCR_VXCVI);
			CPU.FPSCR.FI = 0;
			CPU.FPSCR.FR = 0;
		}
		else
		{
			s32 i = (s32)b;
			double di = i;
			if (di == b)
			{
				CPU.SetFPSCR_FI(0);
				CPU.FPSCR.FR = 0;
			}
			else
			{
				CPU.SetFPSCR_FI(1);
				CPU.FPSCR.FR = fabs(di) > fabs(b);
			}
			value = (u32)i;
		}

		(u64&)CPU.FPR[frd] = 0xfff8000000000000ull | value;
		if (value == 0 && ( (u64&)b & DOUBLE_SIGN ))
			(u64&)CPU.FPR[frd] |= 0x100000000ull;

		if(rc) UNK("fctiwz.");
	}
	void FDIV(u32 frd, u32 fra, u32 frb, bool rc)
	{
		double res;

		if(FPRdouble::IsNaN(CPU.FPR[fra]))
		{
			res = CPU.FPR[fra];
		}
		else if(FPRdouble::IsNaN(CPU.FPR[frb]))
		{
			res = CPU.FPR[frb];
		}
		else
		{
			if(CPU.FPR[frb] == 0.0)
			{
				if(CPU.FPR[fra] == 0.0)
				{
					CPU.FPSCR.VXZDZ = 1;
					res = FPR_NAN;
				}
				else
				{
					res = CPU.FPR[fra] / CPU.FPR[frb];
				}

				CPU.SetFPSCRException(FPSCR_ZX);
			}
			else if(FPRdouble::IsINF(CPU.FPR[fra]) && FPRdouble::IsINF(CPU.FPR[frb]))
			{
				CPU.FPSCR.VXIDI = 1;
				res = FPR_NAN;
			}
			else
			{
				res = CPU.FPR[fra] / CPU.FPR[frb];
			}
		}

		CPU.FPR[frd] = res;
		CPU.FPSCR.FPRF = CPU.FPR[frd].GetType();
		if(rc) UNK("fdiv.");//CPU.UpdateCR1(CPU.FPR[frd]);
	}
	void FSUB(u32 frd, u32 fra, u32 frb, bool rc)
	{
		CPU.FPR[frd] = CPU.FPR[fra] - CPU.FPR[frb];
		CPU.FPSCR.FPRF = CPU.FPR[frd].GetType();
		if(rc) UNK("fsub.");//CPU.UpdateCR1(CPU.FPR[frd]);
	}
	void FADD(u32 frd, u32 fra, u32 frb, bool rc)
	{
		CPU.FPR[frd] = CPU.FPR[fra] + CPU.FPR[frb];
		CPU.FPSCR.FPRF = CPU.FPR[frd].GetType();
		if(rc) UNK("fadd.");//CPU.UpdateCR1(CPU.FPR[frd]);
	}
	void FSQRT(u32 frd, u32 frb, bool rc)
	{
		CPU.FPR[frd] = sqrt(CPU.FPR[frb]);
		CPU.FPSCR.FPRF = CPU.FPR[frd].GetType();
		if(rc) UNK("fsqrt.");//CPU.UpdateCR1(CPU.FPR[frd]);
	}
	void FSEL(u32 frd, u32 fra, u32 frc, u32 frb, bool rc)
	{
		CPU.FPR[frd] = CPU.FPR[fra] >= 0.0 ? CPU.FPR[frc] : CPU.FPR[frb];
		if(rc) UNK("fsel.");//CPU.UpdateCR1(CPU.FPR[frd]);
	}
	void FMUL(u32 frd, u32 fra, u32 frc, bool rc)
	{
		if((FPRdouble::IsINF(CPU.FPR[fra]) && CPU.FPR[frc] == 0.0) || (FPRdouble::IsINF(CPU.FPR[frc]) && CPU.FPR[fra] == 0.0))
		{
			CPU.SetFPSCRException(FPSCR_VXIMZ);
			CPU.FPR[frd] = FPR_NAN;
			CPU.FPSCR.FI = 0;
			CPU.FPSCR.FR = 0;
			CPU.FPSCR.FPRF = FPR_QNAN;
		}
		else
		{
			if(FPRdouble::IsSNaN(CPU.FPR[fra]) || FPRdouble::IsSNaN(CPU.FPR[frc]))
			{
				CPU.SetFPSCRException(FPSCR_VXSNAN);
			}

			CPU.FPR[frd] = CPU.FPR[fra] * CPU.FPR[frc];
			CPU.FPSCR.FPRF = CPU.FPR[frd].GetType();
		}

		if(rc) UNK("fmul.");//CPU.UpdateCR1(CPU.FPR[frd]);
	}
	void FRSQRTE(u32 frd, u32 frb, bool rc)
	{
		if(CPU.FPR[frb] == 0.0)
		{
			CPU.SetFPSCRException(FPSCR_ZX);
		}
		CPU.FPR[frd] = static_cast<float>(1.0 / sqrt(CPU.FPR[frb]));
		if(rc) UNK("frsqrte.");//CPU.UpdateCR1(CPU.FPR[frd]);
	}
	void FMSUB(u32 frd, u32 fra, u32 frc, u32 frb, bool rc)
	{
		CPU.FPR[frd] = CPU.FPR[fra] * CPU.FPR[frc] - CPU.FPR[frb];
		CPU.FPSCR.FPRF = CPU.FPR[frd].GetType();
		if(rc) UNK("fmsub.");//CPU.UpdateCR1(CPU.FPR[frd]);
	}
	void FMADD(u32 frd, u32 fra, u32 frc, u32 frb, bool rc)
	{
		CPU.FPR[frd] = CPU.FPR[fra] * CPU.FPR[frc] + CPU.FPR[frb];
		CPU.FPSCR.FPRF = CPU.FPR[frd].GetType();
		if(rc) UNK("fmadd.");//CPU.UpdateCR1(CPU.FPR[frd]);
	}
	void FNMSUB(u32 frd, u32 fra, u32 frc, u32 frb, bool rc)
	{
		CPU.FPR[frd] = -(CPU.FPR[fra] * CPU.FPR[frc] - CPU.FPR[frb]);
		CPU.FPSCR.FPRF = CPU.FPR[frd].GetType();
		if(rc) UNK("fnmsub.");//CPU.UpdateCR1(CPU.FPR[frd]);
	}
	void FNMADD(u32 frd, u32 fra, u32 frc, u32 frb, bool rc)
	{
		CPU.FPR[frd] = -(CPU.FPR[fra] * CPU.FPR[frc] + CPU.FPR[frb]);
		CPU.FPSCR.FPRF = CPU.FPR[frd].GetType();
		if(rc) UNK("fnmadd.");//CPU.UpdateCR1(CPU.FPR[frd]);
	}
	void FCMPO(u32 crfd, u32 fra, u32 frb)
	{
		int cmp_res = FPRdouble::Cmp(CPU.FPR[fra], CPU.FPR[frb]);

		if(cmp_res == CR_SO)
		{
			if(FPRdouble::IsSNaN(CPU.FPR[fra]) || FPRdouble::IsSNaN(CPU.FPR[frb]))
			{
				CPU.SetFPSCRException(FPSCR_VXSNAN);
				if(!CPU.FPSCR.VE) CPU.SetFPSCRException(FPSCR_VXVC);
			}
			else
			{
				CPU.SetFPSCRException(FPSCR_VXVC);
			}

			CPU.FPSCR.FX = 1;
		}

		CPU.FPSCR.FPRF = cmp_res;
		CPU.SetCR(crfd, cmp_res);
	}
	void FNEG(u32 frd, u32 frb, bool rc)
	{
		CPU.FPR[frd] = -CPU.FPR[frb];
		if(rc) UNK("fneg.");//CPU.UpdateCR1(CPU.FPR[frd]);
	}
	void FMR(u32 frd, u32 frb, bool rc)
	{
		CPU.FPR[frd] = CPU.FPR[frb];
		if(rc) UNK("fmr.");//CPU.UpdateCR1(CPU.FPR[frd]);
	}
	void FNABS(u32 frd, u32 frb, bool rc)
	{
		CPU.FPR[frd] = -fabs(CPU.FPR[frb]);
		if(rc) UNK("fnabs.");//CPU.UpdateCR1(CPU.FPR[frd]);
	}
	void FABS(u32 frd, u32 frb, bool rc)
	{
		CPU.FPR[frd] = fabs(CPU.FPR[frb]);
		if(rc) UNK("fabs.");//CPU.UpdateCR1(CPU.FPR[frd]);
	}
	void FCTID(u32 frd, u32 frb, bool rc)
	{
		const double b = CPU.FPR[frb];
		u64 r;
		if(b > (double)0x7fffffffffffffff)
		{
			r = 0x7fffffffffffffff;
			CPU.SetFPSCRException(FPSCR_VXCVI);
			CPU.FPSCR.FI = 0;
			CPU.FPSCR.FR = 0;
		}
		else if (b < -(double)0x8000000000000000)
		{
			r = 0x8000000000000000;
			CPU.SetFPSCRException(FPSCR_VXCVI);
			CPU.FPSCR.FI = 0;
			CPU.FPSCR.FR = 0;
		}
		else
		{
			s64 i = 0;
			switch(CPU.FPSCR.RN)
			{
			case FPSCR_RN_NEAR:
				{
					double t = b + 0.5;
					i = (s64)t;
					if (t - i < 0 || (t - i == 0 && b > 0)) i--;
					break;
				}
			case FPSCR_RN_ZERO:
				i = (s64)b;
				break;
			case FPSCR_RN_PINF:
				i = (s64)b;
				if (b - i > 0) i++;
				break;
			case FPSCR_RN_MINF:
				i = (s64)b;
				if (b - i < 0) i--;
				break;
			}
			r = (u64)i;
			double di = i;
			if (di == b)
			{
				CPU.SetFPSCR_FI(0);
				CPU.FPSCR.FR = 0;
			}
			else
			{
				CPU.SetFPSCR_FI(1);
				CPU.FPSCR.FR = fabs(di) > fabs(b);
			}
		}

		(u64&)CPU.FPR[frd] = 0xfff8000000000000ull | r;
		if(r == 0 && ( (u64&)b & DOUBLE_SIGN )) (u64&)CPU.FPR[frd] |= 0x100000000ull;

		if(rc) UNK("fctid.");
	}
	void FCTIDZ(u32 frd, u32 frb, bool rc)
	{
		const double b = CPU.FPR[frb];
		u64 r;
		if(b > (double)0x7fffffffffffffff)
		{
			r = 0x7fffffffffffffff;
			CPU.SetFPSCRException(FPSCR_VXCVI);
			CPU.FPSCR.FI = 0;
			CPU.FPSCR.FR = 0;
		}
		else if (b < -(double)0x8000000000000000)
		{
			r = 0x8000000000000000;
			CPU.SetFPSCRException(FPSCR_VXCVI);
			CPU.FPSCR.FI = 0;
			CPU.FPSCR.FR = 0;
		}
		else
		{
			s64 i = (s64)b;
			double di = i;
			if (di == b)
			{
				CPU.SetFPSCR_FI(0);
				CPU.FPSCR.FR = 0;
			}
			else
			{
				CPU.SetFPSCR_FI(1);
				CPU.FPSCR.FR = fabs(di) > fabs(b);
			}
			r = (u64)i;
		}

		(u64&)CPU.FPR[frd] = 0xfff8000000000000ull | r;
		if(r == 0 && ( (u64&)b & DOUBLE_SIGN )) (u64&)CPU.FPR[frd] |= 0x100000000ull;

		if(rc) UNK("fctidz.");
	}
	void FCFID(u32 frd, u32 frb, bool rc)
	{
		s64 bi = (s64&)CPU.FPR[frb];
		double bf = (double)bi;
		s64 bfi = (s64)bf;

		if(bi == bfi)
		{
			CPU.SetFPSCR_FI(0);
			CPU.FPSCR.FR = 0;
		}
		else
		{
			CPU.SetFPSCR_FI(1);
			CPU.FPSCR.FR = abs(bfi) > abs(bi);
		}

		CPU.FPR[frd] = bf;

		CPU.FPSCR.FPRF = CPU.FPR[frd].GetType();
		if(rc) UNK("fcfid.");//CPU.UpdateCR1(CPU.FPR[frd]);
	}

	void UNK(const u32 code, const u32 opcode, const u32 gcode)
	{
		UNK(fmt::Format("Unknown/Illegal opcode! (0x%08x : 0x%x : 0x%x)", code, opcode, gcode));
	}

	void UNK(const std::string& err, bool pause = true)
	{
		ConLog.Error(err + fmt::Format(" #pc: 0x%llx", CPU.PC));

		if(!pause) return;

		Emu.Pause();

		for(uint i=0; i<32; ++i) ConLog.Write("r%d = 0x%llx", i, CPU.GPR[i]);
		for(uint i=0; i<32; ++i) ConLog.Write("f%d = %llf", i, CPU.FPR[i]);
		for(uint i=0; i<32; ++i) ConLog.Write("v%d = 0x%s [%s]", i, CPU.VPR[i].ToString(true).c_str(), CPU.VPR[i].ToString().c_str());
		ConLog.Write("CR = 0x%08x", CPU.CR);
		ConLog.Write("LR = 0x%llx", CPU.LR);
		ConLog.Write("CTR = 0x%llx", CPU.CTR);
		ConLog.Write("XER = 0x%llx [CA=%lld | OV=%lld | SO=%lld]", CPU.XER, fmt::by_value(CPU.XER.CA), fmt::by_value(CPU.XER.OV), fmt::by_value(CPU.XER.SO));
		ConLog.Write("FPSCR = 0x%x "
			"[RN=%d | NI=%d | XE=%d | ZE=%d | UE=%d | OE=%d | VE=%d | "
			"VXCVI=%d | VXSQRT=%d | VXSOFT=%d | FPRF=%d | "
			"FI=%d | FR=%d | VXVC=%d | VXIMZ=%d | "
			"VXZDZ=%d | VXIDI=%d | VXISI=%d | VXSNAN=%d | "
			"XX=%d | ZX=%d | UX=%d | OX=%d | VX=%d | FEX=%d | FX=%d]",
			CPU.FPSCR,
			fmt::by_value(CPU.FPSCR.RN),
			fmt::by_value(CPU.FPSCR.NI), fmt::by_value(CPU.FPSCR.XE), fmt::by_value(CPU.FPSCR.ZE), fmt::by_value(CPU.FPSCR.UE), fmt::by_value(CPU.FPSCR.OE), fmt::by_value(CPU.FPSCR.VE),
			fmt::by_value(CPU.FPSCR.VXCVI), fmt::by_value(CPU.FPSCR.VXSQRT), fmt::by_value(CPU.FPSCR.VXSOFT), fmt::by_value(CPU.FPSCR.FPRF),
			fmt::by_value(CPU.FPSCR.FI), fmt::by_value(CPU.FPSCR.FR), fmt::by_value(CPU.FPSCR.VXVC), fmt::by_value(CPU.FPSCR.VXIMZ),
			fmt::by_value(CPU.FPSCR.VXZDZ), fmt::by_value(CPU.FPSCR.VXIDI), fmt::by_value(CPU.FPSCR.VXISI), fmt::by_value(CPU.FPSCR.VXSNAN),
			fmt::by_value(CPU.FPSCR.XX), fmt::by_value(CPU.FPSCR.ZX), fmt::by_value(CPU.FPSCR.UX), fmt::by_value(CPU.FPSCR.OX), fmt::by_value(CPU.FPSCR.VX), fmt::by_value(CPU.FPSCR.FEX), fmt::by_value(CPU.FPSCR.FX));
	}
};
