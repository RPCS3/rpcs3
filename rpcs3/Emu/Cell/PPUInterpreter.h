#pragma once

#include "Emu/Cell/PPUOpcodes.h"
#include "Emu/SysCalls/SysCalls.h"
#include "rpcs3/Ini.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Static.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/lv2/sys_time.h"

#include <stdint.h>
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#define _rotl64(x,r) (((u64)(x) << (r)) | ((u64)(x) >> (64 - (r))))
#endif

#if 0//def _DEBUG
#define HLE_CALL_DEBUG
#endif

extern u64 rotate_mask[64][64]; // defined in PPUThread.cpp, static didn't work correctly in GCC 4.9 for some reason
inline void InitRotateMask()
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

inline u8 rotl8(const u8 x, const u8 n) { return (x << n) | (x >> (8 - n)); }
inline u8 rotr8(const u8 x, const u8 n) { return (x >> n) | (x << (8 - n)); }

inline u16 rotl16(const u16 x, const u8 n) { return (x << n) | (x >> (16 - n)); }
inline u16 rotr16(const u16 x, const u8 n) { return (x >> n) | (x << (16 - n)); }
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

namespace ppu_recompiler_llvm {
	class Compiler;
}

class PPUInterpreter : public PPUOpcodes
{
#ifdef PPU_LLVM_RECOMPILER
	friend class ppu_recompiler_llvm::Compiler;
#endif
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
		const u64 sc = CPU.GPR[11];
		const u64 old_sc = CPU.m_last_syscall;

		CPU.m_last_syscall = sc;
		SysCalls::DoSyscall(CPU, (u32)sc);

		if(Ini.HLELogging.GetValue())
		{
			LOG_WARNING(PPU, "SysCall[0x%llx ('%s')] done with code [0x%llx]! #pc: 0x%x",
				sc, SysCalls::GetHLEFuncName((u32)sc).c_str(), CPU.GPR[3], CPU.PC);
		}
#ifdef HLE_CALL_DEBUG
		LOG_NOTICE(PPU, "SysCall[%lld] done with code [0x%llx]! #pc: 0x%x", sc, CPU.GPR[3], CPU.PC);
#endif

