#pragma once

#include "Emu/Cell/PPUOpcodes.h"
#include "Emu/Memory/Memory.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/SysCalls/SysCalls.h"
#include "rpcs3.h"
#include <stdint.h>

#define START_OPCODES_GROUP(x) /*x*/
#define END_OPCODES_GROUP(x) /*x*/
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

#define rotl32 _rotl
#define rotr32 _rotr
#define rotl64 _rotl64
#define rotr64 _rotr64

class PPU_Interpreter : public PPU_Opcodes
{
private:
	PPUThread& CPU;

public:
	PPU_Interpreter(PPUThread& cpu) : CPU(cpu)
	{
		InitRotateMask();
	}

private:
	void Exit() {}

	void SysCall()
	{
		CPU.GPR[3] = CPU.DoSyscall(CPU.GPR[11]);

		//if((s32)CPU.GPR[3] < 0)
			//ConLog.Warning("SysCall[%lld] done with code [0x%x]! #pc: 0x%llx", CPU.GPR[11], (u32)CPU.GPR[3], CPU.PC);
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

		int fpc = _fpclass(v);
		if(fpc & _FPCLASS_ND) return -0.0f;
		if(fpc & _FPCLASS_PD) return  0.0f;

		return v;
	}

	bool CheckCondition(OP_uIMM bo, OP_uIMM bi)
	{
		const u8 bo0 = (bo & 0x10) ? 1 : 0;
		const u8 bo1 = (bo & 0x08) ? 1 : 0;
		const u8 bo2 = (bo & 0x04) ? 1 : 0;
		const u8 bo3 = (bo & 0x02) ? 1 : 0;
		const u8 bo4 = (bo & 0x01) ? 1 : 0;

		if(!bo2) --CPU.CTR;

		const u8 ctr_ok = bo2 | ((CPU.CTR != 0) ^ bo3);
		const u8 cond_ok = bo0 | (CPU.IsCR(bi) ^ (~bo1 & 0x1));

		//if(bo1) CPU.SetCR(bi, bo4 ? 1 : 0);
		//if(bo1) return !bo4;

		//ConLog.Write("bo0: 0x%x, bo1: 0x%x, bo2: 0x%x, bo3: 0x%x", bo0, bo1, bo2, bo3);

		return ctr_ok && cond_ok;
	}

	u64& GetRegBySPR(OP_uIMM spr)
	{
		const u32 n = (spr >> 5) | ((spr & 0x1f) << 5);

		switch(n)
		{
		case 0x001: return CPU.XER.XER;
		case 0x008: return CPU.LR;
		case 0x009: return CPU.CTR;
		case 0x100: return CPU.USPRG0;
		}

		UNK(wxString::Format("GetRegBySPR error: Unknown SPR 0x%x!", n));
		return CPU.XER.XER;
	}
	
	void TDI(OP_uIMM to, OP_REG ra, OP_sIMM simm16)
	{
		s64 a = CPU.GPR[ra];

		if( (a < (s64)simm16  && (to & 0x10)) ||
			(a > (s64)simm16  && (to & 0x8))  ||
			(a == (s64)simm16 && (to & 0x4))  ||
			((u64)a < (u64)simm16 && (to & 0x2)) ||
			((u64)a > (u64)simm16 && (to & 0x1)) )
		{
			UNK(wxString::Format("Trap! (tdi %x, r%d, %x)", to, ra, simm16));
		}
	}

	void TWI(OP_uIMM to, OP_REG ra, OP_sIMM simm16)
	{
		s32 a = CPU.GPR[ra];

		if( (a < simm16  && (to & 0x10)) ||
			(a > simm16  && (to & 0x8))  ||
			(a == simm16 && (to & 0x4))  ||
			((u32)a < (u32)simm16 && (to & 0x2)) ||
			((u32)a > (u32)simm16 && (to & 0x1)) )
		{
			UNK(wxString::Format("Trap! (twi %x, r%d, %x)", to, ra, simm16));
		}
	}

