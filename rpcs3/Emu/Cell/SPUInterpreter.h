#pragma once

#include "Emu/Cell/SPUOpcodes.h"
#include "Emu/Memory/Memory.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/SysCalls/SysCalls.h"

#define START_OPCODES_GROUP(x)
#define END_OPCODES_GROUP(x)

class SPU_Interpreter : public SPU_Opcodes
{
private:
	SPUThread& CPU;

public:
	SPU_Interpreter(SPUThread& cpu) : CPU(cpu)
	{
	}

private:
	virtual void Exit(){}

	virtual void SysCall()
	{
	}

	//0 - 10
	virtual void STOP(OP_uIMM code)
	{
		Emu.Pause();
	}
	virtual void LNOP()
	{
	}
	virtual void RDCH(OP_REG rt, OP_REG ra)
	{
		CPU.ReadChannel(CPU.GPR[rt], ra);
	}
	virtual void RCHCNT(OP_REG rt, OP_REG ra)
	{
		CPU.GPR[rt].Reset();
		CPU.GPR[rt]._u32[0] = CPU.GetChannelCount(ra);
	}
	virtual void SF(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		CPU.GPR[rt]._u32[0] = CPU.GPR[rb]._u32[0] + ~CPU.GPR[ra]._u32[0] + 1;
		CPU.GPR[rt]._u32[1] = CPU.GPR[rb]._u32[1] + ~CPU.GPR[ra]._u32[1] + 1;
		CPU.GPR[rt]._u32[2] = CPU.GPR[rb]._u32[2] + ~CPU.GPR[ra]._u32[2] + 1;
		CPU.GPR[rt]._u32[3] = CPU.GPR[rb]._u32[3] + ~CPU.GPR[ra]._u32[3] + 1;
	}
	virtual void SHLI(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		const u32 s = i7 & 0x3f;

		for(u32 j = 0; j < 4; ++j)
		{
			const u32 t = CPU.GPR[ra]._u32[j];
			u32 r = 0;

			for(u32 b = 0; b + s < 32; ++b)
			{
				r |= t & (1 << (b + s));
			}

			CPU.GPR[rt]._u32[j] = r;
		}
	}
	virtual void A(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		CPU.GPR[rt]._u32[0] = CPU.GPR[ra]._u32[0] + CPU.GPR[rb]._u32[0];
		CPU.GPR[rt]._u32[1] = CPU.GPR[ra]._u32[1] + CPU.GPR[rb]._u32[1];
		CPU.GPR[rt]._u32[2] = CPU.GPR[ra]._u32[2] + CPU.GPR[rb]._u32[2];
		CPU.GPR[rt]._u32[3] = CPU.GPR[ra]._u32[3] + CPU.GPR[rb]._u32[3];
	}
	virtual void SPU_AND(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		CPU.GPR[rt]._u32[0] = CPU.GPR[ra]._u32[0] & CPU.GPR[rb]._u32[0];
		CPU.GPR[rt]._u32[1] = CPU.GPR[ra]._u32[1] & CPU.GPR[rb]._u32[1];
		CPU.GPR[rt]._u32[2] = CPU.GPR[ra]._u32[2] & CPU.GPR[rb]._u32[2];
		CPU.GPR[rt]._u32[3] = CPU.GPR[ra]._u32[3] & CPU.GPR[rb]._u32[3];
	}
	virtual void LQX(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		CPU.LSA = CPU.GPR[ra]._u32[0] + CPU.GPR[rb]._u32[0];
		CPU.GPR[rt]._u128 = CPU.ReadLSA128();
	}
	virtual void WRCH(OP_REG ra, OP_REG rt)
	{
		CPU.WriteChannel(ra, CPU.GPR[rt]);
	}
	virtual void STQX(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		CPU.LSA = CPU.GPR[ra]._u32[0] + CPU.GPR[rb]._u32[0];
		CPU.WriteLSA128(CPU.GPR[rt]._u128);
	}
	virtual void BI(OP_REG ra)
	{
		CPU.SetBranch(CPU.GPR[ra]._u32[0] & 0xfffffffc);
	}
	virtual void BISL(OP_REG rt, OP_REG ra)
	{
		CPU.SetBranch(CPU.GPR[ra]._u32[0] & 0xfffffffc);
		CPU.GPR[rt].Reset();
		CPU.GPR[rt]._u32[0] = CPU.PC + 4;
	}
	virtual void HBR(OP_REG p, OP_REG ro, OP_REG ra)
	{
		CPU.SetBranch(CPU.GPR[ra]._u32[0]);
	}
	virtual void CWX(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		const u32 t = ((CPU.GPR[ra]._u32[0] + CPU.GPR[rb]._u32[0]) & 0xc) / 4;
		for(u32 i=0; i<16; ++i) CPU.GPR[rt]._i8[i] = 0x1f - i;
		CPU.GPR[rt]._u32[t] = 0x10203;
	}
	virtual void ROTQBY(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		const s32 s = CPU.GPR[rb]._u8[0] & 0xf;

		for(u32 b = 0; b < 16; ++b)
		{
			if(b + s < 16)
			{
				CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b + s];
			}
			else
			{
				CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b + s - 16];
			}
		}
	}
	virtual void ROTQBYI(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		const u16 s = i7 & 0xf;

		for(u32 b = 0; b < 16; ++b)
		{
			if(b + s < 16)
			{
				CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b + s];
			}
			else
			{
				CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b + s - 16];
			}
		}
	}
	virtual void SHLQBYI(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		const u16 s = i7 & 0x1f;

		CPU.GPR[rt].Reset();

		for(u32 b = 0; b + s < 16; ++b)
		{
			CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b + s];
		}
	}
	virtual void SPU_NOP(OP_REG rt)
	{
	}
	virtual void CLGT(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for(u32 i = 0; i < 4; ++i)
		{
			CPU.GPR[rt]._u32[i] = (CPU.GPR[ra]._u32[i] > CPU.GPR[rb]._u32[i]) ? 0xffffffff : 0x00000000;
		}
	}

	//0 - 8
	virtual void BRZ(OP_REG rt, OP_sIMM i16)
	{
		if(!CPU.GPR[rt]._u32[0]) CPU.SetBranch(branchTarget(CPU.PC, i16));
	}
	virtual void BRHZ(OP_REG rt, OP_sIMM i16)
	{
		if(!CPU.GPR[rt]._u16[0]) CPU.SetBranch(branchTarget(CPU.PC, i16));
	}
	virtual void BRHNZ(OP_REG rt, OP_sIMM i16)
	{
		if(CPU.GPR[rt]._u16[0]) CPU.SetBranch(branchTarget(CPU.PC, i16));
	}
	virtual void STQR(OP_REG rt, OP_sIMM i16)
	{
		CPU.LSA = branchTarget(CPU.PC, i16);
		CPU.WriteLSA128(CPU.GPR[rt]._u128);
	}
	virtual void BR(OP_sIMM i16)
	{
		CPU.SetBranch(branchTarget(CPU.PC, i16));
	}
	virtual void FSMBI(OP_REG rt, OP_sIMM i16)
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
	virtual void BRSL(OP_REG rt, OP_sIMM i16)
	{
		CPU.GPR[rt].Reset();
		CPU.GPR[rt]._u32[0] = CPU.PC + 4;
		CPU.SetBranch(branchTarget(CPU.PC, i16));
	}
	virtual void IL(OP_REG rt, OP_sIMM i16)
	{
		CPU.GPR[rt]._u32[0] = i16;
		CPU.GPR[rt]._u32[1] = i16;
		CPU.GPR[rt]._u32[2] = i16;
		CPU.GPR[rt]._u32[3] = i16;
	}
	virtual void LQR(OP_REG rt, OP_sIMM i16)
	{
		CPU.LSA = branchTarget(CPU.PC, i16);
		if(!Memory.IsGoodAddr(CPU.LSA))
		{
			ConLog.Warning("LQR: Bad addr: 0x%x", CPU.LSA);
			return;
		}

		CPU.GPR[rt]._u128 = CPU.ReadLSA128();
	}

	//0 - 7
	virtual void SPU_ORI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		for(u32 i = 0; i < 4; ++i)
		{
			CPU.GPR[rt]._u32[i] = CPU.GPR[ra]._u32[i] | i10;
		}
	}
	virtual void AI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		for(u32 i = 0; i < 4; ++i)
		{
			CPU.GPR[rt]._u32[i] = CPU.GPR[ra]._u32[i] + i10;
		}
	}
	virtual void AHI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		for(u32 i = 0; i < 8; ++i)
		{
			CPU.GPR[rt]._u16[i] = CPU.GPR[ra]._u16[i] + i10;
		}
	}
	virtual void STQD(OP_REG rt, OP_sIMM i10, OP_REG ra)
	{
		CPU.LSA = branchTarget(0, i10 + CPU.GPR[ra]._u32[0]);
		CPU.WriteLSA128(CPU.GPR[rt]._u128);
	}
	virtual void LQD(OP_REG rt, OP_sIMM i10, OP_REG ra)
	{
		CPU.LSA = branchTarget(0, i10 + CPU.GPR[ra]._u32[0]);
		CPU.GPR[rt]._u128 = CPU.ReadLSA128();
	}
	virtual void CLGTI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		for(u32 i = 0; i < 4; ++i)
		{
			CPU.GPR[rt]._u32[i] = (CPU.GPR[rt]._u32[i] > (u32)i10) ? 0xffffffff : 0x00000000;
		}
	}
	virtual void CLGTHI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		for(u32 i = 0; i < 8; ++i)
		{
			CPU.GPR[rt]._u16[i] = (CPU.GPR[rt]._u16[i] > (u16)i10) ? 0xffff : 0x0000;
		}
	}
	virtual void CEQI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		for(u32 i = 0; i < 4; ++i)
		{
			CPU.GPR[rt]._u32[i] = (CPU.GPR[rt]._u32[i] == (u32)i10) ? 0xffffffff : 0x00000000;
		}
	}

	//0 - 6
	virtual void HBRR(OP_sIMM ro, OP_sIMM i16)
	{
		//CHECK ME
		//CPU.GPR[0]._u64[0] = branchTarget(CPU.PC, i16);
		//CPU.SetBranch(branchTarget(CPU.PC, ro));
	}
	virtual void ILA(OP_REG rt, OP_sIMM i18)
	{
		CPU.GPR[rt]._u32[0] = i18;
		CPU.GPR[rt]._u32[1] = i18;
		CPU.GPR[rt]._u32[2] = i18;
		CPU.GPR[rt]._u32[3] = i18;
	}

	//0 - 3
	virtual void SELB(OP_REG rt, OP_REG ra, OP_REG rb, OP_REG rc)
	{
		for(u32 i = 0; i < 4; ++i)
		{
			CPU.GPR[rt]._u32[i] =
				( CPU.GPR[rc]._u32[i] & CPU.GPR[rb]._u32[i]) |
				(~CPU.GPR[rc]._u32[i] & CPU.GPR[ra]._u32[i]);
		}
		/*
		CPU.GPR[rt] = _mm_or_si128(
				_mm_and_si128(CPU.GPR[rc], CPU.GPR[rb]),
				_mm_andnot_si128(CPU.GPR[rc], CPU.GPR[ra])
			);
		*/
	}
	virtual void SHUFB(OP_REG rc, OP_REG ra, OP_REG rb, OP_REG rt)
	{
		ConLog.Warning("SHUFB");
	}

	virtual void UNK(const s32 code, const s32 opcode, const s32 gcode)
	{
		UNK(wxString::Format("Unknown/Illegal opcode! (0x%08x)", code));
	}

	void UNK(const wxString& err)
	{
		ConLog.Error(err + wxString::Format(" #pc: 0x%x", CPU.PC));
		Emu.Pause();
		for(uint i=0; i<128; ++i) ConLog.Write("r%d = 0x%s", i, CPU.GPR[i].ToString());
	}
};

#undef START_OPCODES_GROUP
#undef END_OPCODES_GROUP