		CPU.m_last_syscall = old_sc;
	}

	void NULL_OP()
	{
		throw "Null operation";
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

	u64 ReadSPR(u32 spr)
	{
		const u32 n = (spr >> 5) | ((spr & 0x1f) << 5);

		switch (n)
		{
		case 0x001: return CPU.XER.XER;
		case 0x008: return CPU.LR;
		case 0x009: return CPU.CTR;
		case 0x100: return CPU.VRSAVE;
		case 0x103: return CPU.SPRG[3];

		case 0x10C: CPU.TB = get_time(); return CPU.TB;
		case 0x10D: CPU.TB = get_time(); return CPU.TB >> 32;

		case 0x110:
		case 0x111:
		case 0x112:
		case 0x113:
		case 0x114:
		case 0x115:
		case 0x116:
		case 0x117: return CPU.SPRG[n - 0x110];
		}

		throw fmt::Format("ReadSPR(0x%x) error: unknown SPR (0x%x)", spr, n);
	}

	void WriteSPR(u32 spr, u64 value)
	{
		const u32 n = (spr >> 5) | ((spr & 0x1f) << 5);

		switch (n)
		{
		case 0x001: CPU.XER.XER = value; return;
		case 0x008: CPU.LR = value; return;
		case 0x009: CPU.CTR = value; return;
		case 0x100: CPU.VRSAVE = (u32)value; return;
		case 0x103: throw fmt::Format("WriteSPR(0x103, 0x%llx): Write to read-only SPR", value);

		case 0x10C: throw fmt::Format("WriteSPR(0x10C, 0x%llx): Write to time-based SPR", value);
		case 0x10D: throw fmt::Format("WriteSPR(0x10D, 0x%llx): Write to time-based SPR", value);

		case 0x110:
		case 0x111:
		case 0x112:
		case 0x113:
		case 0x114:
		case 0x115:
		case 0x116:
		case 0x117: CPU.SPRG[n - 0x110] = value; return;
		}

		throw fmt::Format("WriteSPR(0x%x, 0x%llx) error: unknown SPR (0x%x)", spr, value, n);
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
			throw fmt::Format("Trap! (tdi 0x%x, r%d, 0x%x)", to, ra, simm16);
		}
	}

	void TWI(u32 to, u32 ra, s32 simm16)
	{
		s32 a = (s32)CPU.GPR[ra];

		if( (a < simm16  && (to & 0x10)) ||
			(a > simm16  && (to & 0x8))  ||
			(a == simm16 && (to & 0x4))  ||
			((u32)a < (u32)simm16 && (to & 0x2)) ||
			((u32)a > (u32)simm16 && (to & 0x1)) )
		{
			throw fmt::Format("Trap! (twi 0x%x, r%d, 0x%x)", to, ra, simm16);
		}
	}

	void MFVSCR(u32 vd) //nf
	{
		CPU.VPR[vd].clear();
		CPU.VPR[vd]._u32[0] = CPU.VSCR.VSCR;
	}
	void MTVSCR(u32 vb)
	{
		CPU.VSCR.VSCR = CPU.VPR[vb]._u32[0];
		CPU.VSCR.X = CPU.VSCR.Y = 0;
	}
	void VADDCUW(u32 vd, u32 va, u32 vb) //nf
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
	void VADDSBS(u32 vd, u32 va, u32 vb) //nf
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
				CPU.VPR[vd]._s8[b] = (s8)result;
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
	void VADDSWS(u32 vd, u32 va, u32 vb) //nf
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
				CPU.VPR[vd]._s32[w] = (s32)result;
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
				CPU.VPR[vd]._u8[b] = (u8)result;
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
				CPU.VPR[vd]._u32[w] = (u32)result;
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
	void VAVGSB(u32 vd, u32 va, u32 vb) //nf
	{
		for (uint b = 0; b < 16; b++)
		{
			CPU.VPR[vd]._s8[b] = (CPU.VPR[va]._s8[b] + CPU.VPR[vb]._s8[b] + 1) >> 1;
		}
	}
	void VAVGSH(u32 vd, u32 va, u32 vb) //nf
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._s16[h] = (CPU.VPR[va]._s16[h] + CPU.VPR[vb]._s16[h] + 1) >> 1;
		}
	}
	void VAVGSW(u32 vd, u32 va, u32 vb) //nf
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
	void VAVGUH(u32 vd, u32 va, u32 vb) //nf
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._u16[h] = (CPU.VPR[va]._u16[h] + CPU.VPR[vb]._u16[h] + 1) >> 1;
		}
	}
	void VAVGUW(u32 vd, u32 va, u32 vb) //nf
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
		CPU.SetCR(6, 0);
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
	void VCMPEQUH(u32 vd, u32 va, u32 vb) //nf
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._u16[h] = CPU.VPR[va]._u16[h] == CPU.VPR[vb]._u16[h] ? 0xffff : 0;
		}
	}
	void VCMPEQUH_(u32 vd, u32 va, u32 vb) //nf
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
	void VCMPGTSB(u32 vd, u32 va, u32 vb) //nf
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
		u32 nScale = 1 << uimm5;

		for (uint w = 0; w < 4; w++)
		{
			double result = (double)CPU.VPR[vb]._f[w] * nScale;

			if (result > 0x7fffffff)
			{
				CPU.VPR[vd]._s32[w] = (int)0x7fffffff;
				CPU.VSCR.SAT = 1;
			}
			else if (result < -pow(2, 31))
			{
				CPU.VPR[vd]._s32[w] = (int)0x80000000;
				CPU.VSCR.SAT = 1;
			}
			else // C rounding = Round towards 0
				CPU.VPR[vd]._s32[w] = (int)result;
		}
	}
	void VCTUXS(u32 vd, u32 uimm5, u32 vb)
	{
		u32 nScale = 1 << uimm5;

		for (uint w = 0; w < 4; w++)
		{
			// C rounding = Round towards 0
			double result = (double)CPU.VPR[vb]._f[w] * nScale;

			if (result > 0xffffffffu)
			{
				CPU.VPR[vd]._u32[w] = 0xffffffffu;
				CPU.VSCR.SAT = 1;
			}
			else if (result < 0)
			{
				CPU.VPR[vd]._u32[w] = 0;
				CPU.VSCR.SAT = 1;
			}
			else
				CPU.VPR[vd]._u32[w] = (u32)result;
		}
	}
	void VEXPTEFP(u32 vd, u32 vb)
	{
		// vd = 2^x
		// ISA : Note that the value placed into the element of vD may vary between implementations
		// and between different executions on the same implementation.
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._f[w] = powf(2.0f, CPU.VPR[vb]._f[w]);
		}
	}
	void VLOGEFP(u32 vd, u32 vb)
	{
		// ISA : Note that the value placed into the element of vD may vary between implementations
		// and between different executions on the same implementation.
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._f[w] = log2f(CPU.VPR[vb]._f[w]);
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
			CPU.VPR[vd]._f[w] = std::max(CPU.VPR[va]._f[w], CPU.VPR[vb]._f[w]);
		}
	}
	void VMAXSB(u32 vd, u32 va, u32 vb) //nf
	{
		for (uint b = 0; b < 16; b++)
			CPU.VPR[vd]._s8[b] = std::max(CPU.VPR[va]._s8[b], CPU.VPR[vb]._s8[b]);
	}
	void VMAXSH(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._s16[h] = std::max(CPU.VPR[va]._s16[h], CPU.VPR[vb]._s16[h]);
		}
	}
	void VMAXSW(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._s32[w] = std::max(CPU.VPR[va]._s32[w], CPU.VPR[vb]._s32[w]);
		}
	}
	void VMAXUB(u32 vd, u32 va, u32 vb)
	{
		for (uint b = 0; b < 16; b++)
			CPU.VPR[vd]._u8[b] = std::max(CPU.VPR[va]._u8[b], CPU.VPR[vb]._u8[b]);
	}
	void VMAXUH(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._u16[h] = std::max(CPU.VPR[va]._u16[h], CPU.VPR[vb]._u16[h]);
		}
	}
	void VMAXUW(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u32[w] = std::max(CPU.VPR[va]._u32[w], CPU.VPR[vb]._u32[w]);
		}
	}
	void VMHADDSHS(u32 vd, u32 va, u32 vb, u32 vc)
	{
		for (uint h = 0; h < 8; h++)
		{
			s32 result = (s32)CPU.VPR[va]._s16[h] * (s32)CPU.VPR[vb]._s16[h];
			result     = (result >> 15) + (s32)CPU.VPR[vc]._s16[h];

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
			s32 result = ((s32)CPU.VPR[va]._s16[h] * (s32)CPU.VPR[vb]._s16[h]) + 0x4000;
			result     = (result >> 15) + (s32)CPU.VPR[vc]._s16[h];

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
			CPU.VPR[vd]._f[w] = std::min(CPU.VPR[va]._f[w], CPU.VPR[vb]._f[w]);
		}
	}
	void VMINSB(u32 vd, u32 va, u32 vb) //nf
	{
		for (uint b = 0; b < 16; b++)
		{
			CPU.VPR[vd]._s8[b] = std::min(CPU.VPR[va]._s8[b], CPU.VPR[vb]._s8[b]);
		}
	}
	void VMINSH(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._s16[h] = std::min(CPU.VPR[va]._s16[h], CPU.VPR[vb]._s16[h]);
		}
	}
	void VMINSW(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._s32[w] = std::min(CPU.VPR[va]._s32[w], CPU.VPR[vb]._s32[w]);
		}
	}
	void VMINUB(u32 vd, u32 va, u32 vb)
	{
		for (uint b = 0; b < 16; b++)
		{
			CPU.VPR[vd]._u8[b] = std::min(CPU.VPR[va]._u8[b], CPU.VPR[vb]._u8[b]);
		}
	}
	void VMINUH(u32 vd, u32 va, u32 vb)
	{
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._u16[h] = std::min(CPU.VPR[va]._u16[h], CPU.VPR[vb]._u16[h]);
		}
	}
	void VMINUW(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u32[w] = std::min(CPU.VPR[va]._u32[w], CPU.VPR[vb]._u32[w]);
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
		u128 VA = CPU.VPR[va];
		u128 VB = CPU.VPR[vb];
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._u8[15 - h*2] = VA._u8[15 - h];
			CPU.VPR[vd]._u8[15 - h*2 - 1] = VB._u8[15 - h];
		}
	}
	void VMRGHH(u32 vd, u32 va, u32 vb)
	{
		u128 VA = CPU.VPR[va];
		u128 VB = CPU.VPR[vb];
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u16[7 - w*2] = VA._u16[7 - w];
			CPU.VPR[vd]._u16[7 - w*2 - 1] = VB._u16[7 - w];
		}
	}
	void VMRGHW(u32 vd, u32 va, u32 vb)
	{
		u128 VA = CPU.VPR[va];
		u128 VB = CPU.VPR[vb];
		for (uint d = 0; d < 2; d++)
		{
			CPU.VPR[vd]._u32[3 - d*2] = VA._u32[3 - d];
			CPU.VPR[vd]._u32[3 - d*2 - 1] = VB._u32[3 - d];
		}
	}
	void VMRGLB(u32 vd, u32 va, u32 vb)
	{
		u128 VA = CPU.VPR[va];
		u128 VB = CPU.VPR[vb];
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._u8[15 - h*2] = VA._u8[7 - h];
			CPU.VPR[vd]._u8[15 - h*2 - 1] = VB._u8[7 - h];
		}
	}
	void VMRGLH(u32 vd, u32 va, u32 vb)
	{
		u128 VA = CPU.VPR[va];
		u128 VB = CPU.VPR[vb];
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u16[7 - w*2] = VA._u16[3 - w];
			CPU.VPR[vd]._u16[7 - w*2 - 1] = VB._u16[3 - w];
		}
	}
	void VMRGLW(u32 vd, u32 va, u32 vb)
	{
		u128 VA = CPU.VPR[va];
		u128 VB = CPU.VPR[vb];
		for (uint d = 0; d < 2; d++)
		{
			CPU.VPR[vd]._u32[3 - d*2] = VA._u32[1 - d];
			CPU.VPR[vd]._u32[3 - d*2 - 1] = VB._u32[1 - d];
		}
	}
	void VMSUMMBM(u32 vd, u32 va, u32 vb, u32 vc) //nf
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
	void VMSUMSHM(u32 vd, u32 va, u32 vb, u32 vc) //nf
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
	void VMSUMSHS(u32 vd, u32 va, u32 vb, u32 vc) //nf
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

			if (result > 0x7fffffff)
			{
				saturated = 0x7fffffff;
				CPU.VSCR.SAT = 1;
			}
			else if (result < (s64)(s32)0x80000000)
			{
				saturated = 0x80000000;
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
				result += (u32)CPU.VPR[va]._u8[w*4 + b] * (u32)CPU.VPR[vb]._u8[w*4 + b];
			}

			result += CPU.VPR[vc]._u32[w];
			CPU.VPR[vd]._u32[w] = result;
		}
	}
	void VMSUMUHM(u32 vd, u32 va, u32 vb, u32 vc) //nf
	{
		for (uint w = 0; w < 4; w++)
		{
			u32 result = 0;

			for (uint h = 0; h < 2; h++)
			{
				result += (u32)CPU.VPR[va]._u16[w*2 + h] * (u32)CPU.VPR[vb]._u16[w*2 + h];
			}

			result += CPU.VPR[vc]._u32[w];
			CPU.VPR[vd]._u32[w] = result;
		}
	}
	void VMSUMUHS(u32 vd, u32 va, u32 vb, u32 vc) //nf
	{
		for (uint w = 0; w < 4; w++)
		{
			u64 result = 0;
			u32 saturated = 0;

			for (uint h = 0; h < 2; h++)
			{
				result += (u64)CPU.VPR[va]._u16[w*2 + h] * (u64)CPU.VPR[vb]._u16[w*2 + h];
			}

			result += CPU.VPR[vc]._u32[w];

			if (result > 0xffffffffu)
			{
				saturated = 0xffffffff;
				CPU.VSCR.SAT = 1;
			}
			else
				saturated = (u32)result;

			CPU.VPR[vd]._u32[w] = saturated;
		}
	}
	void VMULESB(u32 vd, u32 va, u32 vb) //nf
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
	void VMULOSB(u32 vd, u32 va, u32 vb) //nf
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
		u128 VA = CPU.VPR[va];
		u128 VB = CPU.VPR[vb];
		for (uint h = 0; h < 4; h++)
		{
			u16 bb7  = VB._u8[15 - (h*4 + 0)] & 0x1;
			u16 bb8  = VB._u8[15 - (h*4 + 1)] >> 3;
			u16 bb16 = VB._u8[15 - (h*4 + 2)] >> 3;
			u16 bb24 = VB._u8[15 - (h*4 + 3)] >> 3;
			u16 ab7  = VA._u8[15 - (h*4 + 0)] & 0x1;
			u16 ab8  = VA._u8[15 - (h*4 + 1)] >> 3;
			u16 ab16 = VA._u8[15 - (h*4 + 2)] >> 3;
			u16 ab24 = VA._u8[15 - (h*4 + 3)] >> 3;

			CPU.VPR[vd]._u16[3 - h]			= (bb7 << 15) | (bb8 << 10) | (bb16 << 5) | bb24;
			CPU.VPR[vd]._u16[4 + (3 - h)]	= (ab7 << 15) | (ab8 << 10) | (ab16 << 5) | ab24;
		}
	}
	void VPKSHSS(u32 vd, u32 va, u32 vb) //nf
	{
		u128 VA = CPU.VPR[va];
		u128 VB = CPU.VPR[vb];
		for (uint b = 0; b < 8; b++)
		{
			s16 result = VA._s16[b];

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
				
			CPU.VPR[vd]._s8[b+8] = (s8)result;

			result = VB._s16[b];

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

			CPU.VPR[vd]._s8[b] = (s8)result;
		}
	}
	void VPKSHUS(u32 vd, u32 va, u32 vb)
	{
		u128 VA = CPU.VPR[va];
		u128 VB = CPU.VPR[vb];
		for (uint b = 0; b < 8; b++)
		{
			s16 result = VA._s16[b];

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
				
			CPU.VPR[vd]._u8[b+8] = (u8)result;

			result = VB._s16[b];

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
				
			CPU.VPR[vd]._u8[b] = (u8)result;
		}
	}
	void VPKSWSS(u32 vd, u32 va, u32 vb)
	{
		u128 VA = CPU.VPR[va];
		u128 VB = CPU.VPR[vb];
		for (uint h = 0; h < 4; h++)
		{
			s32 result = VA._s32[h];

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

			result = VB._s32[h];

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
	void VPKSWUS(u32 vd, u32 va, u32 vb) //nf
	{
		u128 VA = CPU.VPR[va];
		u128 VB = CPU.VPR[vb];
		for (uint h = 0; h < 4; h++)
		{
			s32 result = VA._s32[h];

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

			result = VB._s32[h];

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
	void VPKUHUM(u32 vd, u32 va, u32 vb) //nf
	{
		u128 VA = CPU.VPR[va];
		u128 VB = CPU.VPR[vb];
		for (uint b = 0; b < 8; b++)
		{
			CPU.VPR[vd]._u8[b+8] = VA._u8[b*2];
			CPU.VPR[vd]._u8[b  ] = VB._u8[b*2];
		}
	}
	void VPKUHUS(u32 vd, u32 va, u32 vb)
	{
		u128 VA = CPU.VPR[va];
		u128 VB = CPU.VPR[vb];
		for (uint b = 0; b < 8; b++)
		{
			u16 result = VA._u16[b];

			if (result > UINT8_MAX)
			{
				result = UINT8_MAX;
				CPU.VSCR.SAT = 1;
			}
				
			CPU.VPR[vd]._u8[b+8] = (u8)result;

			result = VB._u16[b];

			if (result > UINT8_MAX)
			{
				result = UINT8_MAX;
				CPU.VSCR.SAT = 1;
			}
				
			CPU.VPR[vd]._u8[b] = (u8)result;
		}
	}
	void VPKUWUM(u32 vd, u32 va, u32 vb)
	{
		u128 VA = CPU.VPR[va];
		u128 VB = CPU.VPR[vb];
		for (uint h = 0; h < 4; h++)
		{
			CPU.VPR[vd]._u16[h+4] = VA._u16[h*2];
			CPU.VPR[vd]._u16[h  ] = VB._u16[h*2];
		}
	}
	void VPKUWUS(u32 vd, u32 va, u32 vb) //nf
	{
		u128 VA = CPU.VPR[va];
		u128 VB = CPU.VPR[vb];
		for (uint h = 0; h < 4; h++)
		{
			u32 result = VA._u32[h];

			if (result > UINT16_MAX)
			{
				result = UINT16_MAX;
				CPU.VSCR.SAT = 1;
			}
				
			CPU.VPR[vd]._u16[h+4] = result;

			result = VB._u32[h];

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
			CPU.VPR[vd]._f[w] = floorf(CPU.VPR[vb]._f[w]);
		}
	}
	void VRFIN(u32 vd, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._f[w] = nearbyintf(CPU.VPR[vb]._f[w]);
		}
	}
	void VRFIP(u32 vd, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._f[w] = ceilf(CPU.VPR[vb]._f[w]);
		}
	}
	void VRFIZ(u32 vd, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._f[w] = truncf(CPU.VPR[vb]._f[w]);
		}
	}
	void VRLB(u32 vd, u32 va, u32 vb) //nf
	{
		for (uint b = 0; b < 16; b++)
		{
			int nRot = CPU.VPR[vb]._u8[b] & 0x7;

			CPU.VPR[vd]._u8[b] = (CPU.VPR[va]._u8[b] << nRot) | (CPU.VPR[va]._u8[b] >> (8 - nRot));
		}
	}
	void VRLH(u32 vd, u32 va, u32 vb) //nf
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
			CPU.VPR[vd]._u32[w] = (u32)rotl32(CPU.VPR[va]._u32[w], CPU.VPR[vb]._u8[w*4] & 0x1f);
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
	void VSL(u32 vd, u32 va, u32 vb) //nf
	{
		u128 VA = CPU.VPR[va];
		u8 sh = CPU.VPR[vb]._u8[0] & 0x7;

		CPU.VPR[vd]._u8[0] = VA._u8[0] << sh;
		for (uint b = 1; b < 16; b++)
		{
			CPU.VPR[vd]._u8[b] = (VA._u8[b] << sh) | (VA._u8[b-1] >> (8 - sh));
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
			CPU.VPR[vd]._u16[h] = CPU.VPR[va]._u16[h] << (CPU.VPR[vb]._u16[h] & 0xf);
		}
	}
	void VSLO(u32 vd, u32 va, u32 vb)
	{
		u128 VA = CPU.VPR[va];
		u8 nShift = (CPU.VPR[vb]._u8[0] >> 3) & 0xf;

		CPU.VPR[vd].clear();

		for (u8 b = 0; b < 16 - nShift; b++)
		{
			CPU.VPR[vd]._u8[15 - b] = VA._u8[15 - (b + nShift)];
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
	void VSR(u32 vd, u32 va, u32 vb) //nf
	{
		u128 VA = CPU.VPR[va];
		u8 sh = CPU.VPR[vb]._u8[0] & 0x7;

		CPU.VPR[vd]._u8[15] = VA._u8[15] >> sh;
		for (uint b = 14; ~b; b--)
		{
			CPU.VPR[vd]._u8[b] = (VA._u8[b] >> sh) | (VA._u8[b+1] << (8 - sh));
		}
	}
	void VSRAB(u32 vd, u32 va, u32 vb) //nf
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
			CPU.VPR[vd]._s16[h] = CPU.VPR[va]._s16[h] >> (CPU.VPR[vb]._u16[h] & 0xf);
		}
	}
	void VSRAW(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._s32[w] = CPU.VPR[va]._s32[w] >> (CPU.VPR[vb]._u32[w] & 0x1f);
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
			CPU.VPR[vd]._u16[h] = CPU.VPR[va]._u16[h] >> (CPU.VPR[vb]._u16[h] & 0xf);
		}
	}
	void VSRO(u32 vd, u32 va, u32 vb)
	{
		u128 VA = CPU.VPR[va];
		u8 nShift = (CPU.VPR[vb]._u8[0] >> 3) & 0xf;

		CPU.VPR[vd].clear();

		for (u8 b = 0; b < 16 - nShift; b++)
		{
			CPU.VPR[vd]._u8[b] = VA._u8[b + nShift];
		}
	}
	void VSRW(u32 vd, u32 va, u32 vb)
	{
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._u32[w] = CPU.VPR[va]._u32[w] >> (CPU.VPR[vb]._u32[w] & 0x1f);
		}
	}
	void VSUBCUW(u32 vd, u32 va, u32 vb) //nf
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
	void VSUBSBS(u32 vd, u32 va, u32 vb) //nf
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
	void VSUBUHS(u32 vd, u32 va, u32 vb) //nf
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
		CPU.VPR[vd].clear();
			
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
	void VSUM4SBS(u32 vd, u32 va, u32 vb) //nf
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
		u128 VB = CPU.VPR[vb];
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._s8[w*4 + 3] = VB._s8[8 + w*2 + 1] >> 7;  // signed shift sign extends
			CPU.VPR[vd]._u8[w*4 + 2] = (VB._u8[8 + w*2 + 1] >> 2) & 0x1f;
			CPU.VPR[vd]._u8[w*4 + 1] = ((VB._u8[8 + w*2 + 1] & 0x3) << 3) | ((VB._u8[8 + w*2 + 0] >> 5) & 0x7);
			CPU.VPR[vd]._u8[w*4 + 0] = VB._u8[8 + w*2 + 0] & 0x1f;
		}
	}
	void VUPKHSB(u32 vd, u32 vb)
	{
		u128 VB = CPU.VPR[vb];
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._s16[h] = VB._s8[8 + h];
		}
	}
	void VUPKHSH(u32 vd, u32 vb)
	{
		u128 VB = CPU.VPR[vb];
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._s32[w] = VB._s16[4 + w];
		}
	}
	void VUPKLPX(u32 vd, u32 vb)
	{
		u128 VB = CPU.VPR[vb];
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._s8[w*4 + 3] = VB._s8[w*2 + 1] >> 7;  // signed shift sign extends
			CPU.VPR[vd]._u8[w*4 + 2] = (VB._u8[w*2 + 1] >> 2) & 0x1f;
			CPU.VPR[vd]._u8[w*4 + 1] = ((VB._u8[w*2 + 1] & 0x3) << 3) | ((VB._u8[w*2 + 0] >> 5) & 0x7);
			CPU.VPR[vd]._u8[w*4 + 0] = VB._u8[w*2 + 0] & 0x1f;
		}
	}
	void VUPKLSB(u32 vd, u32 vb) //nf
	{
		u128 VB = CPU.VPR[vb];
		for (uint h = 0; h < 8; h++)
		{
			CPU.VPR[vd]._s16[h] = VB._s8[h];
		}
	}
	void VUPKLSH(u32 vd, u32 vb)
	{
		u128 VB = CPU.VPR[vb];
		for (uint w = 0; w < 4; w++)
		{
			CPU.VPR[vd]._s32[w] = VB._s16[w];
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
		const u64 IMM = (s64)simm16;
		CPU.GPR[rd] = ~RA + IMM + 1;

		CPU.XER.CA = CPU.IsCarry(~RA, IMM, 1);
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
			const u32 nextLR = CPU.PC + 4;
			CPU.SetBranch(branchTarget((aa ? 0 : CPU.PC), bd), lk);
			if(lk) CPU.LR = nextLR;
		}
	}
	void SC(u32 lev)
	{
		switch (lev)
		{
		case 0x0: SysCall(); break;
		case 0x1: throw "SC(): HyperCall LV1";
		case 0x2:
			Emu.GetSFuncManager().StaticExecute(CPU, (u32)CPU.GPR[11]);
			if (Ini.HLELogging.GetValue())
			{
				LOG_NOTICE(PPU, "'%s' done with code[0x%llx]! #pc: 0x%x",
					Emu.GetSFuncManager()[CPU.GPR[11]]->name, CPU.GPR[3], CPU.PC);
			}
			break;
		case 0x3: CPU.FastStop(); break;
		default: throw fmt::Format("SC(): unknown level (0x%x)", lev);
		}
	}
	void B(s32 ll, u32 aa, u32 lk)
	{
		const u32 nextLR = CPU.PC + 4;
		CPU.SetBranch(branchTarget(aa ? 0 : CPU.PC, ll), lk);
		if(lk) CPU.LR = nextLR;
	}
	void MCRF(u32 crfd, u32 crfs)
	{
		CPU.SetCR(crfd, CPU.GetCR(crfs));
	}
	void BCLR(u32 bo, u32 bi, u32 bh, u32 lk)
	{
		if (CheckCondition(bo, bi))
		{
			const u32 nextLR = CPU.PC + 4;
			CPU.SetBranch(branchTarget(0, (u32)CPU.LR), true);
			if(lk) CPU.LR = nextLR;
		}
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
		if(bo & 0x10 || CPU.IsCR(bi) == ((bo & 0x8) != 0))
		{
			const u32 nextLR = CPU.PC + 4;
			CPU.SetBranch(branchTarget(0, (u32)CPU.CTR), true);
			if(lk) CPU.LR = nextLR;
		}
	}	
	void RLWIMI(u32 ra, u32 rs, u32 sh, u32 mb, u32 me, bool rc)
	{
		const u64 mask = rotate_mask[32 + mb][32 + me];
		CPU.GPR[ra] = (CPU.GPR[ra] & ~mask) | (rotl32(CPU.GPR[rs], sh) & mask);
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void RLWINM(u32 ra, u32 rs, u32 sh, u32 mb, u32 me, bool rc)
	{
		CPU.GPR[ra] = rotl32(CPU.GPR[rs], sh) & rotate_mask[32 + mb][32 + me];
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void RLWNM(u32 ra, u32 rs, u32 rb, u32 mb, u32 me, bool rc)
	{
		CPU.GPR[ra] = rotl32(CPU.GPR[rs], CPU.GPR[rb] & 0x1f) & rotate_mask[32 + mb][32 + me];
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void ORI(u32 ra, u32 rs, u32 uimm16)
	{
		CPU.GPR[ra] = CPU.GPR[rs] | uimm16;
	}
	void ORIS(u32 ra, u32 rs, u32 uimm16)
	{
		CPU.GPR[ra] = CPU.GPR[rs] | ((u64)uimm16 << 16);
	}
	void XORI(u32 ra, u32 rs, u32 uimm16)
	{
		CPU.GPR[ra] = CPU.GPR[rs] ^ uimm16;
	}
	void XORIS(u32 ra, u32 rs, u32 uimm16)
	{
		CPU.GPR[ra] = CPU.GPR[rs] ^ ((u64)uimm16 << 16);
	}
	void ANDI_(u32 ra, u32 rs, u32 uimm16)
	{
		CPU.GPR[ra] = CPU.GPR[rs] & uimm16;
		CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void ANDIS_(u32 ra, u32 rs, u32 uimm16)
	{
		CPU.GPR[ra] = CPU.GPR[rs] & ((u64)uimm16 << 16);
		CPU.UpdateCR0<s64>(CPU.GPR[ra]);
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
			RLDICR(ra, rs, (u32)(CPU.GPR[rb] & 0x3F), m_eb, rc);
		}
		else // rldcl
		{
			RLDICL(ra, rs, (u32)(CPU.GPR[rb] & 0x3F), m_eb, rc);
		}
	}
	void CMP(u32 crfd, u32 l, u32 ra, u32 rb)
	{
		CPU.UpdateCRnS(l, crfd, CPU.GPR[ra], CPU.GPR[rb]);
	}
	void TW(u32 to, u32 ra, u32 rb)
	{
		s32 a = (s32)CPU.GPR[ra];
		s32 b = (s32)CPU.GPR[rb];

		if( (a < b  && (to & 0x10)) ||
			(a > b  && (to & 0x8))  ||
			(a == b && (to & 0x4))  ||
			((u32)a < (u32)b && (to & 0x2)) ||
			((u32)a > (u32)b && (to & 0x1)) )
		{
			throw fmt::Format("Trap! (tw 0x%x, r%d, r%d)", to, ra, rb);
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
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		CPU.VPR[vd]._u8[15 - (addr & 0xf)] = vm::read8(vm::cast(addr));
		// check LVEWX comments
	}
	void SUBFC(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		const u64 RA = CPU.GPR[ra];
		const u64 RB = CPU.GPR[rb];
		CPU.GPR[rd] = ~RA + RB + 1;
		CPU.XER.CA = CPU.IsCarry(~RA, RB, 1);
		if(oe) CPU.SetOV((~RA>>63 == RB>>63) && (~RA>>63 != CPU.GPR[rd]>>63));
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void ADDC(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		const u64 RA = CPU.GPR[ra];
		const u64 RB = CPU.GPR[rb];
		CPU.GPR[rd] = RA + RB;
		CPU.XER.CA = CPU.IsCarry(RA, RB);
		if(oe) CPU.SetOV((RA>>63 == RB>>63) && (RA>>63 != CPU.GPR[rd]>>63));
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void MULHDU(u32 rd, u32 ra, u32 rb, bool rc)
	{
		CPU.GPR[rd] = __umulh(CPU.GPR[ra], CPU.GPR[rb]);
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void MULHWU(u32 rd, u32 ra, u32 rb, bool rc)
	{
		u32 a = (u32)CPU.GPR[ra];
		u32 b = (u32)CPU.GPR[rb];
		CPU.GPR[rd] = ((u64)a * (u64)b) >> 32;
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
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
		CPU.R_ADDR = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		CPU.R_VALUE = vm::get_ref<u32>(vm::cast(CPU.R_ADDR));
		CPU.GPR[rd] = re32((u32)CPU.R_VALUE);
	}
	void LDX(u32 rd, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		CPU.GPR[rd] = vm::read64(vm::cast(addr));
	}
	void LWZX(u32 rd, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		CPU.GPR[rd] = vm::read32(vm::cast(addr));
	}
	void SLW(u32 ra, u32 rs, u32 rb, bool rc)
	{
		u32 n = CPU.GPR[rb] & 0x1f;
		u32 r = (u32)rotl32((u32)CPU.GPR[rs], n);
		u32 m = ((u32)CPU.GPR[rb] & 0x20) ? 0 : (u32)rotate_mask[32][63 - n];

		CPU.GPR[ra] = r & m;

		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void CNTLZW(u32 ra, u32 rs, bool rc)
	{
		u32 i;
		for(i=0; i < 32; i++)
		{
			if(CPU.GPR[rs] & (1ULL << (31 - i))) break;
		}

		CPU.GPR[ra] = i;
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
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
		const u64 addr = (ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~1ULL;
		CPU.VPR[vd]._u16[7 - ((addr >> 1) & 0x7)] = vm::read16(vm::cast(addr));
		// check LVEWX comments
	}
	void SUBF(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		const u64 RA = CPU.GPR[ra];
		const u64 RB = CPU.GPR[rb];
		CPU.GPR[rd] = RB - RA;
		if(oe) CPU.SetOV((~RA>>63 == RB>>63) && (~RA>>63 != CPU.GPR[rd]>>63));
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void LDUX(u32 rd, u32 ra, u32 rb)
	{
		const u64 addr = CPU.GPR[ra] + CPU.GPR[rb];
		CPU.GPR[rd] = vm::read64(vm::cast(addr));
		CPU.GPR[ra] = addr;
	}
	void DCBST(u32 ra, u32 rb)
	{
	}
	void LWZUX(u32 rd, u32 ra, u32 rb)
	{
		const u64 addr = CPU.GPR[ra] + CPU.GPR[rb];
		CPU.GPR[rd] = vm::read32(vm::cast(addr));
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
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void ANDC(u32 ra, u32 rs, u32 rb, bool rc)
	{
		CPU.GPR[ra] = CPU.GPR[rs] & ~CPU.GPR[rb];
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void TD(u32 to, u32 ra, u32 rb)
	{
		throw "TD()";
	}
	void LVEWX(u32 vd, u32 ra, u32 rb)
	{
		const u64 addr = (ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~3ULL;
		CPU.VPR[vd]._u32[3 - ((addr >> 2) & 0x3)] = vm::read32(vm::cast(addr));
		// It's not very good idea to implement it using read128(),
		// because it can theoretically read RawSPU 32-bit MMIO register (read128() will fail)
		//CPU.VPR[vd] = vm::read128((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~0xfULL);
	}
	void MULHD(u32 rd, u32 ra, u32 rb, bool rc)
	{
		CPU.GPR[rd] = __mulh(CPU.GPR[ra], CPU.GPR[rb]);
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void MULHW(u32 rd, u32 ra, u32 rb, bool rc)
	{
		s32 a = (s32)CPU.GPR[ra];
		s32 b = (s32)CPU.GPR[rb];
		CPU.GPR[rd] = ((s64)a * (s64)b) >> 32;
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void LDARX(u32 rd, u32 ra, u32 rb)
	{
		CPU.R_ADDR = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		CPU.R_VALUE = vm::get_ref<u64>(vm::cast(CPU.R_ADDR));
		CPU.GPR[rd] = re64(CPU.R_VALUE);
	}
	void DCBF(u32 ra, u32 rb)
	{
	}
	void LBZX(u32 rd, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		CPU.GPR[rd] = vm::read8(vm::cast(addr));
	}
	void LVX(u32 vd, u32 ra, u32 rb)
	{
		CPU.VPR[vd] = vm::read128((u64)((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~0xfULL));
	}
	void NEG(u32 rd, u32 ra, u32 oe, bool rc)
	{
		const u64 RA = CPU.GPR[ra];
		CPU.GPR[rd] = 0 - RA;
		if(oe) CPU.SetOV((~RA>>63 == 0) && (~RA>>63 != CPU.GPR[rd]>>63));
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void LBZUX(u32 rd, u32 ra, u32 rb)
	{
		//if(ra == 0 || ra == rd) throw "Bad instruction [LBZUX]";

		const u64 addr = CPU.GPR[ra] + CPU.GPR[rb];
		CPU.GPR[rd] = vm::read8(vm::cast(addr));
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
		vm::write8(vm::cast(addr), CPU.VPR[vs]._u8[15 - eb]);
	}
	void SUBFE(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		const u64 RA = CPU.GPR[ra];
		const u64 RB = CPU.GPR[rb];
		CPU.GPR[rd] = ~RA + RB + CPU.XER.CA;
		CPU.XER.CA = CPU.IsCarry(~RA, RB, CPU.XER.CA);
		if(oe) CPU.SetOV((~RA>>63 == RB>>63) && (~RA>>63 != CPU.GPR[rd]>>63));
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
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
		if(oe) CPU.SetOV((RA>>63 == RB>>63) && (RA>>63 != CPU.GPR[rd]>>63));
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
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
					CPU.SetCR(7 - i, (CPU.GPR[rs] >> (i * 4)) & 0xf);
				}
			}
		}
	}
	void STDX(u32 rs, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		vm::write64(vm::cast(addr), CPU.GPR[rs]);
	}
	void STWCX_(u32 rs, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];

		if (CPU.R_ADDR == addr)
		{
			CPU.SetCR_EQ(0, InterlockedCompareExchange(vm::get_ptr<volatile u32>(vm::cast(CPU.R_ADDR)), re32((u32)CPU.GPR[rs]), (u32)CPU.R_VALUE) == (u32)CPU.R_VALUE);
		}
		else
		{
			CPU.SetCR_EQ(0, false);
		}
		CPU.R_ADDR = 0;
	}
	void STWX(u32 rs, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		vm::write32(vm::cast(addr), (u32)CPU.GPR[rs]);
	}
	void STVEHX(u32 vs, u32 ra, u32 rb)
	{
		const u64 addr = (ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~1ULL;
		const u8 eb = (addr & 0xf) >> 1;
		vm::write16(vm::cast(addr), CPU.VPR[vs]._u16[7 - eb]);
	}
	void STDUX(u32 rs, u32 ra, u32 rb)
	{
		const u64 addr = CPU.GPR[ra] + CPU.GPR[rb];
		vm::write64(vm::cast(addr), CPU.GPR[rs]);
		CPU.GPR[ra] = addr;
	}
	void STWUX(u32 rs, u32 ra, u32 rb)
	{
		const u64 addr = CPU.GPR[ra] + CPU.GPR[rb];
		vm::write32(vm::cast(addr), (u32)CPU.GPR[rs]);
		CPU.GPR[ra] = addr;
	}
	void STVEWX(u32 vs, u32 ra, u32 rb)
	{
		const u64 addr = (ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~3ULL;
		const u8 eb = (addr & 0xf) >> 2;
		vm::write32(vm::cast(addr), CPU.VPR[vs]._u32[3 - eb]);
	}
	void ADDZE(u32 rd, u32 ra, u32 oe, bool rc)
	{
		const u64 RA = CPU.GPR[ra];
		CPU.GPR[rd] = RA + CPU.XER.CA;
		CPU.XER.CA = CPU.IsCarry(RA, CPU.XER.CA);
		if(oe) CPU.SetOV((RA>>63 == 0) && (RA>>63 != CPU.GPR[rd]>>63));
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void SUBFZE(u32 rd, u32 ra, u32 oe, bool rc)
	{
		const u64 RA = CPU.GPR[ra];
		CPU.GPR[rd] = ~RA + CPU.XER.CA;
		CPU.XER.CA = CPU.IsCarry(~RA, CPU.XER.CA);
		if(oe) CPU.SetOV((~RA>>63 == 0) && (~RA>>63 != CPU.GPR[rd]>>63));
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void STDCX_(u32 rs, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];

		if (CPU.R_ADDR == addr)
		{
			CPU.SetCR_EQ(0, InterlockedCompareExchange(vm::get_ptr<volatile u64>(vm::cast(CPU.R_ADDR)), re64(CPU.GPR[rs]), CPU.R_VALUE) == CPU.R_VALUE);
		}
		else
		{
			CPU.SetCR_EQ(0, false);
		}
		CPU.R_ADDR = 0;
	}
	void STBX(u32 rs, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		vm::write8(vm::cast(addr), (u8)CPU.GPR[rs]);
	}
	void STVX(u32 vs, u32 ra, u32 rb)
	{
		vm::write128((u64)((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~0xfULL), CPU.VPR[vs]);
	}
	void SUBFME(u32 rd, u32 ra, u32 oe, bool rc)
	{
		const u64 RA = CPU.GPR[ra];
		CPU.GPR[rd] = ~RA + CPU.XER.CA + ~0ULL;
		CPU.XER.CA = CPU.IsCarry(~RA, CPU.XER.CA, ~0ULL);
		if(oe) CPU.SetOV((~RA>>63 == 1) && (~RA>>63 != CPU.GPR[rd]>>63));
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void MULLD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		const s64 RA = CPU.GPR[ra];
		const s64 RB = CPU.GPR[rb];
		CPU.GPR[rd] = (s64)(RA * RB);
		if(oe)
		{
			const s64 high = __mulh(RA, RB);
			CPU.SetOV(high != s64(CPU.GPR[rd]) >> 63);
		}
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void ADDME(u32 rd, u32 ra, u32 oe, bool rc)
	{
		const s64 RA = CPU.GPR[ra];
		CPU.GPR[rd] = RA + CPU.XER.CA - 1;
		CPU.XER.CA |= RA != 0;

		if(oe) CPU.SetOV((u64(RA)>>63 == 1) && (u64(RA)>>63 != CPU.GPR[rd]>>63));
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void MULLW(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		CPU.GPR[rd] = (s64)((s64)(s32)CPU.GPR[ra] * (s64)(s32)CPU.GPR[rb]);
		if(oe) CPU.SetOV(s64(CPU.GPR[rd]) < s64(-1)<<31 || s64(CPU.GPR[rd]) >= s64(1)<<31);
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void DCBTST(u32 ra, u32 rb, u32 th)
	{
	}
	void STBUX(u32 rs, u32 ra, u32 rb)
	{
		const u64 addr = CPU.GPR[ra] + CPU.GPR[rb];
		vm::write8(vm::cast(addr), (u8)CPU.GPR[rs]);
		CPU.GPR[ra] = addr;
	}
	void ADD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		const u64 RA = CPU.GPR[ra];
		const u64 RB = CPU.GPR[rb];
		CPU.GPR[rd] = RA + RB;
		if(oe) CPU.SetOV((RA>>63 == RB>>63) && (RA>>63 != CPU.GPR[rd]>>63));
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void DCBT(u32 ra, u32 rb, u32 th)
	{
	}
	void LHZX(u32 rd, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		CPU.GPR[rd] = vm::read16(vm::cast(addr));
	}
	void EQV(u32 ra, u32 rs, u32 rb, bool rc)
	{
		CPU.GPR[ra] = ~(CPU.GPR[rs] ^ CPU.GPR[rb]);
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void ECIWX(u32 rd, u32 ra, u32 rb)
	{
		//HACK!
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		CPU.GPR[rd] = vm::read32(vm::cast(addr));
	}
	void LHZUX(u32 rd, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		CPU.GPR[rd] = vm::read16(vm::cast(addr));
		CPU.GPR[ra] = addr;
	}
	void XOR(u32 ra, u32 rs, u32 rb, bool rc)
	{
		CPU.GPR[ra] = CPU.GPR[rs] ^ CPU.GPR[rb];
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void MFSPR(u32 rd, u32 spr)
	{
		CPU.GPR[rd] = ReadSPR(spr);
	}
	void LWAX(u32 rd, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		CPU.GPR[rd] = (s64)(s32)vm::read32(vm::cast(addr));
	}
	void DST(u32 ra, u32 rb, u32 strm, u32 t)
	{
	}
	void LHAX(u32 rd, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		CPU.GPR[rd] = (s64)(s16)vm::read16(vm::cast(addr));
	}
	void LVXL(u32 vd, u32 ra, u32 rb)
	{
		CPU.VPR[vd] = vm::read128((u64)((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~0xfULL));
	}
	void MFTB(u32 rd, u32 spr)
	{
		const u32 n = (spr >> 5) | ((spr & 0x1f) << 5);

		CPU.TB = get_time();
		switch(n)
		{
		case 0x10C: CPU.GPR[rd] = CPU.TB; break;
		case 0x10D: CPU.GPR[rd] = CPU.TB >> 32; break;
		default: throw fmt::Format("mftb r%d, %d", rd, spr);
		}
	}
	void LWAUX(u32 rd, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		CPU.GPR[rd] = (s64)(s32)vm::read32(vm::cast(addr));
		CPU.GPR[ra] = addr;
	}
	void DSTST(u32 ra, u32 rb, u32 strm, u32 t)
	{
	}
	void LHAUX(u32 rd, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		CPU.GPR[rd] = (s64)(s16)vm::read16(vm::cast(addr));
		CPU.GPR[ra] = addr;
	}
	void STHX(u32 rs, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		vm::write16(vm::cast(addr), (u16)CPU.GPR[rs]);
	}
	void ORC(u32 ra, u32 rs, u32 rb, bool rc)
	{
		CPU.GPR[ra] = CPU.GPR[rs] | ~CPU.GPR[rb];
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void ECOWX(u32 rs, u32 ra, u32 rb)
	{
		//HACK!
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		vm::write32(vm::cast(addr), (u32)CPU.GPR[rs]);
	}
	void STHUX(u32 rs, u32 ra, u32 rb)
	{
		const u64 addr = CPU.GPR[ra] + CPU.GPR[rb];
		vm::write16(vm::cast(addr), (u16)CPU.GPR[rs]);
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
			if(oe) CPU.SetOV(true);
			CPU.GPR[rd] = 0;
		}
		else
		{
			if(oe) CPU.SetOV(false);
			CPU.GPR[rd] = RA / RB;
		}

		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void DIVWU(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		const u32 RA = (u32)CPU.GPR[ra];
		const u32 RB = (u32)CPU.GPR[rb];

		if(RB == 0)
		{
			if(oe) CPU.SetOV(true);
			CPU.GPR[rd] = 0;
		}
		else
		{
			if(oe) CPU.SetOV(false);
			CPU.GPR[rd] = RA / RB;
		}

		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void MTSPR(u32 spr, u32 rs)
	{
		WriteSPR(spr, CPU.GPR[rs]);
	}
	void DCBI(u32 ra, u32 rb)
	{
	}
	void NAND(u32 ra, u32 rs, u32 rb, bool rc)
	{
		CPU.GPR[ra] = ~(CPU.GPR[rs] & CPU.GPR[rb]);

		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void STVXL(u32 vs, u32 ra, u32 rb)
	{
		vm::write128((u64)((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~0xfULL), CPU.VPR[vs]);
	}
	void DIVD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		const s64 RA = CPU.GPR[ra];
		const s64 RB = CPU.GPR[rb];

		if (RB == 0 || ((u64)RA == (1ULL << 63) && RB == -1))
		{
			if(oe) CPU.SetOV(true);
			CPU.GPR[rd] = /*(((u64)RA & (1ULL << 63)) && RB == 0) ? -1 :*/ 0;
		}
		else
		{
			if(oe) CPU.SetOV(false);
			CPU.GPR[rd] = RA / RB;
		}

		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void DIVW(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		const s32 RA = (s32)CPU.GPR[ra];
		const s32 RB = (s32)CPU.GPR[rb];

		if (RB == 0 || ((u32)RA == (1 << 31) && RB == -1))
		{
			if(oe) CPU.SetOV(true);
			CPU.GPR[rd] = /*(((u32)RA & (1 << 31)) && RB == 0) ? -1 :*/ 0;
		}
		else
		{
			if(oe) CPU.SetOV(false);
			CPU.GPR[rd] = (u32)(RA / RB);
		}

		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void LVLX(u32 vd, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		const u32 eb = addr & 0xf;

		CPU.VPR[vd].clear();
		for (u32 i = 0; i < 16u - eb; ++i) CPU.VPR[vd]._u8[15 - i] = vm::read8(vm::cast(addr + i));
	}
	void LDBRX(u32 rd, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		CPU.GPR[rd] = vm::get_ref<u64>(vm::cast(addr));
	}
	void LSWX(u32 rd, u32 ra, u32 rb)
	{
		u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		u32 count = CPU.XER.XER & 0x7F;
		for (; count >= 4; count -= 4, addr += 4, rd = (rd+1) & 31)
		{
			CPU.GPR[rd] = vm::get_ref<be_t<u32>>(vm::cast(addr));
		}
		if (count)
		{
			u32 value = 0;
			for (u32 byte = 0; byte < count; byte++)
			{
				u32 byte_value = vm::get_ref<u8>(vm::cast(addr+byte));
				value |= byte_value << ((3^byte)*8);
			}
			CPU.GPR[rd] = value;
		}
	}
	void LWBRX(u32 rd, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		CPU.GPR[rd] = vm::get_ref<u32>(vm::cast(addr));
	}
	void LFSX(u32 frd, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		float val = vm::get_ref<be_t<float>>(vm::cast(addr)).value();
		if (!FPRdouble::IsNaN(val))
		{
			CPU.FPR[frd] = val;
		}
		else
		{
			u64 bits = (u32&)val;
			(u64&)CPU.FPR[frd] = (bits & 0x80000000) << 32 | 7ULL << 60 | (bits & 0x7fffffff) << 29;
		}
	}
	void SRW(u32 ra, u32 rs, u32 rb, bool rc)
	{
		u32 n = CPU.GPR[rb] & 0x1f;
		u32 r = (u32)rotl32((u32)CPU.GPR[rs], 64 - n);
		u32 m = ((u32)CPU.GPR[rb] & 0x20) ? 0 : (u32)rotate_mask[32 + n][63];
		CPU.GPR[ra] = r & m;
		
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
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

		CPU.VPR[vd].clear();
		for (u32 i = 16 - eb; i < 16; ++i) CPU.VPR[vd]._u8[15 - i] = vm::read8(vm::cast(addr + i - 16));
	}
	void LSWI(u32 rd, u32 ra, u32 nb)
	{
		u64 addr = ra ? CPU.GPR[ra] : 0;
		u64 N = nb ? nb : 32;
		u8 reg = rd;

		while (N > 0)
		{
			if (N > 3)
			{
				CPU.GPR[reg] = vm::read32(vm::cast(addr));
				addr += 4;
				N -= 4;
			}
			else
			{
				u32 buf = 0;
				u32 i   = 3;
				while (N > 0)
				{
					N = N - 1;
					buf |= vm::read8(vm::cast(addr)) << (i * 8);
					addr++;
					i--;
				}
				CPU.GPR[reg] = buf;
			}
			reg = (reg + 1) % 32;
		}
	}
	void LFSUX(u32 frd, u32 ra, u32 rb)
	{
		const u64 addr = CPU.GPR[ra] + CPU.GPR[rb];
		float val = vm::get_ref<be_t<float>>(vm::cast(addr)).value();
		if (!FPRdouble::IsNaN(val))
		{
			CPU.FPR[frd] = val;
		}
		else
		{
			u64 bits = (u32&)val;
			(u64&)CPU.FPR[frd] = (bits & 0x80000000) << 32 | 7ULL << 60 | (bits & 0x7fffffff) << 29;
		}
		CPU.GPR[ra] = addr;
	}
	void SYNC(u32 l)
	{
		_mm_mfence();
	}
	void LFDX(u32 frd, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		CPU.FPR[frd] = vm::get_ref<be_t<double>>(vm::cast(addr)).value();
	}
	void LFDUX(u32 frd, u32 ra, u32 rb)
	{
		const u64 addr = CPU.GPR[ra] + CPU.GPR[rb];
		CPU.FPR[frd] = vm::get_ref<be_t<double>>(vm::cast(addr)).value();
		CPU.GPR[ra] = addr;
	}
	void STVLX(u32 vs, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		const u32 eb = addr & 0xf;

		for (u32 i = 0; i < 16u - eb; ++i) vm::write8(vm::cast(addr + i), CPU.VPR[vs]._u8[15 - i]);
	}
	void STDBRX(u32 rs, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		vm::get_ref<u64>(vm::cast(addr)) = CPU.GPR[rs];
	}
	void STSWX(u32 rs, u32 ra, u32 rb)
	{
		u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		u32 count = CPU.XER.XER & 0x7F;
		for (; count >= 4; count -= 4, addr += 4, rs = (rs+1) & 31)
		{
			vm::write32(vm::cast(addr), (u32)CPU.GPR[rs]);
		}
		if (count)
		{
			u32 value = (u32)CPU.GPR[rs];
			for (u32 byte = 0; byte < count; byte++)
			{
				u32 byte_value = (u8)(value >> ((3^byte)*8));
				vm::write8(vm::cast(addr+byte), byte_value);
			}
		}
	}
	void STWBRX(u32 rs, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		vm::get_ref<u32>(vm::cast(addr)) = (u32)CPU.GPR[rs];
	}
	void STFSX(u32 frs, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		double val = CPU.FPR[frs];
		if (!FPRdouble::IsNaN(val))
		{
			vm::get_ref<be_t<float>>(vm::cast(addr)) = (float)val;
		}
		else
		{
			u64 bits = (u64&)val;
			u32 bits32 = (bits>>32 & 0x80000000) | (bits>>29 & 0x7fffffff);
			vm::get_ref<be_t<u32>>(vm::cast(addr)) = (float)bits32;
		}
	}
	void STVRX(u32 vs, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		const u8 eb = addr & 0xf;

		for (u32 i = 16 - eb; i < 16; ++i) vm::write8(vm::cast(addr + i - 16), CPU.VPR[vs]._u8[15 - i]);
	}
	void STFSUX(u32 frs, u32 ra, u32 rb)
	{
		const u64 addr = CPU.GPR[ra] + CPU.GPR[rb];
		double val = CPU.FPR[frs];
		if (!FPRdouble::IsNaN(val))
		{
			vm::get_ref<be_t<float>>(vm::cast(addr)) = (float)val;
		}
		else
		{
			u64 bits = (u64&)val;
			u32 bits32 = (bits>>32 & 0x80000000) | (bits>>29 & 0x7fffffff);
			vm::get_ref<be_t<u32>>(vm::cast(addr)) = (float)bits32;
		}
		CPU.GPR[ra] = addr;
	}
	void STSWI(u32 rd, u32 ra, u32 nb)
	{
		u64 addr = ra ? CPU.GPR[ra] : 0;
		u64 N = nb ? nb : 32;
		u8 reg = rd;

		while (N > 0)
		{
			if (N > 3)
			{
				vm::write32(vm::cast(addr), (u32)CPU.GPR[reg]);
				addr += 4;
				N -= 4;
			}
			else
			{
				u32 buf = (u32)CPU.GPR[reg];
				while (N > 0)
				{
					N = N - 1;
					vm::write8(vm::cast(addr), (0xFF000000 & buf) >> 24);
					buf <<= 8;
					addr++;
				}
			}
			reg = (reg + 1) % 32;
		}
	}
	void STFDX(u32 frs, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		vm::get_ref<be_t<double>>(vm::cast(addr)) = CPU.FPR[frs];
	}
	void STFDUX(u32 frs, u32 ra, u32 rb)
	{
		const u64 addr = CPU.GPR[ra] + CPU.GPR[rb];
		vm::get_ref<be_t<double>>(vm::cast(addr)) = CPU.FPR[frs];
		CPU.GPR[ra] = addr;
	}
	void LVLXL(u32 vd, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		const u32 eb = addr & 0xf;

		CPU.VPR[vd].clear();
		for (u32 i = 0; i < 16u - eb; ++i) CPU.VPR[vd]._u8[15 - i] = vm::read8(vm::cast(addr + i));
	}
	void LHBRX(u32 rd, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		CPU.GPR[rd] = vm::get_ref<u16>(vm::cast(addr));
	}
	void SRAW(u32 ra, u32 rs, u32 rb, bool rc)
	{
		s32 RS = (s32)CPU.GPR[rs];
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

		CPU.VPR[vd].clear();
		for (u32 i = 16 - eb; i < 16; ++i) CPU.VPR[vd]._u8[15 - i] = vm::read8(vm::cast(addr + i - 16));
	}
	void DSS(u32 strm, u32 a)
	{
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
		const u32 eb = addr & 0xf;

		for (u32 i = 0; i < 16u - eb; ++i) vm::write8(vm::cast(addr + i), CPU.VPR[vs]._u8[15 - i]);
	}
	void STHBRX(u32 rs, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		vm::get_ref<u16>(vm::cast(addr)) = (u16)CPU.GPR[rs];
	}
	void EXTSH(u32 ra, u32 rs, bool rc)
	{
		CPU.GPR[ra] = (s64)(s16)CPU.GPR[rs];
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void STVRXL(u32 vs, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		const u8 eb = addr & 0xf;

		for (u32 i = 16 - eb; i < 16; ++i) vm::write8(vm::cast(addr + i - 16), CPU.VPR[vs]._u8[15 - i]);
	}
	void EXTSB(u32 ra, u32 rs, bool rc)
	{
		CPU.GPR[ra] = (s64)(s8)CPU.GPR[rs];
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void STFIWX(u32 frs, u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
		vm::write32(vm::cast(addr), (u32&)CPU.FPR[frs]);
	}
	void EXTSW(u32 ra, u32 rs, bool rc)
	{
		CPU.GPR[ra] = (s64)(s32)CPU.GPR[rs];
		if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void ICBI(u32 ra, u32 rs)
	{
		// Clear jit for the specified block?  Nothing to do in the interpreter.
	}
	void DCBZ(u32 ra, u32 rb)
	{
		const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];

		auto const cache_line = vm::get_ptr<u8>(vm::cast(addr) & ~127);
		if (cache_line)
			memset(cache_line, 0, 128);
	}
	void LWZ(u32 rd, u32 ra, s32 d)
	{
		const u64 addr = ra ? CPU.GPR[ra] + d : d;
		CPU.GPR[rd] = vm::read32(vm::cast(addr));
	}
	void LWZU(u32 rd, u32 ra, s32 d)
	{
		const u64 addr = CPU.GPR[ra] + d;
		CPU.GPR[rd] = vm::read32(vm::cast(addr));
		CPU.GPR[ra] = addr;
	}
	void LBZ(u32 rd, u32 ra, s32 d)
	{
		const u64 addr = ra ? CPU.GPR[ra] + d : d;
		CPU.GPR[rd] = vm::read8(vm::cast(addr));
	}
	void LBZU(u32 rd, u32 ra, s32 d)
	{
		const u64 addr = CPU.GPR[ra] + d;
		CPU.GPR[rd] = vm::read8(vm::cast(addr));
		CPU.GPR[ra] = addr;
	}
	void STW(u32 rs, u32 ra, s32 d)
	{
		const u64 addr = ra ? CPU.GPR[ra] + d : d;
		vm::write32(vm::cast(addr), (u32)CPU.GPR[rs]);
	}
	void STWU(u32 rs, u32 ra, s32 d)
	{
		const u64 addr = CPU.GPR[ra] + d;
		vm::write32(vm::cast(addr), (u32)CPU.GPR[rs]);
		CPU.GPR[ra] = addr;
	}
	void STB(u32 rs, u32 ra, s32 d)
	{
		const u64 addr = ra ? CPU.GPR[ra] + d : d;
		vm::write8(vm::cast(addr), (u8)CPU.GPR[rs]);
	}
	void STBU(u32 rs, u32 ra, s32 d)
	{
		const u64 addr = CPU.GPR[ra] + d;
		vm::write8(vm::cast(addr), (u8)CPU.GPR[rs]);
		CPU.GPR[ra] = addr;
	}
	void LHZ(u32 rd, u32 ra, s32 d)
	{
		const u64 addr = ra ? CPU.GPR[ra] + d : d;
		CPU.GPR[rd] = vm::read16(vm::cast(addr));
	}
	void LHZU(u32 rd, u32 ra, s32 d)
	{
		const u64 addr = CPU.GPR[ra] + d;
		CPU.GPR[rd] = vm::read16(vm::cast(addr));
		CPU.GPR[ra] = addr;
	}
	void LHA(u32 rd, u32 ra, s32 d)
	{
		const u64 addr = ra ? CPU.GPR[ra] + d : d;
		CPU.GPR[rd] = (s64)(s16)vm::read16(vm::cast(addr));
	}
	void LHAU(u32 rd, u32 ra, s32 d)
	{
		const u64 addr = CPU.GPR[ra] + d;
		CPU.GPR[rd] = (s64)(s16)vm::read16(vm::cast(addr));
		CPU.GPR[ra] = addr;
	}
	void STH(u32 rs, u32 ra, s32 d)
	{
		const u64 addr = ra ? CPU.GPR[ra] + d : d;
		vm::write16(vm::cast(addr), (u16)CPU.GPR[rs]);
	}
	void STHU(u32 rs, u32 ra, s32 d)
	{
		const u64 addr = CPU.GPR[ra] + d;
		vm::write16(vm::cast(addr), (u16)CPU.GPR[rs]);
		CPU.GPR[ra] = addr;
	}
	void LMW(u32 rd, u32 ra, s32 d)
	{
		u64 addr = ra ? CPU.GPR[ra] + d : d;
		for(u32 i=rd; i<32; ++i, addr += 4)
		{
			CPU.GPR[i] = vm::read32(vm::cast(addr));
		}
	}
	void STMW(u32 rs, u32 ra, s32 d)
	{
		u64 addr = ra ? CPU.GPR[ra] + d : d;
		for(u32 i=rs; i<32; ++i, addr += 4)
		{
			vm::write32(vm::cast(addr), (u32)CPU.GPR[i]);
		}
	}
	void LFS(u32 frd, u32 ra, s32 d)
	{
		const u64 addr = ra ? CPU.GPR[ra] + d : d;
		float val = vm::get_ref<be_t<float>>(vm::cast(addr)).value();
		if (!FPRdouble::IsNaN(val))
		{
			CPU.FPR[frd] = val;
		}
		else
		{
			u64 bits = (u32&)val;
			(u64&)CPU.FPR[frd] = (bits & 0x80000000) << 32 | 7ULL << 60 | (bits & 0x7fffffff) << 29;
		}
	}
	void LFSU(u32 frd, u32 ra, s32 ds)
	{
		const u64 addr = CPU.GPR[ra] + ds;
		float val = vm::get_ref<be_t<float>>(vm::cast(addr)).value();
		if (!FPRdouble::IsNaN(val))
		{
			CPU.FPR[frd] = val;
		}
		else
		{
			u64 bits = (u32&)val;
			(u64&)CPU.FPR[frd] = (bits & 0x80000000) << 32 | 7ULL << 60 | (bits & 0x7fffffff) << 29;
		}
		CPU.GPR[ra] = addr;
	}
	void LFD(u32 frd, u32 ra, s32 d)
	{
		const u64 addr = ra ? CPU.GPR[ra] + d : d;
		CPU.FPR[frd] = vm::get_ref<be_t<double>>(vm::cast(addr)).value();
	}
	void LFDU(u32 frd, u32 ra, s32 ds)
	{
		const u64 addr = CPU.GPR[ra] + ds;
		CPU.FPR[frd] = vm::get_ref<be_t<double>>(vm::cast(addr)).value();
		CPU.GPR[ra] = addr;
	}
	void STFS(u32 frs, u32 ra, s32 d)
	{
		const u64 addr = ra ? CPU.GPR[ra] + d : d;
		double val = CPU.FPR[frs];
		if (!FPRdouble::IsNaN(val))
		{
			vm::get_ref<be_t<float>>(vm::cast(addr)) = (float)val;
		}
		else
		{
			u64 bits = (u64&)val;
			u32 bits32 = (bits>>32 & 0x80000000) | (bits>>29 & 0x7fffffff);
			vm::get_ref<be_t<u32>>(vm::cast(addr)) = (float)bits32;
		}
	}
	void STFSU(u32 frs, u32 ra, s32 d)
	{
		const u64 addr = CPU.GPR[ra] + d;
		double val = CPU.FPR[frs];
		if (!FPRdouble::IsNaN(val))
		{
			vm::get_ref<be_t<float>>(vm::cast(addr)) = (float)val;
		}
		else
		{
			u64 bits = (u64&)val;
			u32 bits32 = (bits>>32 & 0x80000000) | (bits>>29 & 0x7fffffff);
			vm::get_ref<be_t<u32>>(vm::cast(addr)) = (float)bits32;
		}
		CPU.GPR[ra] = addr;
	}
	void STFD(u32 frs, u32 ra, s32 d)
	{
		const u64 addr = ra ? CPU.GPR[ra] + d : d;
		vm::get_ref<be_t<double>>(vm::cast(addr)) = CPU.FPR[frs];
	}
	void STFDU(u32 frs, u32 ra, s32 d)
	{
		const u64 addr = CPU.GPR[ra] + d;
		vm::get_ref<be_t<double>>(vm::cast(addr)) = CPU.FPR[frs];
		CPU.GPR[ra] = addr;
	}
	void LD(u32 rd, u32 ra, s32 ds)
	{
		const u64 addr = ra ? CPU.GPR[ra] + ds : ds;
		CPU.GPR[rd] = vm::read64(vm::cast(addr));
	}
	void LDU(u32 rd, u32 ra, s32 ds)
	{
		//if(ra == 0 || rt == ra) return;
		const u64 addr = CPU.GPR[ra] + ds;
		CPU.GPR[rd] = vm::read64(vm::cast(addr));
		CPU.GPR[ra] = addr;
	}
	void LWA(u32 rd, u32 ra, s32 ds)
	{
		const u64 addr = ra ? CPU.GPR[ra] + ds : ds;
		CPU.GPR[rd] = (s64)(s32)vm::read32(vm::cast(addr));
	}
	void FDIVS(u32 frd, u32 fra, u32 frb, bool rc) {FDIV(frd, fra, frb, rc, true);}
	void FSUBS(u32 frd, u32 fra, u32 frb, bool rc) {FSUB(frd, fra, frb, rc, true);}
	void FADDS(u32 frd, u32 fra, u32 frb, bool rc) {FADD(frd, fra, frb, rc, true);}
	void FSQRTS(u32 frd, u32 frb, bool rc) {FSQRT(frd, frb, rc, true);}
	void FRES(u32 frd, u32 frb, bool rc)
	{
		if(CPU.FPR[frb] == 0.0)
		{
			CPU.SetFPSCRException(FPSCR_ZX);
		}
		CPU.FPR[frd] = static_cast<float>(1.0 / CPU.FPR[frb]);
		if(rc) CPU.UpdateCR1();
	}
	void FMULS(u32 frd, u32 fra, u32 frc, bool rc) {FMUL(frd, fra, frc, rc, true);}
	void FMADDS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {FMADD(frd, fra, frc, frb, rc, false, false, true);}
	void FMSUBS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {FMADD(frd, fra, frc, frb, rc, false, true, true);}
	void FNMSUBS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {FMADD(frd, fra, frc, frb, rc, true, true, true);}
	void FNMADDS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {FMADD(frd, fra, frc, frb, rc, true, false, true);}
	void STD(u32 rs, u32 ra, s32 d)
	{
		const u64 addr = ra ? CPU.GPR[ra] + d : d;
		vm::write64(vm::cast(addr), CPU.GPR[rs]);
	}
	void STDU(u32 rs, u32 ra, s32 ds)
	{
		//if(ra == 0 || rs == ra) return;
		const u64 addr = CPU.GPR[ra] + ds;
		vm::write64(vm::cast(addr), CPU.GPR[rs]);
		CPU.GPR[ra] = addr;
	}
	void MTFSB1(u32 crbd, bool rc)
	{
		u64 mask = (1ULL << (31 - crbd));
		if ((crbd >= 3 && crbd <= 6) && !(CPU.FPSCR.FPSCR & mask)) mask |= 1ULL << 31;  //FPSCR.FX
		if ((crbd == 29) && !CPU.FPSCR.NI) LOG_WARNING(PPU, "Non-IEEE mode enabled");
		CPU.SetFPSCR(CPU.FPSCR.FPSCR | mask);

		if(rc) CPU.UpdateCR1();
	}
	void MCRFS(u32 crbd, u32 crbs)
	{
		CPU.SetCR(crbd, (CPU.FPSCR.FPSCR >> ((7 - crbs) * 4)) & 0xf);
		const u32 exceptions_mask = 0x9FF80700;
		CPU.SetFPSCR(CPU.FPSCR.FPSCR & ~(exceptions_mask & 0xf << ((7 - crbs) * 4)));
	}
	void MTFSB0(u32 crbd, bool rc)
	{
		u64 mask = (1ULL << (31 - crbd));
		if ((crbd == 29) && !CPU.FPSCR.NI) LOG_WARNING(PPU, "Non-IEEE mode disabled");
		CPU.SetFPSCR(CPU.FPSCR.FPSCR & ~mask);

		if(rc) CPU.UpdateCR1();
	}
	void MTFSFI(u32 crfd, u32 i, bool rc)
	{
		u32 mask = 0xF0000000 >> (crfd * 4);
		u32 val  = (i & 0xF) << ((7 - crfd) * 4);

		const u32 oldNI  = CPU.FPSCR.NI;
		CPU.SetFPSCR((CPU.FPSCR.FPSCR & ~mask) | val);
		if (CPU.FPSCR.NI != oldNI)
		{
			if (oldNI)
				LOG_WARNING(PPU, "Non-IEEE mode disabled");
			else
				LOG_WARNING(PPU, "Non-IEEE mode enabled");
		}

		if(rc) CPU.UpdateCR1();
	}
	void MFFS(u32 frd, bool rc)
	{
		(u64&)CPU.FPR[frd] = CPU.FPSCR.FPSCR;
		if(rc) CPU.UpdateCR1();
	}
	void MTFSF(u32 flm, u32 frb, bool rc)
	{
		u32 mask = 0;
		for(u32 i=0; i<8; ++i)
		{
			if(flm & (1 << i)) mask |= 0xf << (i * 4);
		}
		mask &= ~0x60000000;

		const u32 oldNI = CPU.FPSCR.NI;
		CPU.SetFPSCR((CPU.FPSCR.FPSCR & ~mask) | ((u32&)CPU.FPR[frb] & mask));
		if (CPU.FPSCR.NI != oldNI)
		{
			if (oldNI)
				LOG_WARNING(PPU, "Non-IEEE mode disabled");
			else
				LOG_WARNING(PPU, "Non-IEEE mode enabled");
		}
		if(rc) CPU.UpdateCR1();
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
		u32 type = PPCdouble(r).GetType();
		if (type == FPR_PN && r < ldexp(1.0, -126)) type = FPR_PD;
		else if (type == FPR_NN && r > ldexp(-1.0, -126)) type = FPR_ND;
		CPU.FPSCR.FPRF = type;
		CPU.FPR[frd] = r;
	}
	void FCTIW(u32 frd, u32 frb, bool rc) {FCTIW(frd, frb, rc, false);}
	void FCTIW(u32 frd, u32 frb, bool rc, bool truncate)
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
			const u32 rn = truncate ? FPSCR_RN_ZERO : CPU.FPSCR.RN;
			switch(rn)
			{
			case FPSCR_RN_NEAR:
				i = (s32)nearbyint(b);
				break;
			case FPSCR_RN_ZERO:
				i = (s32)b;
				break;
			case FPSCR_RN_PINF:
				i = (s32)ceil(b);
				break;
			case FPSCR_RN_MINF:
				i = (s32)floor(b);
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

		(u64&)CPU.FPR[frd] = r;
		if(rc) CPU.UpdateCR1();
	}
	void FCTIWZ(u32 frd, u32 frb, bool rc) {FCTIW(frd, frb, rc, true);}
	void FDIV(u32 frd, u32 fra, u32 frb, bool rc) {FDIV(frd, fra, frb, rc, false);}
	void FDIV(u32 frd, u32 fra, u32 frb, bool rc, bool single)
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

		if(single) CPU.FPR[frd] = (float)res;
		else       CPU.FPR[frd] = res;
		CPU.FPSCR.FPRF = CPU.FPR[frd].GetType();
		if(rc) CPU.UpdateCR1();
	}
	void FSUB(u32 frd, u32 fra, u32 frb, bool rc) {FSUB(frd, fra, frb, rc, false);}
	void FSUB(u32 frd, u32 fra, u32 frb, bool rc, bool single)
	{
		const double res = CPU.FPR[fra] - CPU.FPR[frb];
		if(single) CPU.FPR[frd] = (float)res;
		else       CPU.FPR[frd] = res;
		CPU.FPSCR.FPRF = CPU.FPR[frd].GetType();
		if(rc) CPU.UpdateCR1();
	}
	void FADD(u32 frd, u32 fra, u32 frb, bool rc) {FADD(frd, fra, frb, rc, false);}
	void FADD(u32 frd, u32 fra, u32 frb, bool rc, bool single)
	{
		const double res = CPU.FPR[fra] + CPU.FPR[frb];
		if(single) CPU.FPR[frd] = (float)res;
		else       CPU.FPR[frd] = res;
		CPU.FPSCR.FPRF = CPU.FPR[frd].GetType();
		if(rc) CPU.UpdateCR1();
	}
	void FSQRT(u32 frd, u32 frb, bool rc) {FSQRT(frd, frb, rc, false);}
	void FSQRT(u32 frd, u32 frb, bool rc, bool single)
	{
		const double res = sqrt(CPU.FPR[frb]);
		if(single) CPU.FPR[frd] = (float)res;
		else       CPU.FPR[frd] = res;
		CPU.FPSCR.FPRF = CPU.FPR[frd].GetType();
		if(rc) CPU.UpdateCR1();
	}
	void FSEL(u32 frd, u32 fra, u32 frc, u32 frb, bool rc)
	{
		CPU.FPR[frd] = CPU.FPR[fra] >= 0.0 ? CPU.FPR[frc] : CPU.FPR[frb];
		if(rc) CPU.UpdateCR1();
	}
	void FMUL(u32 frd, u32 fra, u32 frc, bool rc) {FMUL(frd, fra, frc, rc, false);}
	void FMUL(u32 frd, u32 fra, u32 frc, bool rc, bool single)
	{
		double res;
		if((FPRdouble::IsINF(CPU.FPR[fra]) && CPU.FPR[frc] == 0.0) || (FPRdouble::IsINF(CPU.FPR[frc]) && CPU.FPR[fra] == 0.0))
		{
			CPU.SetFPSCRException(FPSCR_VXIMZ);
			res = FPR_NAN;
			CPU.FPSCR.FI = 0;
			CPU.FPSCR.FR = 0;
		}
		else
		{
			if(FPRdouble::IsSNaN(CPU.FPR[fra]) || FPRdouble::IsSNaN(CPU.FPR[frc]))
			{
				CPU.SetFPSCRException(FPSCR_VXSNAN);
			}

			res = CPU.FPR[fra] * CPU.FPR[frc];
		}

		if(single) CPU.FPR[frd] = (float)res;
		else       CPU.FPR[frd] = res;
		CPU.FPSCR.FPRF = CPU.FPR[frd].GetType();
		if(rc) CPU.UpdateCR1();
	}
	void FRSQRTE(u32 frd, u32 frb, bool rc)
	{
		if(CPU.FPR[frb] == 0.0)
		{
			CPU.SetFPSCRException(FPSCR_ZX);
		}
		CPU.FPR[frd] = 1.0 / sqrt(CPU.FPR[frb]);
		if(rc) CPU.UpdateCR1();
	}
	void FMSUB(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {FMADD(frd, fra, frc, frb, rc, false, true, false);}
	void FMADD(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {FMADD(frd, fra, frc, frb, rc, false, false, false);}
	void FMADD(u32 frd, u32 fra, u32 frc, u32 frb, bool rc, bool neg, bool sub, bool single)
	{
		const double a = CPU.FPR[fra];
		const double b = CPU.FPR[frb];
		const double c = CPU.FPR[frc];
		const double res = fma(a, c, sub ? -b : b);
		if(single) CPU.FPR[frd] = (float)(neg ? -res : res);
		else       CPU.FPR[frd] = (neg ? -res : res);
		CPU.FPSCR.FPRF = CPU.FPR[frd].GetType();
		if(rc) CPU.UpdateCR1();
	}
	void FNMSUB(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {FMADD(frd, fra, frc, frb, rc, true, true, false);}
	void FNMADD(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {FMADD(frd, fra, frc, frb, rc, true, false, false);}
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
		if(rc) CPU.UpdateCR1();
	}
	void FMR(u32 frd, u32 frb, bool rc)
	{
		CPU.FPR[frd] = CPU.FPR[frb];
		if(rc) CPU.UpdateCR1();
	}
	void FNABS(u32 frd, u32 frb, bool rc)
	{
		CPU.FPR[frd] = -fabs(CPU.FPR[frb]);
		if(rc) CPU.UpdateCR1();
	}
	void FABS(u32 frd, u32 frb, bool rc)
	{
		CPU.FPR[frd] = fabs(CPU.FPR[frb]);
		if(rc) CPU.UpdateCR1();
	}
	void FCTID(u32 frd, u32 frb, bool rc) {FCTID(frd, frb, rc, false);}
	void FCTID(u32 frd, u32 frb, bool rc, bool truncate)
	{
		const double b = CPU.FPR[frb];
		u64 r;
		if(b >= (double)0x8000000000000000)
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
			const u32 rn = truncate ? FPSCR_RN_ZERO : CPU.FPSCR.RN;
			switch(rn)
			{
			case FPSCR_RN_NEAR:
				i = (s64)nearbyint(b);
				break;
			case FPSCR_RN_ZERO:
				i = (s64)b;
				break;
			case FPSCR_RN_PINF:
				i = (s64)ceil(b);
				break;
			case FPSCR_RN_MINF:
				i = (s64)floor(b);
				break;
			}
			r = (u64)i;
			double di = (double)i;
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

		(u64&)CPU.FPR[frd] = r;
		if(rc) CPU.UpdateCR1();
	}
	void FCTIDZ(u32 frd, u32 frb, bool rc) {FCTID(frd, frb, rc, true);}
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
		if(rc) CPU.UpdateCR1();
	}

	void UNK(const u32 code, const u32 opcode, const u32 gcode)
	{
		throw fmt::Format("Unknown/Illegal opcode! (0x%08x : 0x%x : 0x%x)", code, opcode, gcode);
	}
};