	START_OPCODES_GROUP(G_04)
		void MFVSCR(OP_REG vd)
		{
			CPU.VPR[vd].Clear();
			CPU.VPR[vd]._u32[0] = CPU.VSCR.VSCR;
		}
		void MTVSCR(OP_REG vb)
		{
			CPU.VSCR.VSCR = CPU.VPR[vb]._u32[0];
			CPU.VSCR.X = CPU.VSCR.Y = 0;
		}
		void VADDCUW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._u32[w] = ~CPU.VPR[va]._u32[w] < CPU.VPR[vb]._u32[w];
			}
		}
		void VADDFP(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._f[w] = CPU.VPR[va]._f[w] + CPU.VPR[vb]._f[w];
			}
		}
		void VADDSBS(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VADDSHS(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VADDSWS(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VADDUBM(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint b = 0; b < 16; b++)
			{
				CPU.VPR[vd]._u8[b] = CPU.VPR[va]._u8[b] + CPU.VPR[vb]._u8[b];
			}
		}
		void VADDUBS(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VADDUHM(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint h = 0; h < 8; h++)
			{
				CPU.VPR[vd]._u16[h] = CPU.VPR[va]._u16[h] + CPU.VPR[vb]._u16[h];
			}
		}
		void VADDUHS(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VADDUWM(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._u32[w] = CPU.VPR[va]._u32[w] + CPU.VPR[vb]._u32[w];
			}
		}
		void VADDUWS(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VAND(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._u32[w] = CPU.VPR[va]._u32[w] & CPU.VPR[vb]._u32[w];
			}
		}
		void VANDC(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._u32[w] = CPU.VPR[va]._u32[w] & (~CPU.VPR[vb]._u32[w]);
			}
		}
		void VAVGSB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint b = 0; b < 16; b++)
			{
				CPU.VPR[vd]._s8[b] = (CPU.VPR[va]._s8[b] + CPU.VPR[vb]._s8[b] + 1) >> 1;
			}
		}
		void VAVGSH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint h = 0; h < 8; h++)
			{
				CPU.VPR[vd]._s16[h] = (CPU.VPR[va]._s16[h] + CPU.VPR[vb]._s16[h] + 1) >> 1;
			}
		}
		void VAVGSW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._s32[w] = ((s64)CPU.VPR[va]._s32[w] + (s64)CPU.VPR[vb]._s32[w] + 1) >> 1;
			}
		}
		void VAVGUB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint b = 0; b < 16; b++)
				CPU.VPR[vd]._u8[b] = (CPU.VPR[va]._u8[b] + CPU.VPR[vb]._u8[b] + 1) >> 1;
		}
		void VAVGUH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint h = 0; h < 8; h++)
			{
				CPU.VPR[vd]._u16[h] = (CPU.VPR[va]._u16[h] + CPU.VPR[vb]._u16[h] + 1) >> 1;
			}
		}
		void VAVGUW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._u32[w] = ((u64)CPU.VPR[va]._u32[w] + (u64)CPU.VPR[vb]._u32[w] + 1) >> 1;
			}
		}
		void VCFSX(OP_REG vd, OP_uIMM uimm5, OP_REG vb)
		{
			u32 scale = 1 << uimm5;
			
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._f[w] = ((float)CPU.VPR[vb]._s32[w]) / scale;
			}
		}
		void VCFUX(OP_REG vd, OP_uIMM uimm5, OP_REG vb)
		{
			u32 scale = 1 << uimm5;
			
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._f[w] = ((float)CPU.VPR[vb]._u32[w]) / scale;
			}
		}
		void VCMPBFP(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VCMPBFP_(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VCMPEQFP(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._u32[w] = CPU.VPR[va]._f[w] == CPU.VPR[vb]._f[w] ? 0xffffffff : 0;
			}
		}
		void VCMPEQFP_(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VCMPEQUB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint b = 0; b < 16; b++)
			{
				CPU.VPR[vd]._u8[b] = CPU.VPR[va]._u8[b] == CPU.VPR[vb]._u8[b] ? 0xff : 0;
			}
		}
		void VCMPEQUB_(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VCMPEQUH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint h = 0; h < 8; h++)
			{
				CPU.VPR[vd]._u16[h] = CPU.VPR[va]._u16[h] == CPU.VPR[vb]._u16[h] ? 0xffff : 0;
			}
		}
		void VCMPEQUH_(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VCMPEQUW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._u32[w] = CPU.VPR[va]._u32[w] == CPU.VPR[vb]._u32[w] ? 0xffffffff : 0;
			}
		}
		void VCMPEQUW_(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VCMPGEFP(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._u32[w] = CPU.VPR[va]._f[w] >= CPU.VPR[vb]._f[w] ? 0xffffffff : 0;
			}
		}
		void VCMPGEFP_(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VCMPGTFP(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._u32[w] = CPU.VPR[va]._f[w] > CPU.VPR[vb]._f[w] ? 0xffffffff : 0;
			}
		}
		void VCMPGTFP_(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VCMPGTSB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint b = 0; b < 16; b++)
			{
				CPU.VPR[vd]._u8[b] = CPU.VPR[va]._s8[b] > CPU.VPR[vb]._s8[b] ? 0xff : 0;
			}
		}
		void VCMPGTSB_(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VCMPGTSH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint h = 0; h < 8; h++)
			{
				CPU.VPR[vd]._u16[h] = CPU.VPR[va]._s16[h] > CPU.VPR[vb]._s16[h] ? 0xffff : 0;
			}
		}
		void VCMPGTSH_(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VCMPGTSW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._u32[w] = CPU.VPR[va]._s32[w] > CPU.VPR[vb]._s32[w] ? 0xffffffff : 0;
			}
		}
		void VCMPGTSW_(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VCMPGTUB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint b = 0; b < 16; b++)
			{
				CPU.VPR[vd]._u8[b] = CPU.VPR[va]._u8[b] > CPU.VPR[vb]._u8[b] ? 0xff : 0;
			}
		}
		void VCMPGTUB_(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VCMPGTUH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint h = 0; h < 8; h++)
			{
				CPU.VPR[vd]._u16[h] = CPU.VPR[va]._u16[h] > CPU.VPR[vb]._u16[h] ? 0xffff : 0;
			}
		}
		void VCMPGTUH_(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VCMPGTUW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._u32[w] = CPU.VPR[va]._u32[w] > CPU.VPR[vb]._u32[w] ? 0xffffffff : 0;
			}
		}
		void VCMPGTUW_(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VCTSXS(OP_REG vd, OP_uIMM uimm5, OP_REG vb)
		{
			int nScale = 1 << uimm5;
			
			for (uint w = 0; w < 4; w++)
			{
				// C rounding = Round towards 0
				s64 result = (s64)(CPU.VPR[vb]._f[w] * nScale);

				if (result > INT_MAX)
					CPU.VPR[vd]._s32[w] = (int)INT_MAX;
				else if (result < INT_MIN)
					CPU.VPR[vd]._s32[w] = (int)INT_MIN;
				else
					CPU.VPR[vd]._s32[w] = (int)result;
			}
		}
		void VCTUXS(OP_REG vd, OP_uIMM uimm5, OP_REG vb)
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
		void VEXPTEFP(OP_REG vd, OP_REG vb)
		{
			// vd = exp(vb * log(2))
			// ISA : Note that the value placed into the element of vD may vary between implementations
			// and between different executions on the same implementation.
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._f[w] = exp(CPU.VPR[vb]._f[w] * log(2.0f));
			}
		}
		void VLOGEFP(OP_REG vd, OP_REG vb)
		{
			// ISA : Note that the value placed into the element of vD may vary between implementations
			// and between different executions on the same implementation.
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._f[w] = log(CPU.VPR[vb]._f[w]) / log(2.0f);
			}
		}
		void VMADDFP(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._f[w] = CPU.VPR[va]._f[w] * CPU.VPR[vc]._f[w] + CPU.VPR[vb]._f[w];
			}
		}
		void VMAXFP(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._f[w] = max(CPU.VPR[va]._f[w], CPU.VPR[vb]._f[w]);
			}
		}
		void VMAXSB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint b = 0; b < 16; b++)
				CPU.VPR[vd]._s8[b] = max(CPU.VPR[va]._s8[b], CPU.VPR[vb]._s8[b]);
		}
		void VMAXSH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint h = 0; h < 8; h++)
			{
				CPU.VPR[vd]._s16[h] = max(CPU.VPR[va]._s16[h], CPU.VPR[vb]._s16[h]);
			}
		}
		void VMAXSW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._s32[w] = max(CPU.VPR[va]._s32[w], CPU.VPR[vb]._s32[w]);
			}
		}
		void VMAXUB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint b = 0; b < 16; b++)
				CPU.VPR[vd]._u8[b] = max(CPU.VPR[va]._u8[b], CPU.VPR[vb]._u8[b]);
		}
		void VMAXUH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint h = 0; h < 8; h++)
			{
				CPU.VPR[vd]._u16[h] = max(CPU.VPR[va]._u16[h], CPU.VPR[vb]._u16[h]);
			}
		}
		void VMAXUW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._u32[w] = max(CPU.VPR[va]._u32[w], CPU.VPR[vb]._u32[w]);
			}
		}
		void VMHADDSHS(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc)
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
		void VMHRADDSHS(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc)
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
		void VMINFP(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._f[w] = min(CPU.VPR[va]._f[w], CPU.VPR[vb]._f[w]);
			}
		}
		void VMINSB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint b = 0; b < 16; b++)
			{
				CPU.VPR[vd]._s8[b] = min(CPU.VPR[va]._s8[b], CPU.VPR[vb]._s8[b]);
			}
		}
		void VMINSH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint h = 0; h < 8; h++)
			{
				CPU.VPR[vd]._s16[h] = min(CPU.VPR[va]._s16[h], CPU.VPR[vb]._s16[h]);
			}
		}
		void VMINSW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._s32[w] = min(CPU.VPR[va]._s32[w], CPU.VPR[vb]._s32[w]);
			}
		}
		void VMINUB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint b = 0; b < 16; b++)
			{
				CPU.VPR[vd]._u8[b] = min(CPU.VPR[va]._u8[b], CPU.VPR[vb]._u8[b]);
			}
		}
		void VMINUH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint h = 0; h < 8; h++)
			{
				CPU.VPR[vd]._u16[h] = min(CPU.VPR[va]._u16[h], CPU.VPR[vb]._u16[h]);
			}
		}
		void VMINUW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._u32[w] = min(CPU.VPR[va]._u32[w], CPU.VPR[vb]._u32[w]);
			}
		}
		void VMLADDUHM(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc)
		{
			for (uint h = 0; h < 8; h++)
			{
				CPU.VPR[vd]._u16[h] = CPU.VPR[va]._u16[h] * CPU.VPR[vb]._u16[h] + CPU.VPR[vc]._u16[h];
			}
		}
		void VMRGHB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint h = 0; h < 8; h++)
			{
				CPU.VPR[vd]._u8[h*2] = CPU.VPR[va]._u8[h];
				CPU.VPR[vd]._u8[h*2 + 1] = CPU.VPR[vb]._u8[h];
			}
		}
		void VMRGHH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._u16[w*2] = CPU.VPR[va]._u16[w];
				CPU.VPR[vd]._u16[w*2 + 1] = CPU.VPR[vb]._u16[w];
			}
		}
		void VMRGHW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint d = 0; d < 2; d++)
			{
				CPU.VPR[vd]._u32[d*2] = CPU.VPR[va]._u32[d];
				CPU.VPR[vd]._u32[d*2 + 1] = CPU.VPR[vb]._u32[d];
			}
		}
		void VMRGLB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint h = 0; h < 8; h++)
			{
				CPU.VPR[vd]._u8[h*2] = CPU.VPR[va]._u8[h + 8];
				CPU.VPR[vd]._u8[h*2 + 1] = CPU.VPR[vb]._u8[h + 8];
			}
		}
		void VMRGLH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._u16[w*2] = CPU.VPR[va]._u16[w + 4];
				CPU.VPR[vd]._u16[w*2 + 1] = CPU.VPR[vb]._u16[w + 4];
			}
		}
		void VMRGLW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint d = 0; d < 2; d++)
			{
				CPU.VPR[vd]._u32[d*2] = CPU.VPR[va]._u32[d + 2];
				CPU.VPR[vd]._u32[d*2 + 1] = CPU.VPR[vb]._u32[d + 2];
			}
		}
		void VMSUMMBM(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc)
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
		void VMSUMSHM(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc)
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
		void VMSUMSHS(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc)
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
		void VMSUMUBM(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc)
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
		void VMSUMUHM(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc)
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
		void VMSUMUHS(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc)
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
		void VMULESB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint h = 0; h < 8; h++)
			{
				CPU.VPR[vd]._s16[h] = (s16)CPU.VPR[va]._s8[h*2+1] * (s16)CPU.VPR[vb]._s8[h*2+1];
			}
		}
		void VMULESH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._s32[w] = (s32)CPU.VPR[va]._s16[w*2+1] * (s32)CPU.VPR[vb]._s16[w*2+1];
			}
		}
		void VMULEUB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint h = 0; h < 8; h++)
			{
				CPU.VPR[vd]._u16[h] = (u16)CPU.VPR[va]._u8[h*2+1] * (u16)CPU.VPR[vb]._u8[h*2+1];
			}
		}
		void VMULEUH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._u32[w] = (u32)CPU.VPR[va]._u16[w*2+1] * (u32)CPU.VPR[vb]._u16[w*2+1];
			}
		}
		void VMULOSB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint h = 0; h < 8; h++)
			{
				CPU.VPR[vd]._s16[h] = (s16)CPU.VPR[va]._s8[h*2] * (s16)CPU.VPR[vb]._s8[h*2];
			}
		}
		void VMULOSH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._s32[w] = (s32)CPU.VPR[va]._s16[w*2] * (s32)CPU.VPR[vb]._s16[w*2];
			}
		}
		void VMULOUB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint h = 0; h < 8; h++)
			{
				CPU.VPR[vd]._u16[h] = (u16)CPU.VPR[va]._u8[h*2] * (u16)CPU.VPR[vb]._u8[h*2];
			}
		}
		void VMULOUH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._u32[w] = (u32)CPU.VPR[va]._u16[w*2] * (u32)CPU.VPR[vb]._u16[w*2];
			}
		}
		void VNMSUBFP(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._f[w] = (double)CPU.VPR[vb]._f[w] - (double)CPU.VPR[va]._f[w] * (double)CPU.VPR[vc]._f[w];
			}
		}
		void VNOR(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._u32[w] = ~(CPU.VPR[va]._u32[w] | CPU.VPR[vb]._u32[w]);
			}
		}
		void VOR(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._u32[w] = CPU.VPR[va]._u32[w] | CPU.VPR[vb]._u32[w];
			}
		}
		void VPERM(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc)
		{
			for (uint b = 0; b < 16; b++)
			{
				u8 index = CPU.VPR[vc]._u8[b] & 0x1f;
				
				CPU.VPR[vd]._u8[b] = index < 0x10 ? CPU.VPR[va]._u8[0xf - index] : CPU.VPR[vb]._u8[0xf - (index - 0x10)];
			}
		}
		void VPKPX(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VPKSHSS(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VPKSHUS(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VPKSWSS(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VPKSWUS(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VPKUHUM(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint b = 0; b < 8; b++)
			{
				CPU.VPR[vd]._u8[b+8] = CPU.VPR[va]._u8[b*2];
				CPU.VPR[vd]._u8[b  ] = CPU.VPR[vb]._u8[b*2];
			}
		}
		void VPKUHUS(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VPKUWUM(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint h = 0; h < 4; h++)
			{
				CPU.VPR[vd]._u16[h+4] = CPU.VPR[va]._u16[h*2];
				CPU.VPR[vd]._u16[h  ] = CPU.VPR[vb]._u16[h*2];
			}
		}
		void VPKUWUS(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VREFP(OP_REG vd, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._f[w] = 1.0f / CPU.VPR[vb]._f[w];
			}
		}
		void VRFIM(OP_REG vd, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._f[w] = floor(CPU.VPR[vb]._f[w]);
			}
		}
		void VRFIN(OP_REG vd, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._f[w] = floor(CPU.VPR[vb]._f[w] + 0.5f);
			}
		}
		void VRFIP(OP_REG vd, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._f[w] = ceil(CPU.VPR[vb]._f[w]);
			}
		}
		void VRFIZ(OP_REG vd, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				float f;
				modf(CPU.VPR[vb]._f[w], &f);
				CPU.VPR[vd]._f[w] = f;
			}
		}
		void VRLB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint b = 0; b < 16; b++)
			{
				int nRot = CPU.VPR[vb]._u8[b] & 0x7;

				CPU.VPR[vd]._u8[b] = (CPU.VPR[va]._u8[b] << nRot) | (CPU.VPR[va]._u8[b] >> (8 - nRot));
			}
		}
		void VRLH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint h = 0; h < 8; h++)
			{
				CPU.VPR[vd]._u16[h] = rotl16(CPU.VPR[va]._u16[h], CPU.VPR[vb]._u8[h*2] & 0xf);
			}
		}
		void VRLW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._u32[w] = rotl32(CPU.VPR[va]._u32[w], CPU.VPR[vb]._u8[w*4] & 0x1f);
			}
		}
		void VRSQRTEFP(OP_REG vd, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				//TODO: accurate div
				CPU.VPR[vd]._f[w] = 1.0f / sqrtf(CPU.VPR[vb]._f[w]);
			}
		}
		void VSEL(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc)
		{
			for (uint b = 0; b < 16; b++)
			{
				CPU.VPR[vd]._u8[b] = (CPU.VPR[vb]._u8[b] & CPU.VPR[vc]._u8[b]) | (CPU.VPR[va]._u8[b] & (~CPU.VPR[vc]._u8[b]));
			}
		}
		void VSL(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VSLB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint b = 0; b < 16; b++)
			{
				CPU.VPR[vd]._u8[b] = CPU.VPR[va]._u8[b] << (CPU.VPR[vb]._u8[b] & 0x7);
			}
		}
		void VSLDOI(OP_REG vd, OP_REG va, OP_REG vb, OP_uIMM sh)
		{
			for (uint b = 0; b < 16 - sh; b++)
			{
				CPU.VPR[vd]._u8[15 - b] = CPU.VPR[va]._u8[15 - (b + sh)];
			}
			for (uint b = 16 - sh; b < 16; b++)
			{
				CPU.VPR[vd]._u8[15 - b] = CPU.VPR[vb]._u8[15 - (b - (16 - sh))];
			}
		}
		void VSLH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint h = 0; h < 8; h++)
			{
				CPU.VPR[vd]._u16[h] = CPU.VPR[va]._u16[h] << (CPU.VPR[vb]._u8[h*2] & 0xf);
			}
		}
		void VSLO(OP_REG vd, OP_REG va, OP_REG vb)
		{
			u8 nShift = (CPU.VPR[vb]._u8[0] >> 3) & 0xf;

			CPU.VPR[vd].Clear();

			for (u8 b = 0; b < 16 - nShift; b++)
			{
				CPU.VPR[vd]._u8[15 - b] = CPU.VPR[va]._u8[15 - (b + nShift)];
			}
		}
		void VSLW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._u32[w] = CPU.VPR[va]._u32[w] << (CPU.VPR[vb]._u8[w*4] & 0x1f);
			}
		}
		void VSPLTB(OP_REG vd, OP_uIMM uimm5, OP_REG vb)
		{
			u8 byte = CPU.VPR[vb]._u8[15 - uimm5];
			
			for (uint b = 0; b < 16; b++)
			{
				CPU.VPR[vd]._u8[b] = byte;
			}
		}
		void VSPLTH(OP_REG vd, OP_uIMM uimm5, OP_REG vb)
		{
			assert(uimm5 < 8);
			
			u16 hword = CPU.VPR[vb]._u16[7 - uimm5];

			for (uint h = 0; h < 8; h++)
			{
				CPU.VPR[vd]._u16[h] = hword;
			}
		}
		void VSPLTISB(OP_REG vd, OP_sIMM simm5)
		{
			for (uint b = 0; b < 16; b++)
			{
				CPU.VPR[vd]._u8[b] = simm5;
			}
		}
		void VSPLTISH(OP_REG vd, OP_sIMM simm5)
		{
			for (uint h = 0; h < 8; h++)
			{
				CPU.VPR[vd]._u16[h] = (s16)simm5;
			}
		}
		void VSPLTISW(OP_REG vd, OP_sIMM simm5)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._u32[w] = (s32)simm5;
			}
		}
		void VSPLTW(OP_REG vd, OP_uIMM uimm5, OP_REG vb)
		{
			assert(uimm5 < 4);

			u32 word = CPU.VPR[vb]._u32[uimm5];
			
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._u32[w] = word;
			}
		}
		void VSR(OP_REG vd, OP_REG va, OP_REG vb)
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

				for (uint b = 14; b >= 0; b--)
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
		void VSRAB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint b = 0; b < 16; b++)
			{
				CPU.VPR[vd]._s8[b] = CPU.VPR[va]._s8[b] >> (CPU.VPR[vb]._u8[b] & 0x7);
			}
		}
		void VSRAH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint h = 0; h < 8; h++)
			{
				CPU.VPR[vd]._s16[h] = CPU.VPR[va]._s16[h] >> (CPU.VPR[vb]._u8[h*2] & 0xf);
			}
		}
		void VSRAW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._s32[w] = CPU.VPR[va]._s32[w] >> (CPU.VPR[vb]._u8[w*4] & 0x1f);
			}
		}
		void VSRB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint b = 0; b < 16; b++)
			{
				CPU.VPR[vd]._u8[b] = CPU.VPR[va]._u8[b] >> (CPU.VPR[vb]._u8[b] & 0x7);
			}
		}
		void VSRH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint h = 0; h < 8; h++)
			{
				CPU.VPR[vd]._u16[h] = CPU.VPR[va]._u16[h] >> (CPU.VPR[vb]._u8[h*2] & 0xf);
			}
		}
		void VSRO(OP_REG vd, OP_REG va, OP_REG vb)
		{
			u8 nShift = (CPU.VPR[vb]._u8[0] >> 3) & 0xf;

			CPU.VPR[vd].Clear();

			for (u8 b = 0; b < 16 - nShift; b++)
			{
				CPU.VPR[vd]._u8[b] = CPU.VPR[va]._u8[b + nShift];
			}
		}
		void VSRW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._u32[w] = CPU.VPR[va]._u32[w] >> (CPU.VPR[vb]._u8[w*4] & 0x1f);
			}
		}
		void VSUBCUW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._u32[w] = CPU.VPR[va]._u32[w] < CPU.VPR[vb]._u32[w] ? 0 : 1;
			}
		}
		void VSUBFP(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._f[w] = CPU.VPR[va]._f[w] - CPU.VPR[vb]._f[w];
			}
		}
		void VSUBSBS(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VSUBSHS(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VSUBSWS(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VSUBUBM(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint b = 0; b < 16; b++)
			{
				CPU.VPR[vd]._u8[b] = (u8)((CPU.VPR[va]._u8[b] - CPU.VPR[vb]._u8[b]) & 0xff);
			}
		}
		void VSUBUBS(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VSUBUHM(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint h = 0; h < 8; h++)
			{
				CPU.VPR[vd]._u16[h] = CPU.VPR[va]._u16[h] - CPU.VPR[vb]._u16[h];
			}
		}
		void VSUBUHS(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VSUBUWM(OP_REG vd, OP_REG va, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._u32[w] = CPU.VPR[va]._u32[w] - CPU.VPR[vb]._u32[w];
			}
		}
		void VSUBUWS(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VSUMSWS(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VSUM2SWS(OP_REG vd, OP_REG va, OP_REG vb)
		{
			CPU.VPR[vd].Clear();

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
		}
		void VSUM4SBS(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VSUM4SHS(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VSUM4UBS(OP_REG vd, OP_REG va, OP_REG vb)
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
		void VUPKHPX(OP_REG vd, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._s8[(3 - w)*4 + 3] = CPU.VPR[vb]._s8[w*2 + 0] >> 7;  // signed shift sign extends
				CPU.VPR[vd]._u8[(3 - w)*4 + 2] = (CPU.VPR[vb]._u8[w*2 + 0] >> 2) & 0x1f;
				CPU.VPR[vd]._u8[(3 - w)*4 + 1] = ((CPU.VPR[vb]._u8[w*2 + 0] & 0x3) << 3) | ((CPU.VPR[vb]._u8[w*2 + 1] >> 5) & 0x7);
				CPU.VPR[vd]._u8[(3 - w)*4 + 0] = CPU.VPR[vb]._u8[w*2 + 1] & 0x1f;
			}
		}
		void VUPKHSB(OP_REG vd, OP_REG vb)
		{
			for (uint h = 0; h < 8; h++)
			{
				CPU.VPR[vd]._s16[h] = CPU.VPR[vb]._s8[h];
			}
		}
		void VUPKHSH(OP_REG vd, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._s32[w] = CPU.VPR[vb]._s16[w];
			}
		}
		void VUPKLPX(OP_REG vd, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._s8[(3 - w)*4 + 3] = CPU.VPR[vb]._s8[8 + w*2 + 0] >> 7;  // signed shift sign extends
				CPU.VPR[vd]._u8[(3 - w)*4 + 2] = (CPU.VPR[vb]._u8[8 + w*2 + 0] >> 2) & 0x1f;
				CPU.VPR[vd]._u8[(3 - w)*4 + 1] = ((CPU.VPR[vb]._u8[8 + w*2 + 0] & 0x3) << 3) | ((CPU.VPR[vb]._u8[8 + w*2 + 1] >> 5) & 0x7);
				CPU.VPR[vd]._u8[(3 - w)*4 + 0] = CPU.VPR[vb]._u8[8 + w*2 + 1] & 0x1f;
			}
		}
		void VUPKLSB(OP_REG vd, OP_REG vb)
		{
			for (uint h = 0; h < 8; h++)
			{
				CPU.VPR[vd]._s16[h] = CPU.VPR[vb]._s8[8 + h];
			}
		}
		void VUPKLSH(OP_REG vd, OP_REG vb)
		{
			for (uint w = 0; w < 4; w++)
			{
				CPU.VPR[vd]._s32[w] = CPU.VPR[vb]._s16[4 + w];
			}
		}
		void VXOR(OP_REG vd, OP_REG va, OP_REG vb)
		{
			CPU.VPR[vd]._u32[0] = CPU.VPR[va]._u32[0] ^ CPU.VPR[vb]._u32[0];
			CPU.VPR[vd]._u32[1] = CPU.VPR[va]._u32[1] ^ CPU.VPR[vb]._u32[1];
			CPU.VPR[vd]._u32[2] = CPU.VPR[va]._u32[2] ^ CPU.VPR[vb]._u32[2];
			CPU.VPR[vd]._u32[3] = CPU.VPR[va]._u32[3] ^ CPU.VPR[vb]._u32[3];
		}
	END_OPCODES_GROUP(G_04);

	void MULLI(OP_REG rd, OP_REG ra, OP_sIMM simm16)
	{
		CPU.GPR[rd] = (s64)CPU.GPR[ra] * simm16;
	}
	void SUBFIC(OP_REG rd, OP_REG ra, OP_sIMM simm16)
	{
		s64 RA = CPU.GPR[ra];
		CPU.GPR[rd] = (s64)simm16 - RA;
		CPU.XER.CA = RA <= simm16;
	}
	void CMPLI(OP_REG crfd, OP_REG l, OP_REG ra, OP_uIMM uimm16)
	{
		CPU.UpdateCRn<u64>(crfd, l ? CPU.GPR[ra] : (u32)CPU.GPR[ra], uimm16);
	}
	void CMPI(OP_REG crfd, OP_REG l, OP_REG ra, OP_sIMM simm16)
	{
		CPU.UpdateCRn<s64>(crfd, l ? CPU.GPR[ra] : (s32)CPU.GPR[ra], simm16);
	}
	void ADDIC(OP_REG rd, OP_REG ra, OP_sIMM simm16)
	{
		const u64 RA = CPU.GPR[ra];
		CPU.GPR[rd] = RA + simm16;
		CPU.XER.CA = CPU.IsCarry(RA, simm16);
	}
	void ADDIC_(OP_REG rd, OP_REG ra, OP_sIMM simm16)
	{
		const u64 RA = CPU.GPR[ra];
		CPU.GPR[rd] = RA + simm16;
		CPU.XER.CA = CPU.IsCarry(RA, simm16);
		CPU.UpdateCR0<s64>(CPU.GPR[rd]);
	}
	void ADDI(OP_REG rd, OP_REG ra, OP_sIMM simm16)
	{
		CPU.GPR[rd] = ra ? ((s64)CPU.GPR[ra] + simm16) : simm16;
	}
	void ADDIS(OP_REG rd, OP_REG ra, OP_sIMM simm16)
	{
		CPU.GPR[rd] = ra ? ((s64)CPU.GPR[ra] + (simm16 << 16)) : (simm16 << 16);
	}
	void BC(OP_REG bo, OP_REG bi, OP_sIMM bd, OP_REG aa, OP_REG lk)
	{
		if(!CheckCondition(bo, bi)) return;
		CPU.SetBranch(branchTarget((aa ? 0 : CPU.PC), bd));
		if(lk) CPU.LR = CPU.PC + 4;
	}
	void SC(OP_sIMM sc_code)
	{
		switch(sc_code)
		{
		case 0x1: UNK(wxString::Format("HyperCall %d", CPU.GPR[0])); break;
		case 0x2: SysCall(); break;
		case 0x22: UNK("HyperCall LV1"); break;
		default: UNK(wxString::Format("Unknown sc: %x", sc_code));
		}
	}
	void B(OP_sIMM ll, OP_REG aa, OP_REG lk)
	{
		CPU.SetBranch(branchTarget(aa ? 0 : CPU.PC, ll));
		if(lk) CPU.LR = CPU.PC + 4;
	}
	
	START_OPCODES_GROUP(G_13)
		void MCRF(OP_REG crfd, OP_REG crfs)
		{
			CPU.SetCR(crfd, CPU.GetCR(crfs));
		}
		void BCLR(OP_REG bo, OP_REG bi, OP_REG bh, OP_REG lk)
		{
			if(!CheckCondition(bo, bi)) return;
			CPU.SetBranch(branchTarget(0, CPU.LR));
			if(lk) CPU.LR = CPU.PC + 4;
		}
		void CRNOR(OP_REG bt, OP_REG ba, OP_REG bb)
		{
			const u8 v = 1 ^ (CPU.IsCR(ba) | CPU.IsCR(bb));
			CPU.SetCRBit2(bt, v & 0x1);
		}
		void CRANDC(OP_REG bt, OP_REG ba, OP_REG bb)
		{
			const u8 v = CPU.IsCR(ba) & (1 ^ CPU.IsCR(bb));
			CPU.SetCRBit2(bt, v & 0x1);
		}
		void ISYNC()
		{
		}
		void CRXOR(OP_REG bt, OP_REG ba, OP_REG bb)
		{
			const u8 v = CPU.IsCR(ba) ^ CPU.IsCR(bb);
			CPU.SetCRBit2(bt, v & 0x1);
		}
		void CRNAND(OP_REG bt, OP_REG ba, OP_REG bb)
		{
			const u8 v = 1 ^ (CPU.IsCR(ba) & CPU.IsCR(bb));
			CPU.SetCRBit2(bt, v & 0x1);
		}
		void CRAND(OP_REG bt, OP_REG ba, OP_REG bb)
		{
			const u8 v = CPU.IsCR(ba) & CPU.IsCR(bb);
			CPU.SetCRBit2(bt, v & 0x1);
		}
		void CREQV(OP_REG bt, OP_REG ba, OP_REG bb)
		{
			const u8 v = 1 ^ (CPU.IsCR(ba) ^ CPU.IsCR(bb));
			CPU.SetCRBit2(bt, v & 0x1);
		}
		void CRORC(OP_REG bt, OP_REG ba, OP_REG bb)
		{
			const u8 v = CPU.IsCR(ba) | (1 ^ CPU.IsCR(bb));
			CPU.SetCRBit2(bt, v & 0x1);
		}
		void CROR(OP_REG bt, OP_REG ba, OP_REG bb)
		{
			const u8 v = CPU.IsCR(ba) | CPU.IsCR(bb);
			CPU.SetCRBit2(bt, v & 0x1);
		}
		void BCCTR(OP_REG bo, OP_REG bi, OP_REG bh, OP_REG lk)
		{
			if(bo & 0x10 || CPU.IsCR(bi) == (bo & 0x8))
			{
				CPU.SetBranch(branchTarget(0, CPU.CTR));
				if(lk) CPU.LR = CPU.PC + 4;
			}
		}
	END_OPCODES_GROUP(G_13);
	
	void RLWIMI(OP_REG ra, OP_REG rs, OP_REG sh, OP_REG mb, OP_REG me, bool rc)
	{
		const u32 mask = rotate_mask[32 + mb][32 + me];
		CPU.GPR[ra] = (CPU.GPR[ra] & ~mask) | (rotl32(CPU.GPR[rs], sh) & mask);
		if(rc) CPU.UpdateCR0<s32>(CPU.GPR[ra]);
	}
	void RLWINM(OP_REG ra, OP_REG rs, OP_REG sh, OP_REG mb, OP_REG me, bool rc)
	{
		CPU.GPR[ra] = rotl32(CPU.GPR[rs], sh) & rotate_mask[32 + mb][32 + me];
		if(rc) CPU.UpdateCR0<s32>(CPU.GPR[ra]);
	}
	void RLWNM(OP_REG ra, OP_REG rs, OP_REG rb, OP_REG mb, OP_REG me, bool rc)
	{
		CPU.GPR[ra] = rotl32(CPU.GPR[rs], CPU.GPR[rb] & 0x1f) & rotate_mask[32 + mb][32 + me];
		if(rc) CPU.UpdateCR0<s32>(CPU.GPR[ra]);
	}
	void ORI(OP_REG ra, OP_REG rs, OP_uIMM uimm16)
	{
		CPU.GPR[ra] = CPU.GPR[rs] | uimm16;
	}
	void ORIS(OP_REG ra, OP_REG rs, OP_uIMM uimm16)
	{
		CPU.GPR[ra] = CPU.GPR[rs] | (uimm16 << 16);
	}
	void XORI(OP_REG ra, OP_REG rs, OP_uIMM uimm16)
	{
		CPU.GPR[ra] = CPU.GPR[rs] ^ uimm16;
	}
	void XORIS(OP_REG ra, OP_REG rs, OP_uIMM uimm16)
	{
		CPU.GPR[ra] = CPU.GPR[rs] ^ (uimm16 << 16);
	}
	void ANDI_(OP_REG ra, OP_REG rs, OP_uIMM uimm16)
	{
		CPU.GPR[ra] = CPU.GPR[rs] & uimm16;
		CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}
	void ANDIS_(OP_REG ra, OP_REG rs, OP_uIMM uimm16)
	{
		CPU.GPR[ra] = CPU.GPR[rs] & (uimm16 << 16);
		CPU.UpdateCR0<s64>(CPU.GPR[ra]);
	}

	START_OPCODES_GROUP(G_1e)
		void RLDICL(OP_REG ra, OP_REG rs, OP_REG sh, OP_REG mb, bool rc)
		{
			CPU.GPR[ra] = rotl64(CPU.GPR[rs], sh) & rotate_mask[mb][63];
			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
		}
		void RLDICR(OP_REG ra, OP_REG rs, OP_REG sh, OP_REG me, bool rc)
		{
			CPU.GPR[ra] = rotl64(CPU.GPR[rs], sh) & rotate_mask[0][me];
			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
		}
		void RLDIC(OP_REG ra, OP_REG rs, OP_REG sh, OP_REG mb, bool rc)
		{
			CPU.GPR[ra] = rotl64(CPU.GPR[rs], sh) & rotate_mask[mb][63-sh];
			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
		}
		void RLDIMI(OP_REG ra, OP_REG rs, OP_REG sh, OP_REG mb, bool rc)
		{
			const u64 mask = rotate_mask[mb][63-sh];
			CPU.GPR[ra] = (CPU.GPR[ra] & ~mask) | (rotl64(CPU.GPR[rs], sh) & mask);
			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
		}
	END_OPCODES_GROUP(G_1e);
	
	START_OPCODES_GROUP(G_1f)
		void CMP(OP_REG crfd, OP_REG l, OP_REG ra, OP_REG rb)
		{
			CPU.UpdateCRn<s64>(crfd, l ? CPU.GPR[ra] : (s32)CPU.GPR[ra],  l ? CPU.GPR[rb] : (s32)CPU.GPR[rb]);
		}
		void TW(OP_uIMM to, OP_REG ra, OP_REG rb)
		{
			s32 a = CPU.GPR[ra];
			s32 b = CPU.GPR[rb];

			if( (a < b  && (to & 0x10)) ||
				(a > b  && (to & 0x8))  ||
				(a == b && (to & 0x4))  ||
				((u32)a < (u32)b && (to & 0x2)) ||
				((u32)a > (u32)b && (to & 0x1)) )
			{
				UNK(wxString::Format("Trap! (tw %x, r%d, r%d)", to, ra, rb));
			}
		}
		void LVSL(OP_REG vd, OP_REG ra, OP_REG rb)
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
		void LVEBX(OP_REG vd, OP_REG ra, OP_REG rb)
		{
			//const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
			//CPU.VPR[vd].Clear();
			//CPU.VPR[vd]._u8[addr & 0xf] = Memory.Read8(addr);
			CPU.VPR[vd]._u128 = Memory.Read128((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~0xfULL);
		}
		void SUBFC(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			const s64 RA = CPU.GPR[ra];
			const s64 RB = CPU.GPR[rb];
			CPU.GPR[rd] = RB - RA;
			CPU.XER.CA = CPU.IsCarry(RA, RB);
			if(oe) UNK("subfco");
			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
		}
		void ADDC(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			const s64 RA = CPU.GPR[ra];
			const s64 RB = CPU.GPR[rb];
			CPU.GPR[rd] = RA + RB;
			CPU.XER.CA = RA <= RB;
			if(oe) UNK("addco");
			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
		}
		void MULHDU(OP_REG rd, OP_REG ra, OP_REG rb, bool rc)
		{
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

			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
		}
		void MULHWU(OP_REG rd, OP_REG ra, OP_REG rb, bool rc)
		{
			CPU.GPR[rd] = (CPU.GPR[ra] * CPU.GPR[rb]) >> 32;
			if(rc) CPU.UpdateCR0<s32>(CPU.GPR[rd]);
		}
		void MFOCRF(OP_uIMM a, OP_REG rd, OP_uIMM crm)
		{
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
					CPU.GPR[rd] = (u64)CPU.GetCR(n) << (n * 4);
				}
				else
					CPU.GPR[rd] = 0;
			}
			else
			{
				CPU.GPR[rd] = CPU.CR.CR;
			}
		}
		void LWARX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
			CPU.reserve_addr = addr;
			CPU.reserve = true;
			CPU.GPR[rd] = Memory.Read32(addr);
		}
		void LDX(OP_REG ra, OP_REG rs, OP_REG rb)
		{
			CPU.GPR[ra] = Memory.Read64(rs ? CPU.GPR[rs] + CPU.GPR[rb] : CPU.GPR[rb]);
		}
		void LWZX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			CPU.GPR[rd] = Memory.Read32(ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]);
		}
		void SLW(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			const u32 n = CPU.GPR[rb] & 0x1f;
			const u32 r = rotl32((u32)CPU.GPR[rs], n);
			const u32 m = (CPU.GPR[rb] & 0x20) ? 0 : rotate_mask[32][63 - n];

			CPU.GPR[ra] = r & m;

			if(rc) CPU.UpdateCR0<s32>(CPU.GPR[ra]);
		}
		void CNTLZW(OP_REG ra, OP_REG rs, bool rc)
		{
			u32 i;
			for(i=0; i < 32; i++)
			{
				if(CPU.GPR[rs] & (0x80000000 >> i)) break;
			}

			CPU.GPR[ra] = i;

			if(rc) CPU.UpdateCR0<u32>(CPU.GPR[ra]);
		}
		void SLD(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			CPU.GPR[ra] = CPU.GPR[rb] & 0x40 ? 0 : CPU.GPR[rs] << (CPU.GPR[rb] & 0x3f);
			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
		}
		void AND(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			CPU.GPR[ra] = CPU.GPR[rs] & CPU.GPR[rb];
			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
		}
		void CMPL(OP_REG crfd, OP_REG l, OP_REG ra, OP_REG rb)
		{
			CPU.UpdateCRn<u64>(crfd, l ? CPU.GPR[ra] : (u32)CPU.GPR[ra], l ? CPU.GPR[rb] : (u32)CPU.GPR[rb]);
		}
		void LVSR(OP_REG vd, OP_REG ra, OP_REG rb)
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
		void LVEHX(OP_REG vd, OP_REG ra, OP_REG rb)
		{
			//const u64 addr = (ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~1ULL;
			//CPU.VPR[vd].Clear();
			//(u16&)CPU.VPR[vd]._u8[addr & 0xf] = Memory.Read16(addr);
			CPU.VPR[vd]._u128 = Memory.Read128((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~0xfULL);
		}
		void SUBF(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			CPU.GPR[rd] = CPU.GPR[rb] - CPU.GPR[ra];
			if(oe) UNK("subfo");
			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
		}
		void LDUX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			const u64 addr = CPU.GPR[ra] + CPU.GPR[rb];
			CPU.GPR[rd] = Memory.Read64(addr);
			CPU.GPR[ra] = addr;
		}
		void DCBST(OP_REG ra, OP_REG rb)
		{
			//UNK("dcbst", false);
		}
		void CNTLZD(OP_REG ra, OP_REG rs, bool rc)
		{
			u32 i = 0;
			
			for(u64 mask = 1ULL << 63; i < 64; i++, mask >>= 1)
			{
				if(CPU.GPR[rs] & mask) break;
			}

			CPU.GPR[ra] = i;
			if(rc) CPU.UpdateCR0<u64>(CPU.GPR[ra]);
		}
		void ANDC(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			CPU.GPR[ra] = CPU.GPR[rs] & ~CPU.GPR[rb];
			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
		}
		void LVEWX(OP_REG vd, OP_REG ra, OP_REG rb)
		{
			//const u64 addr = (ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~3ULL;
			//CPU.VPR[vd].Clear();
			//(u32&)CPU.VPR[vd]._u8[addr & 0xf] = Memory.Read32(addr);
			CPU.VPR[vd]._u128 = Memory.Read128((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~0xfULL);
		}
		void MULHD(OP_REG rd, OP_REG ra, OP_REG rb, bool rc)
		{
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

			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
		}
		void MULHW(OP_REG rd, OP_REG ra, OP_REG rb, bool rc)
		{
			CPU.GPR[rd] = (s64)(s32)((CPU.GPR[ra] * CPU.GPR[rb]) >> 32);
			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
		}
		void LDARX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
			CPU.reserve_addr = addr;
			CPU.reserve = true;
			CPU.GPR[rd] = Memory.Read64(addr);
		}
		void DCBF(OP_REG ra, OP_REG rb)
		{
			//UNK("dcbf", false);
		}
		void LBZX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			CPU.GPR[rd] = Memory.Read8(ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]);
		}
		void LVX(OP_REG vd, OP_REG ra, OP_REG rb)
		{
			CPU.VPR[vd]._u128 = Memory.Read128((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~0xfULL);
		}
		void NEG(OP_REG rd, OP_REG ra, OP_REG oe, bool rc)
		{
			CPU.GPR[rd] = 0-CPU.GPR[ra];
			if(oe) UNK("nego");
			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
		}
		void LBZUX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			//if(ra == 0 || ra == rd) throw "Bad instruction [LBZUX]";

			const u64 addr = CPU.GPR[ra] + CPU.GPR[rb];
			CPU.GPR[rd] = Memory.Read8(addr);
			CPU.GPR[ra] = addr;
		}
		void NOR(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			CPU.GPR[ra] = ~(CPU.GPR[rs] | CPU.GPR[rb]);
			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
		}
		void STVEBX(OP_REG vs, OP_REG ra, OP_REG rb)
		{
			const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
			const u8 eb = addr & 0xf;
			Memory.Write8(addr, CPU.VPR[vs]._u8[15 - eb]);
		}
		void SUBFE(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			const s64 RA = CPU.GPR[ra];
			const s64 RB = CPU.GPR[rb];
			CPU.GPR[ra] = ~RA + RB + CPU.XER.CA;
			CPU.XER.CA = ((u64)~RA + CPU.XER.CA > ~(u64)RB) | ((RA == 0) & CPU.XER.CA);
			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
			if(oe) UNK("subfeo");
		}
		void ADDE(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			const s64 RA = CPU.GPR[ra];
			const s64 RB = CPU.GPR[rb];
			CPU.GPR[rd] = RA + RB + CPU.XER.CA;
			CPU.XER.CA = ((u64)RA + CPU.XER.CA > ~(u64)RB) | ((RA == -1) & CPU.XER.CA);
			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
			if(oe) UNK("addeo");
		}
		void MTOCRF(OP_REG crm, OP_REG rs)
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
				CPU.SetCR(n, (CPU.GPR[rs] >> (4*n)) & 0xf);
			}
			else CPU.CR.CR = 0;
		}
		void STDX(OP_REG rs, OP_REG ra, OP_REG rb)
		{
			Memory.Write64((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]), CPU.GPR[rs]);
		}
		void STWCX_(OP_REG rs, OP_REG ra, OP_REG rb)
		{
			CPU.SetCR(0, CPU.XER.SO ? CR_SO : 0);
			if(!CPU.reserve) return;

			const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];

			if(addr == CPU.reserve_addr)
			{
				Memory.Write32(addr, CPU.GPR[rs]);
				CPU.SetCR_EQ(0, true);
			}
			else
			{
				static const bool u = 0;
				if(u) Memory.Write32(addr, CPU.GPR[rs]);
				CPU.SetCR_EQ(0, u);
				CPU.reserve = false;
			}
		}
		void STWX(OP_REG rs, OP_REG ra, OP_REG rb)
		{
			Memory.Write32((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]), CPU.GPR[rs]);
		}
		void STVEHX(OP_REG vs, OP_REG ra, OP_REG rb)
		{
			const u64 addr = (ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~1ULL;
			const u8 eb = (addr & 0xf) >> 1;
			Memory.Write16(addr, CPU.VPR[vs]._u16[7 - eb]);
		}
		void STDUX(OP_REG rs, OP_REG ra, OP_REG rb)
		{
			const u64 addr = CPU.GPR[ra] + CPU.GPR[rb];
			Memory.Write64(addr, CPU.GPR[rs]);
			CPU.GPR[ra] = addr;
		}
		void STVEWX(OP_REG vs, OP_REG ra, OP_REG rb)
		{
			const u64 addr = (ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~3ULL;
			const u8 eb = (addr & 0xf) >> 2;
			Memory.Write32(addr, CPU.VPR[vs]._u32[3 - eb]);
		}
		void ADDZE(OP_REG rd, OP_REG ra, OP_REG oe, bool rc)
		{
			const s64 RA = CPU.GPR[ra];
			CPU.GPR[rd] = RA + CPU.XER.CA;

			CPU.XER.CA = CPU.IsCarry(RA, CPU.XER.CA);
			if(oe) ConLog.Warning("addzeo");
			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
		}
		void STDCX_(OP_REG rs, OP_REG ra, OP_REG rb)
		{
			CPU.SetCR(0, CPU.XER.SO ? CR_SO : 0);
			if(!CPU.reserve) return;

			const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];

			if(addr == CPU.reserve_addr)
			{
				Memory.Write64(addr, CPU.GPR[rs]);
				CPU.SetCR_EQ(0, true);
			}
			else
			{
				static const bool u = 0;
				if(u) Memory.Write64(addr, CPU.GPR[rs]);
				CPU.SetCR_EQ(0, u);
				CPU.reserve = false;
			}
		}
		void STBX(OP_REG rs, OP_REG ra, OP_REG rb)
		{
			Memory.Write8((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]), CPU.GPR[rs]);
		}
		void STVX(OP_REG vs, OP_REG ra, OP_REG rb)
		{
			Memory.Write128((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~0xfULL, CPU.VPR[vs]._u128);
		}
		void MULLD(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			CPU.GPR[rd] = CPU.GPR[ra] * CPU.GPR[rb];
			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
			if(oe) UNK("mulldo");
		}
		void ADDME(OP_REG rd, OP_REG ra, OP_REG oe, bool rc)
		{
			const s64 RA = CPU.GPR[ra];
			CPU.GPR[rd] = RA + CPU.XER.CA - 1;
			CPU.XER.CA |= RA != 0;

			if(oe) UNK("addmeo");
			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
		}
		void MULLW(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			CPU.GPR[rd] = (s64)(s32)((s32)CPU.GPR[ra] * (s32)CPU.GPR[rb]);
			if(rc) CPU.UpdateCR0<s32>(CPU.GPR[rd]);
			if(oe) UNK("mullwo");
		}
		void DCBTST(OP_REG th, OP_REG ra, OP_REG rb)
		{
			//UNK("dcbtst", false);
		}
		void ADD(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			const u64 RA = CPU.GPR[ra];
			const u64 RB = CPU.GPR[rb];
			CPU.GPR[rd] = RA + RB;
			CPU.XER.CA = CPU.IsCarry(RA, RB);
			if(oe) UNK("addo");
			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
		}
		void DCBT(OP_REG ra, OP_REG rb, OP_REG th)
		{
			//UNK("dcbt", false);
		}
		void LHZX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			CPU.GPR[rd] = Memory.Read16(ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]);
		}
		void EQV(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			CPU.GPR[ra] = ~(CPU.GPR[rs] ^ CPU.GPR[rb]);
			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
		}
		void ECIWX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			//HACK!
			CPU.GPR[rd] = Memory.Read32(ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]);
		}
		void LHZUX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
			CPU.GPR[rd] = Memory.Read16(addr);
			CPU.GPR[ra] = addr;
		}
		void XOR(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			CPU.GPR[ra] = CPU.GPR[rs] ^ CPU.GPR[rb];
			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
		}
		void MFSPR(OP_REG rd, OP_REG spr)
		{
			CPU.GPR[rd] = GetRegBySPR(spr);
		}
		void DST(OP_REG ra, OP_REG rb, OP_uIMM strm, OP_uIMM t)
		{
		}
		void LHAX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			CPU.GPR[rd] = (s64)(s16)Memory.Read16(ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]);
		}
		void LVXL(OP_REG vd, OP_REG ra, OP_REG rb)
		{
			CPU.VPR[vd]._u128 = Memory.Read128((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~0xfULL);
		}
		void ABS(OP_REG rd, OP_REG ra, OP_REG oe, bool rc)
		{
			CPU.GPR[rd] = abs((s64)CPU.GPR[ra]);
			if(oe) UNK("abso");
			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
		}
		void MFTB(OP_REG rd, OP_REG spr)
		{
			const u32 n = (spr >> 5) | ((spr & 0x1f) << 5);

			switch(n)
			{
			case 0x10C: CPU.GPR[rd] = CPU.TB; break;
			case 0x10D: CPU.GPR[rd] = CPU.TBH; break;
			default: UNK(wxString::Format("mftb r%d, %d", rd, spr)); break;
			}
		}
		void DSTST(OP_REG ra, OP_REG rb, OP_uIMM strm, OP_uIMM t)
		{
		}
		void LHAUX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
			CPU.GPR[rd] = (s64)(s16)Memory.Read16(addr);
			CPU.GPR[ra] = addr;
		}
		void STHX(OP_REG rs, OP_REG ra, OP_REG rb)
		{
			const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
			Memory.Write16(addr, CPU.GPR[rs]);
		}
		void ORC(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			CPU.GPR[ra] = CPU.GPR[rs] | ~CPU.GPR[rb];
			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
		}
		void ECOWX(OP_REG rs, OP_REG ra, OP_REG rb)
		{
			//HACK!
			Memory.Write32((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]), CPU.GPR[rs]);
		}
		void OR(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			CPU.GPR[ra] = CPU.GPR[rs] | CPU.GPR[rb];
			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
		}
		void DIVDU(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
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
		void DIVWU(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
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
		void MTSPR(OP_REG spr, OP_REG rs)
		{
			GetRegBySPR(spr) = CPU.GPR[rs];
		}
		/*0x1d6*///DCBI
		void NAND(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			CPU.GPR[ra] = ~(CPU.GPR[rs] & CPU.GPR[rb]);

			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
		}
		void STVXL(OP_REG vs, OP_REG ra, OP_REG rb)
		{
			Memory.Write128((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]) & ~0xfULL, CPU.VPR[vs]._u128);
		}
		void DIVD(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			const s64 RA = CPU.GPR[ra];
			const s64 RB = CPU.GPR[rb];

			if (RB == 0 || ((u64)RA == 0x8000000000000000 && RB == -1))
			{
				if(oe) UNK("divdo");
				CPU.GPR[rd] = (((u64)RA & 0x8000000000000000) && RB == 0) ? -1 : 0;
			}
			else
			{
				CPU.GPR[rd] = RA / RB;
			}

			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[rd]);
		}
		void DIVW(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			const s32 RA = CPU.GPR[ra];
			const s32 RB = CPU.GPR[rb];

			if (RB == 0 || ((u32)RA == 0x80000000 && RB == -1))
			{
				if(oe) UNK("divwo");
				CPU.GPR[rd] = (((u32)RA & 0x80000000) && RB == 0) ? -1 : 0;
			}
			else
			{
				CPU.GPR[rd] = (s64)(RA / RB);
			}

			if(rc) CPU.UpdateCR0<s32>(CPU.GPR[rd]);
		}
		void LVLX(OP_REG vd, OP_REG ra, OP_REG rb)
		{
			const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
			const u8 eb = addr & 0xf;

			Memory.ReadLeft(CPU.VPR[vd]._u8 + eb, addr, 16 - eb);
		}
		void LWBRX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			CPU.GPR[rd] = (u32&)Memory[ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]];
		}
		void LFSX(OP_REG frd, OP_REG ra, OP_REG rb)
		{
			(u32&)CPU.FPR[frd] = Memory.Read32(ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]);
			CPU.FPR[frd] = (float&)CPU.FPR[frd];
		}
		void SRW(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			CPU.GPR[ra] = CPU.GPR[rb] & 0x20 ? 0 : (u32)CPU.GPR[rs] >> (CPU.GPR[rb] & 0x1f);
			if(rc) CPU.UpdateCR0<s32>(CPU.GPR[ra]);
		}
		void SRD(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			CPU.GPR[ra] = CPU.GPR[rb] & 0x40 ? 0 : CPU.GPR[rs] >> (CPU.GPR[rb] & 0x3f);
			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
		}
		void LVRX(OP_REG vd, OP_REG ra, OP_REG rb)
		{
			const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
			const u8 eb = addr & 0xf;

			Memory.ReadRight(CPU.VPR[vd]._u8, addr & ~0xf, eb);
		}
		void LFSUX(OP_REG frd, OP_REG ra, OP_REG rb)
		{
			const u64 addr = CPU.GPR[ra] + CPU.GPR[rb];
			(u64&)CPU.FPR[frd] = Memory.Read32(addr);
			CPU.FPR[frd] = (float&)CPU.FPR[frd];
			CPU.GPR[ra] = addr;
		}
		void SYNC(OP_uIMM l)
		{
		}
		void LFDX(OP_REG frd, OP_REG ra, OP_REG rb)
		{
			(u64&)CPU.FPR[frd] = Memory.Read64(ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]);
		}
		void LFDUX(OP_REG frd, OP_REG ra, OP_REG rb)
		{
			const u64 addr = CPU.GPR[ra] + CPU.GPR[rb];
			(u64&)CPU.FPR[frd] = Memory.Read64(addr);
			CPU.GPR[ra] = addr;
		}
		void STVLX(OP_REG vs, OP_REG ra, OP_REG rb)
		{
			const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
			const u8 eb = addr & 0xf;

			Memory.WriteLeft(addr, 16 - eb, CPU.VPR[vs]._u8 + eb);
		}
		void STFSX(OP_REG frs, OP_REG ra, OP_REG rb)
		{
			Memory.Write32((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]), PPCdouble(CPU.FPR[frs]).To32());
		}
		void STVRX(OP_REG vs, OP_REG ra, OP_REG rb)
		{
			const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
			const u8 eb = addr & 0xf;

			Memory.WriteRight(addr - eb, eb, CPU.VPR[vs]._u8);
		}
		void STFDX(OP_REG frs, OP_REG ra, OP_REG rb)
		{
			Memory.Write64((ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]), (u64&)CPU.FPR[frs]);
		}
		void LVLXL(OP_REG vd, OP_REG ra, OP_REG rb)
		{
			const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
			const u8 eb = addr & 0xf;

			Memory.ReadLeft(CPU.VPR[vd]._u8 + eb, addr, 16 - eb);
		}
		void LHBRX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			CPU.GPR[rd] = (u16&)Memory[ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb]];
		}
		void SRAW(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{    
			s32 RS = CPU.GPR[rs];
			s32 RB = CPU.GPR[rb];
			CPU.GPR[ra] = RS >> RB;
			CPU.XER.CA = (RS < 0) & ((CPU.GPR[ra] << RB) != RS);

			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
		}
		void SRAD(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			s64 RS = CPU.GPR[rs];
			s64 RB = CPU.GPR[rb];
			CPU.GPR[ra] = RS >> RB;
			CPU.XER.CA = (RS < 0) & ((CPU.GPR[ra] << RB) != RS);

			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
		}
		void LVRXL(OP_REG vd, OP_REG ra, OP_REG rb)
		{
			const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
			const u8 eb = addr & 0xf;

			Memory.ReadRight(CPU.VPR[vd]._u8, addr & ~0xf, eb);
		}
		void DSS(OP_uIMM strm, OP_uIMM a)
		{
		}
		void SRAWI(OP_REG ra, OP_REG rs, OP_REG sh, bool rc)
		{
			s32 RS = CPU.GPR[rs];
			CPU.GPR[ra] = RS >> sh;
			CPU.XER.CA = (RS < 0) & ((CPU.GPR[ra] << sh) != RS);

			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
		}
		void SRADI1(OP_REG ra, OP_REG rs, OP_REG sh, bool rc)
		{
			s64 RS = CPU.GPR[rs];
			CPU.GPR[ra] = RS >> sh;
			CPU.XER.CA = (RS < 0) & ((CPU.GPR[ra] << sh) != RS);

			if(rc) CPU.UpdateCR0<s64>(CPU.GPR[ra]);
		}
		void SRADI2(OP_REG ra, OP_REG rs, OP_REG sh, bool rc)
		{
			SRADI1(ra, rs, sh, rc);
		}
		void EIEIO()
		{
		}
		void STVLXL(OP_REG vs, OP_REG ra, OP_REG rb)
		{
			const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
			const u8 eb = addr & 0xf;

			Memory.WriteLeft(addr, 16 - eb, CPU.VPR[vs]._u8 + eb);
		}
		void EXTSH(OP_REG ra, OP_REG rs, bool rc)
		{
			CPU.GPR[ra] = (s64)(s16)CPU.GPR[rs];
			if(rc) CPU.UpdateCR0<s16>(CPU.GPR[ra]);
		}
		void STVRXL(OP_REG vs, OP_REG ra, OP_REG rb)
		{
			const u64 addr = ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb];
			const u8 eb = addr & 0xf;

			Memory.WriteRight(addr - eb, eb, CPU.VPR[vs]._u8);
		}
		void EXTSB(OP_REG ra, OP_REG rs, bool rc)
		{
			CPU.GPR[ra] = (s64)(s8)CPU.GPR[rs];
			if(rc) CPU.UpdateCR0<s8>(CPU.GPR[ra]);
		}
		void STFIWX(OP_REG frs, OP_REG ra, OP_REG rb)
		{
			Memory.Write32(ra ? CPU.GPR[ra] + CPU.GPR[rb] : CPU.GPR[rb], (u32&)CPU.FPR[frs]);
		}
		void EXTSW(OP_REG ra, OP_REG rs, bool rc)
		{
			CPU.GPR[ra] = (s64)(s32)CPU.GPR[rs];
			if(rc) CPU.UpdateCR0<s32>(CPU.GPR[ra]);
		}
		/*0x3d6*///ICBI
		void DCBZ(OP_REG ra, OP_REG rs)
		{
			//UNK("dcbz", false);
		}
	END_OPCODES_GROUP(G_1f);

	void LWZ(OP_REG rd, OP_REG ra, OP_sIMM d)
	{
		CPU.GPR[rd] = Memory.Read32(ra ? CPU.GPR[ra] + d : d);
	}
	void LWZU(OP_REG rd, OP_REG ra, OP_sIMM d)
	{
		const u64 addr = CPU.GPR[ra] + d;
		CPU.GPR[rd] = Memory.Read32(addr);
		CPU.GPR[ra] = addr;
	}
	void LBZ(OP_REG rd, OP_REG ra, OP_sIMM d)
	{
		CPU.GPR[rd] = Memory.Read8(ra ? CPU.GPR[ra] + d : d);
	}
	void LBZU(OP_REG rd, OP_REG ra, OP_sIMM d)
	{
		const u64 addr = CPU.GPR[ra] + d;
		CPU.GPR[rd] = Memory.Read8(addr);
		CPU.GPR[ra] = addr;
	}
	void STW(OP_REG rs, OP_REG ra, OP_sIMM d)
	{
		Memory.Write32(ra ? CPU.GPR[ra] + d : d, CPU.GPR[rs]);
	}
	void STWU(OP_REG rs, OP_REG ra, OP_sIMM d)
	{
		const u64 addr = CPU.GPR[ra] + d;
		Memory.Write32(addr, CPU.GPR[rs]);
		CPU.GPR[ra] = addr;
	}
	void STB(OP_REG rs, OP_REG ra, OP_sIMM d)
	{
		Memory.Write8(ra ? CPU.GPR[ra] + d : d, CPU.GPR[rs]);
	}
	void STBU(OP_REG rs, OP_REG ra, OP_sIMM d)
	{
		const u64 addr = CPU.GPR[ra] + d;
		Memory.Write8(addr, CPU.GPR[rs]);
		CPU.GPR[ra] = addr;
	}
	void LHZ(OP_REG rd, OP_REG ra, OP_sIMM d)
	{
		CPU.GPR[rd] = Memory.Read16(ra ? CPU.GPR[ra] + d : d);
	}
	void LHZU(OP_REG rd, OP_REG ra, OP_sIMM d)
	{
		const u64 addr = CPU.GPR[ra] + d;
		CPU.GPR[rd] = Memory.Read16(addr);
		CPU.GPR[ra] = addr;
	}
	void STH(OP_REG rs, OP_REG ra, OP_sIMM d)
	{
		Memory.Write16(ra ? CPU.GPR[ra] + d : d, CPU.GPR[rs]);
	}
	void STHU(OP_REG rs, OP_REG ra, OP_sIMM d)
	{
		const u64 addr = CPU.GPR[ra] + d;
		Memory.Write16(addr, CPU.GPR[rs]);
		CPU.GPR[ra] = addr;
	}
	void LMW(OP_REG rd, OP_REG ra, OP_sIMM d)
	{
		u64 addr = ra ? CPU.GPR[ra] + d : d;
		for(u32 i=rd; i<32; ++i, addr += 4)
		{
			CPU.GPR[i] = Memory.Read32(addr);
		}
	}
	void STMW(OP_REG rs, OP_REG ra, OP_sIMM d)
	{
		u64 addr = ra ? CPU.GPR[ra] + d : d;
		for(u32 i=rs; i<32; ++i, addr += 4)
		{
			Memory.Write32(addr, CPU.GPR[i]);
		}
	}
	void LFS(OP_REG frd, OP_REG ra, OP_sIMM d)
	{
		const u32 v = Memory.Read32(ra ? CPU.GPR[ra] + d : d);
		CPU.FPR[frd] = (float&)v;
	}
	void LFSU(OP_REG frd, OP_REG ra, OP_sIMM ds)
	{
		const u64 addr = CPU.GPR[ra] + ds;
		const u32 v = Memory.Read32(addr);
		CPU.FPR[frd] = (float&)v;
		CPU.GPR[ra] = addr;
	}
	void LFD(OP_REG frd, OP_REG ra, OP_sIMM d)
	{
		(u64&)CPU.FPR[frd] = Memory.Read64(ra ? CPU.GPR[ra] + d : d);
	}
	void LFDU(OP_REG frd, OP_REG ra, OP_sIMM ds)
	{
		const u64 addr = CPU.GPR[ra] + ds;
		(u64&)CPU.FPR[frd] = Memory.Read64(addr);
		CPU.GPR[ra] = addr;
	}
	void STFS(OP_REG frs, OP_REG ra, OP_sIMM d)
	{
		Memory.Write32(ra ? CPU.GPR[ra] + d : d, PPCdouble(CPU.FPR[frs]).To32());
	}
	void STFSU(OP_REG frs, OP_REG ra, OP_sIMM d)
	{
		const u64 addr = CPU.GPR[ra] + d;
		Memory.Write32(addr, PPCdouble(CPU.FPR[frs]).To32());
		CPU.GPR[ra] = addr;
	}
	void STFD(OP_REG frs, OP_REG ra, OP_sIMM d)
	{
		Memory.Write64(ra ? CPU.GPR[ra] + d : d, (u64&)CPU.FPR[frs]);
	}
	void STFDU(OP_REG frs, OP_REG ra, OP_sIMM d)
	{
		const u64 addr = CPU.GPR[ra] + d;
		Memory.Write64(addr, (u64&)CPU.FPR[frs]);
		CPU.GPR[ra] = addr;
	}
	
	START_OPCODES_GROUP(G_3a)
		void LD(OP_REG rd, OP_REG ra, OP_sIMM ds)
		{
			CPU.GPR[rd] = Memory.Read64(ra ? CPU.GPR[ra] + ds : ds);
		}
		void LDU(OP_REG rd, OP_REG ra, OP_sIMM ds)
		{
			//if(ra == 0 || rt == ra) return;
			const u64 addr = CPU.GPR[ra] + ds;
			CPU.GPR[rd] = Memory.Read64(addr);
			CPU.GPR[ra] = addr;
		}
	END_OPCODES_GROUP(G_3a);

	START_OPCODES_GROUP(G_3b)
		void FDIVS(OP_REG frd, OP_REG fra, OP_REG frb, bool rc)
		{
			if(CPU.FPR[frb] == 0.0) CPU.SetFPSCRException(FPSCR_ZX);
			CPU.FPR[frd] = static_cast<float>(CPU.FPR[fra] / CPU.FPR[frb]);
			CPU.FPSCR.FPRF = PPCdouble(CPU.FPR[frd]).GetType();
			if(rc) UNK("fdivs.");//CPU.UpdateCR1(CPU.FPR[frd]);
		}
		void FSUBS(OP_REG frd, OP_REG fra, OP_REG frb, bool rc)
		{
			CPU.FPR[frd] = static_cast<float>(CPU.FPR[fra] - CPU.FPR[frb]);
			CPU.FPSCR.FPRF = PPCdouble(CPU.FPR[frd]).GetType();
			if(rc) UNK("fsubs.");//CPU.UpdateCR1(CPU.FPR[frd]);
		}
		void FADDS(OP_REG frd, OP_REG fra, OP_REG frb, bool rc)
		{
			CPU.FPR[frd] = static_cast<float>(CPU.FPR[fra] + CPU.FPR[frb]);
			CPU.FPSCR.FPRF = PPCdouble(CPU.FPR[frd]).GetType();
			if(rc) UNK("fadds.");//CPU.UpdateCR1(CPU.FPR[frd]);
		}
		void FSQRTS(OP_REG frd, OP_REG frb, bool rc)
		{
			CPU.FPR[frd] = static_cast<float>(sqrt((float)CPU.FPR[frb]));
			if(rc) UNK("fsqrts.");//CPU.UpdateCR1(CPU.FPR[frd]);
		}
		void FRES(OP_REG frd, OP_REG frb, bool rc)
		{
			if(CPU.FPR[frb] == 0.0) CPU.SetFPSCRException(FPSCR_ZX);
			CPU.FPR[frd] = static_cast<float>(1.0f/CPU.FPR[frb]);
			if(rc) UNK("fres.");//CPU.UpdateCR1(CPU.FPR[frd]);
		}
		void FMULS(OP_REG frd, OP_REG fra, OP_REG frc, bool rc)
		{
			CPU.FPR[frd] = static_cast<float>(CPU.FPR[fra] * CPU.FPR[frc]);
			CPU.FPSCR.FI = 0;
			CPU.FPSCR.FR = 0;
			CPU.FPSCR.FPRF = PPCdouble(CPU.FPR[frd]).GetType();
			if(rc) UNK("fmuls.");//CPU.UpdateCR1(CPU.FPR[frd]);
		}
		void FMADDS(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc)
		{
			CPU.FPR[frd] = static_cast<float>(CPU.FPR[fra] * CPU.FPR[frc] + CPU.FPR[frb]);
			CPU.FPSCR.FPRF = PPCdouble(CPU.FPR[frd]).GetType();
			if(rc) UNK("fmadds.");//CPU.UpdateCR1(CPU.FPR[frd]);
		}
		void FMSUBS(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc)
		{
			CPU.FPR[frd] = static_cast<float>(CPU.FPR[fra] * CPU.FPR[frc] - CPU.FPR[frb]);
			CPU.FPSCR.FPRF = PPCdouble(CPU.FPR[frd]).GetType();
			if(rc) UNK("fmsubs.");//CPU.UpdateCR1(CPU.FPR[frd]);
		}
		void FNMSUBS(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc)
		{
			CPU.FPR[frd] = static_cast<float>(-(CPU.FPR[fra] * CPU.FPR[frc] - CPU.FPR[frb]));
			CPU.FPSCR.FPRF = PPCdouble(CPU.FPR[frd]).GetType();
			if(rc) UNK("fnmsubs.");////CPU.UpdateCR1(CPU.FPR[frd]);
		}
		void FNMADDS(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc)
		{
			CPU.FPR[frd] = static_cast<float>(-(CPU.FPR[fra] * CPU.FPR[frc] + CPU.FPR[frb]));
			CPU.FPSCR.FPRF = PPCdouble(CPU.FPR[frd]).GetType();
			if(rc) UNK("fnmadds.");//CPU.UpdateCR1(CPU.FPR[frd]);
		}
	END_OPCODES_GROUP(G_3b);
	
	START_OPCODES_GROUP(G_3e)
		void STD(OP_REG rs, OP_REG ra, OP_sIMM d)
		{
			Memory.Write64(ra ? CPU.GPR[ra] + d : d, CPU.GPR[rs]);
		}
		void STDU(OP_REG rs, OP_REG ra, OP_sIMM ds)
		{
			//if(ra == 0 || rs == ra) return;
			const u64 addr = CPU.GPR[ra] + ds;
			Memory.Write64(addr, CPU.GPR[rs]);
			CPU.GPR[ra] = addr;
		}
	END_OPCODES_GROUP(G_3e);

	START_OPCODES_GROUP(G_3f)
		void MTFSB1(OP_REG bt, bool rc)
		{
			UNIMPLEMENTED();
		}
		void MCRFS(OP_REG bf, OP_REG bfa)
		{
			UNIMPLEMENTED();
		}
		void MTFSB0(OP_REG bt, bool rc)
		{
			UNIMPLEMENTED();
		}
		void MTFSFI(OP_REG crfd, OP_REG i, bool rc)
		{
			UNIMPLEMENTED();
		}
		void MFFS(OP_REG frd, bool rc)
		{
			(u64&)CPU.FPR[frd] = CPU.FPSCR.FPSCR;
			if(rc) UNK("mffs.");
		}
		void MTFSF(OP_REG flm, OP_REG frb, bool rc)
		{
			u32 mask = 0;
			for(u32 i=0; i<8; ++i)
			{
				if(flm & (1 << i)) mask |= 0xf << (i * 4);
			}

			CPU.FPSCR.FPSCR = (CPU.FPSCR.FPSCR & ~mask) | ((u32&)CPU.FPR[frb] & mask);
			if(rc) UNK("mtfsf.");
		}
		void FCMPU(OP_REG crfd, OP_REG fra, OP_REG frb)
		{
			if((CPU.FPSCR.FPRF = FPRdouble::Cmp(CPU.FPR[fra], CPU.FPR[frb])) == 1)
			{
				if(FPRdouble::IsSNaN(CPU.FPR[fra]) || FPRdouble::IsSNaN(CPU.FPR[frb]))
				{
					CPU.SetFPSCRException(FPSCR_VXSNAN);
				}
			}

			CPU.SetCR(crfd, CPU.FPSCR.FPRF);
		}
		void FRSP(OP_REG frd, OP_REG frb, bool rc)
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
		void FCTIW(OP_REG frd, OP_REG frb, bool rc)
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
		void FCTIWZ(OP_REG frd, OP_REG frb, bool rc)
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
		void FDIV(OP_REG frd, OP_REG fra, OP_REG frb, bool rc)
		{
			if(FPRdouble::IsINF(CPU.FPR[fra]) == 0.0 && CPU.FPR[frb] == 0.0)
			{
				CPU.FPSCR.VXZDZ = 1;
			}
			else if(FPRdouble::IsINF(CPU.FPR[fra]) && FPRdouble::IsINF(CPU.FPR[frb]))
			{
				CPU.FPSCR.VXIDI = 1;
			}
			else if(CPU.FPR[fra] != 0.0 && CPU.FPR[frb] == 0.0)
			{
				CPU.SetFPSCRException(FPSCR_ZX);
			}

			CPU.FPR[frd] = CPU.FPR[fra] / CPU.FPR[frb];
			CPU.FPSCR.FPRF = PPCdouble(CPU.FPR[frd]).GetType();
			if(rc) UNK("fdiv.");//CPU.UpdateCR1(CPU.FPR[frd]);
		}
		void FSUB(OP_REG frd, OP_REG fra, OP_REG frb, bool rc)
		{
			CPU.FPR[frd] = CPU.FPR[fra] - CPU.FPR[frb];
			CPU.FPSCR.FPRF = PPCdouble(CPU.FPR[frd]).GetType();
			if(rc) UNK("fsub.");//CPU.UpdateCR1(CPU.FPR[frd]);
		}
		void FADD(OP_REG frd, OP_REG fra, OP_REG frb, bool rc)
		{
			CPU.FPR[frd] = CPU.FPR[fra] + CPU.FPR[frb];
			CPU.FPSCR.FPRF = PPCdouble(CPU.FPR[frd]).GetType();
			if(rc) UNK("fadd.");//CPU.UpdateCR1(CPU.FPR[frd]);
		}
		void FSQRT(OP_REG frd, OP_REG frb, bool rc)
		{
			CPU.FPR[frd] = sqrt(CPU.FPR[frb]);
			CPU.FPSCR.FPRF = PPCdouble(CPU.FPR[frd]).GetType();
			if(rc) UNK("fsqrt.");//CPU.UpdateCR1(CPU.FPR[frd]);
		}
		void FSEL(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc)
		{
			CPU.FPR[frd] = CPU.FPR[fra] < 0.0 ? CPU.FPR[frc] : CPU.FPR[frb];
			if(rc) UNK("fsel.");//CPU.UpdateCR1(CPU.FPR[frd]);
		}
		void FMUL(OP_REG frd, OP_REG fra, OP_REG frc, bool rc)
		{
			CPU.FPR[frd] = CPU.FPR[fra] * CPU.FPR[frc];
			CPU.FPSCR.FI = 0;
			CPU.FPSCR.FR = 0;
			CPU.FPSCR.FPRF = PPCdouble(CPU.FPR[frd]).GetType();
			if(rc) UNK("fmul.");//CPU.UpdateCR1(CPU.FPR[frd]);
		}
		void FRSQRTE(OP_REG frd, OP_REG frb, bool rc)
		{
			UNIMPLEMENTED();
		}
		void FMSUB(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc)
		{
			CPU.FPR[frd] = CPU.FPR[fra] * CPU.FPR[frc] - CPU.FPR[frb];
			CPU.FPSCR.FPRF = PPCdouble(CPU.FPR[frd]).GetType();
			if(rc) UNK("fmsub.");//CPU.UpdateCR1(CPU.FPR[frd]);
		}
		void FMADD(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc)
		{
			CPU.FPR[frd] = CPU.FPR[fra] * CPU.FPR[frc] + CPU.FPR[frb];
			CPU.FPSCR.FPRF = PPCdouble(CPU.FPR[frd]).GetType();
			if(rc) UNK("fmadd.");//CPU.UpdateCR1(CPU.FPR[frd]);
		}
		void FNMSUB(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc)
		{
			CPU.FPR[frd] = -(CPU.FPR[fra] * CPU.FPR[frc] - CPU.FPR[frb]);
			CPU.FPSCR.FPRF = PPCdouble(CPU.FPR[frd]).GetType();
			if(rc) UNK("fnmsub.");//CPU.UpdateCR1(CPU.FPR[frd]);
		}
		void FNMADD(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc)
		{
			CPU.FPR[frd] = -(CPU.FPR[fra] * CPU.FPR[frc] + CPU.FPR[frb]);
			CPU.FPSCR.FPRF = PPCdouble(CPU.FPR[frd]).GetType();
			if(rc) UNK("fnmadd.");//CPU.UpdateCR1(CPU.FPR[frd]);
		}
		void FCMPO(OP_REG crfd, OP_REG fra, OP_REG frb)
		{
			if((CPU.FPSCR.FPRF = FPRdouble::Cmp(CPU.FPR[fra], CPU.FPR[frb])) == 1)
			{
				if(FPRdouble::IsSNaN(CPU.FPR[fra]) || FPRdouble::IsSNaN(CPU.FPR[frb]))
				{
					CPU.SetFPSCRException(FPSCR_VXSNAN);
					if(!CPU.FPSCR.VE) CPU.SetFPSCRException(FPSCR_VXVC);
				}
				else if(FPRdouble::IsQNaN(CPU.FPR[fra]) || FPRdouble::IsQNaN(CPU.FPR[frb]))
				{
					CPU.SetFPSCRException(FPSCR_VXVC);
				}

				CPU.FPSCR.FX = 1;
			}

			CPU.SetCR(crfd, CPU.FPSCR.FPRF);
		}
		void FNEG(OP_REG frd, OP_REG frb, bool rc)
		{
			CPU.FPR[frd] = -CPU.FPR[frb];
			if(rc) UNK("fneg.");//CPU.UpdateCR1(CPU.FPR[frd]);
		}
		void FMR(OP_REG frd, OP_REG frb, bool rc)
		{
			CPU.FPR[frd] = CPU.FPR[frb];
			if(rc) UNK("fmr.");//CPU.UpdateCR1(CPU.FPR[frd]);
		}
		void FNABS(OP_REG frd, OP_REG frb, bool rc)
		{
			(u64&)CPU.FPR[frd] = (u64&)CPU.FPR[frb] | 0x8000000000000000ULL;
			if(rc) UNK("fnabs.");//CPU.UpdateCR1(CPU.FPR[frd]);
		}
		void FABS(OP_REG frd, OP_REG frb, bool rc)
		{
			CPU.FPR[frd] = fabs(CPU.FPR[frb]);
			if(rc) UNK("fabs.");//CPU.UpdateCR1(CPU.FPR[frd]);
		}
		void FCTID(OP_REG frd, OP_REG frb, bool rc)
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
		void FCTIDZ(OP_REG frd, OP_REG frb, bool rc)
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
		void FCFID(OP_REG frd, OP_REG frb, bool rc)
		{
			CPU.FPR[frd] = (double)(u64&)CPU.FPR[frb];
			if(rc) UNK("fcfid.");//CPU.UpdateCR1(CPU.FPR[frd]);
		}
	END_OPCODES_GROUP(G_3f);

	void UNK(const u32 code, const u32 opcode, const u32 gcode)
	{
		UNK(wxString::Format("Unknown/Illegal opcode! (0x%08x : 0x%x : 0x%x)", code, opcode, gcode));
	}

	void UNK(const wxString& err, bool pause = true)
	{
		ConLog.Error(err + wxString::Format(" #pc: 0x%llx", CPU.PC));

		if(!pause) return;

		Emu.Pause();

		for(uint i=0; i<32; ++i) ConLog.Write("r%d = 0x%llx", i, CPU.GPR[i]);
		for(uint i=0; i<32; ++i) ConLog.Write("f%d = %llf", i, CPU.FPR[i]);
		for(uint i=0; i<32; ++i) ConLog.Write("v%d = 0x%s [%s]", i, CPU.VPR[i].ToString(true), CPU.VPR[i].ToString());
		ConLog.Write("CR = 0x%08x", CPU.CR);
		ConLog.Write("LR = 0x%llx", CPU.LR);
		ConLog.Write("CTR = 0x%llx", CPU.CTR);
		ConLog.Write("XER = 0x%llx [CA=%lld | OV=%lld | SO=%lld]", CPU.XER, CPU.XER.CA, CPU.XER.OV, CPU.XER.SO);
		ConLog.Write("FPSCR = 0x%x "
			"[RN=%d | NI=%d | XE=%d | ZE=%d | UE=%d | OE=%d | VE=%d | "
			"VXCVI=%d | VXSQRT=%d | VXSOFT=%d | FPRF=%d | "
			"FI=%d | FR=%d | VXVC=%d | VXIMZ=%d | "
			"VXZDZ=%d | VXIDI=%d | VXISI=%d | VXSNAN=%d | "
			"XX=%d | ZX=%d | UX=%d | OX=%d | VX=%d | FEX=%d | FX=%d]",
			CPU.FPSCR,
			CPU.FPSCR.RN,
			CPU.FPSCR.NI, CPU.FPSCR.XE, CPU.FPSCR.ZE, CPU.FPSCR.UE, CPU.FPSCR.OE, CPU.FPSCR.VE,
			CPU.FPSCR.VXCVI, CPU.FPSCR.VXSQRT, CPU.FPSCR.VXSOFT, CPU.FPSCR.FPRF,
			CPU.FPSCR.FI, CPU.FPSCR.FR, CPU.FPSCR.VXVC, CPU.FPSCR.VXIMZ,
			CPU.FPSCR.VXZDZ, CPU.FPSCR.VXIDI, CPU.FPSCR.VXISI, CPU.FPSCR.VXSNAN,
			CPU.FPSCR.XX, CPU.FPSCR.ZX, CPU.FPSCR.UX, CPU.FPSCR.OX, CPU.FPSCR.VX, CPU.FPSCR.FEX, CPU.FPSCR.FX);
	}
};

#undef START_OPCODES_GROUP
#undef END_OPCODES_GROUP