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
	virtual void SYNC(OP_uIMM Cbit)
	{
		// TODO
	}
	virtual void DSYNC()
	{
		// TODO
	}
	virtual void MFSPR(OP_REG rt, OP_REG sa)
	{
		// TODO
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
	virtual void OR(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		CPU.GPR[rt]._u32[0] = CPU.GPR[ra]._u32[0] | CPU.GPR[rb]._u32[0];
		CPU.GPR[rt]._u32[1] = CPU.GPR[ra]._u32[1] | CPU.GPR[rb]._u32[1];
		CPU.GPR[rt]._u32[2] = CPU.GPR[ra]._u32[2] | CPU.GPR[rb]._u32[2];
		CPU.GPR[rt]._u32[3] = CPU.GPR[ra]._u32[3] | CPU.GPR[rb]._u32[3];
	}
	virtual void BG(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		CPU.GPR[rt]._u32[0] = CPU.GPR[ra]._u32[0] > CPU.GPR[rb]._u32[0] ? 0 : 1;
		CPU.GPR[rt]._u32[1] = CPU.GPR[ra]._u32[1] > CPU.GPR[rb]._u32[1] ? 0 : 1;
		CPU.GPR[rt]._u32[2] = CPU.GPR[ra]._u32[2] > CPU.GPR[rb]._u32[2] ? 0 : 1;
		CPU.GPR[rt]._u32[3] = CPU.GPR[ra]._u32[3] > CPU.GPR[rb]._u32[3] ? 0 : 1;
	}
	virtual void SFH(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = CPU.GPR[rb]._u16[h] - CPU.GPR[ra]._u16[h];
	}
	virtual void NOR(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		CPU.GPR[rt]._u32[0] = ~(CPU.GPR[ra]._u32[0] | CPU.GPR[rb]._u32[0]);
		CPU.GPR[rt]._u32[1] = ~(CPU.GPR[ra]._u32[1] | CPU.GPR[rb]._u32[1]);
		CPU.GPR[rt]._u32[2] = ~(CPU.GPR[ra]._u32[2] | CPU.GPR[rb]._u32[2]);
		CPU.GPR[rt]._u32[3] = ~(CPU.GPR[ra]._u32[3] | CPU.GPR[rb]._u32[3]);
	}
	virtual void ABSDB(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = CPU.GPR[rb]._u8[b] > CPU.GPR[ra]._u8[b] ? CPU.GPR[rb]._u8[b] - CPU.GPR[ra]._u8[b] : CPU.GPR[ra]._u8[b] - CPU.GPR[rb]._u8[b];
	}
	virtual void ROT(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		CPU.GPR[rt]._u32[0] = (CPU.GPR[ra]._u32[0] << (CPU.GPR[rb]._u32[0] & 0x1f)) | (CPU.GPR[ra]._u32[0] >> (32 - (CPU.GPR[rb]._u32[0] & 0x1f)));
		CPU.GPR[rt]._u32[1] = (CPU.GPR[ra]._u32[1] << (CPU.GPR[rb]._u32[1] & 0x1f)) | (CPU.GPR[ra]._u32[1] >> (32 - (CPU.GPR[rb]._u32[1] & 0x1f)));
		CPU.GPR[rt]._u32[2] = (CPU.GPR[ra]._u32[2] << (CPU.GPR[rb]._u32[2] & 0x1f)) | (CPU.GPR[ra]._u32[2] >> (32 - (CPU.GPR[rb]._u32[2] & 0x1f)));
		CPU.GPR[rt]._u32[3] = (CPU.GPR[ra]._u32[3] << (CPU.GPR[rb]._u32[3] & 0x1f)) | (CPU.GPR[ra]._u32[3] >> (32 - (CPU.GPR[rb]._u32[3] & 0x1f)));
	}
	virtual void ROTM(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		CPU.GPR[rt]._u32[0] = ((0 - CPU.GPR[rb]._u32[0]) % 64) < 32 ? CPU.GPR[ra]._u32[0] >> ((0 - CPU.GPR[rb]._u32[0]) % 64) : 0;
		CPU.GPR[rt]._u32[1] = ((0 - CPU.GPR[rb]._u32[1]) % 64) < 32 ? CPU.GPR[ra]._u32[1] >> ((0 - CPU.GPR[rb]._u32[1]) % 64) : 0;
		CPU.GPR[rt]._u32[2] = ((0 - CPU.GPR[rb]._u32[2]) % 64) < 32 ? CPU.GPR[ra]._u32[2] >> ((0 - CPU.GPR[rb]._u32[2]) % 64) : 0;
		CPU.GPR[rt]._u32[3] = ((0 - CPU.GPR[rb]._u32[3]) % 64) < 32 ? CPU.GPR[ra]._u32[3] >> ((0 - CPU.GPR[rb]._u32[3]) % 64) : 0;
	}
	virtual void ROTMA(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		CPU.GPR[rt]._i32[0] = ((0 - CPU.GPR[rb]._i32[0]) % 64) < 32 ? CPU.GPR[ra]._i32[0] >> ((0 - CPU.GPR[rb]._i32[0]) % 64) : CPU.GPR[ra]._i32[0] >> 31;
		CPU.GPR[rt]._i32[1] = ((0 - CPU.GPR[rb]._i32[1]) % 64) < 32 ? CPU.GPR[ra]._i32[1] >> ((0 - CPU.GPR[rb]._i32[1]) % 64) : CPU.GPR[ra]._i32[1] >> 31;
		CPU.GPR[rt]._i32[2] = ((0 - CPU.GPR[rb]._i32[2]) % 64) < 32 ? CPU.GPR[ra]._i32[2] >> ((0 - CPU.GPR[rb]._i32[2]) % 64) : CPU.GPR[ra]._i32[2] >> 31;
		CPU.GPR[rt]._i32[3] = ((0 - CPU.GPR[rb]._i32[3]) % 64) < 32 ? CPU.GPR[ra]._i32[3] >> ((0 - CPU.GPR[rb]._i32[3]) % 64) : CPU.GPR[ra]._i32[3] >> 31;
	}
	virtual void SHL(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		CPU.GPR[rt]._u32[0] = (CPU.GPR[rb]._u32[0] & 0x3f) > 31 ? 0 : CPU.GPR[ra]._u32[0] << (CPU.GPR[rb]._u32[0] & 0x3f);
		CPU.GPR[rt]._u32[1] = (CPU.GPR[rb]._u32[1] & 0x3f) > 31 ? 0 : CPU.GPR[ra]._u32[1] << (CPU.GPR[rb]._u32[1] & 0x3f);
		CPU.GPR[rt]._u32[2] = (CPU.GPR[rb]._u32[2] & 0x3f) > 31 ? 0 : CPU.GPR[ra]._u32[2] << (CPU.GPR[rb]._u32[2] & 0x3f);
		CPU.GPR[rt]._u32[3] = (CPU.GPR[rb]._u32[3] & 0x3f) > 31 ? 0 : CPU.GPR[ra]._u32[3] << (CPU.GPR[rb]._u32[3] & 0x3f);
	}
	virtual void ROTH(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int h = 0; h < 8; h++) 
			CPU.GPR[rt]._u16[h] = (CPU.GPR[ra]._u16[h] << (CPU.GPR[rb]._u16[h] & 0xf)) | (CPU.GPR[ra]._u16[h] >> (16 - (CPU.GPR[rb]._u32[h] & 0xf)));
	}
	virtual void ROTHM(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = ((0 - CPU.GPR[rb]._u16[h]) % 32) < 16 ? CPU.GPR[ra]._u16[h] >> ((0 - CPU.GPR[rb]._u16[h]) % 32) : 0;
	}
	virtual void ROTMAH(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._i16[h] = ((0 - CPU.GPR[rb]._i16[h]) % 32) < 16 ? CPU.GPR[ra]._i16[h] >> ((0 - CPU.GPR[rb]._i16[h]) % 32) : CPU.GPR[ra]._i16[h] >> 15;
	}
	virtual void SHLH(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = (CPU.GPR[rb]._u16[h] & 0x1f) > 15 ? 0 : CPU.GPR[ra]._u16[h] << (CPU.GPR[rb]._u16[h] & 0x3f);
	}
	virtual void ROTI(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		const int nRot = i7 & 0x1f;
		CPU.GPR[rt]._u32[0] = (CPU.GPR[ra]._u32[0] << nRot) | (CPU.GPR[ra]._u32[0] >> (32 - nRot));
		CPU.GPR[rt]._u32[1] = (CPU.GPR[ra]._u32[1] << nRot) | (CPU.GPR[ra]._u32[1] >> (32 - nRot));
		CPU.GPR[rt]._u32[2] = (CPU.GPR[ra]._u32[2] << nRot) | (CPU.GPR[ra]._u32[2] >> (32 - nRot));
		CPU.GPR[rt]._u32[3] = (CPU.GPR[ra]._u32[3] << nRot) | (CPU.GPR[ra]._u32[3] >> (32 - nRot));
	}
	virtual void ROTMI(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		const int nRot = (0 - i7) % 64;
		CPU.GPR[rt]._u32[0] = nRot < 32 ? CPU.GPR[ra]._u32[0] >> nRot : 0;
		CPU.GPR[rt]._u32[1] = nRot < 32 ? CPU.GPR[ra]._u32[1] >> nRot : 0;
		CPU.GPR[rt]._u32[2] = nRot < 32 ? CPU.GPR[ra]._u32[2] >> nRot : 0;
		CPU.GPR[rt]._u32[3] = nRot < 32 ? CPU.GPR[ra]._u32[3] >> nRot : 0;
	}
	virtual void ROTMAI(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		const int nRot = (0 - i7) % 64;
		CPU.GPR[rt]._i32[0] = nRot < 32 ? CPU.GPR[ra]._i32[0] >> nRot : CPU.GPR[ra]._i32[0] >> 31;
		CPU.GPR[rt]._i32[1] = nRot < 32 ? CPU.GPR[ra]._i32[1] >> nRot : CPU.GPR[ra]._i32[1] >> 31;
		CPU.GPR[rt]._i32[2] = nRot < 32 ? CPU.GPR[ra]._i32[2] >> nRot : CPU.GPR[ra]._i32[2] >> 31;
		CPU.GPR[rt]._i32[3] = nRot < 32 ? CPU.GPR[ra]._i32[3] >> nRot : CPU.GPR[ra]._i32[3] >> 31;
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
	virtual void ROTHI(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		const int nRot = i7 & 0xf;

		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = (CPU.GPR[ra]._u16[h] << nRot) | (CPU.GPR[ra]._u16[h] >> (16 - nRot));
	}
	virtual void ROTHMI(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		const int nRot = (0 - i7) % 32;

		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = nRot < 16 ? CPU.GPR[ra]._u16[h] >> nRot : 0;
	}
	virtual void ROTMAHI(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		const int nRot = (0 - i7) % 32;

		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._i16[h] = nRot < 16 ? CPU.GPR[ra]._i16[h] >> nRot : CPU.GPR[ra]._i16[h] >> 15;
	}
	virtual void SHLHI(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		const int nRot = i7 & 0x1f;

		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[0] = nRot > 15 ? 0 : CPU.GPR[ra]._u16[0] << nRot;
	}
	virtual void A(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		CPU.GPR[rt]._u32[0] = CPU.GPR[ra]._u32[0] + CPU.GPR[rb]._u32[0];
		CPU.GPR[rt]._u32[1] = CPU.GPR[ra]._u32[1] + CPU.GPR[rb]._u32[1];
		CPU.GPR[rt]._u32[2] = CPU.GPR[ra]._u32[2] + CPU.GPR[rb]._u32[2];
		CPU.GPR[rt]._u32[3] = CPU.GPR[ra]._u32[3] + CPU.GPR[rb]._u32[3];
	}
	virtual void AND(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		CPU.GPR[rt]._u32[0] = CPU.GPR[ra]._u32[0] & CPU.GPR[rb]._u32[0];
		CPU.GPR[rt]._u32[1] = CPU.GPR[ra]._u32[1] & CPU.GPR[rb]._u32[1];
		CPU.GPR[rt]._u32[2] = CPU.GPR[ra]._u32[2] & CPU.GPR[rb]._u32[2];
		CPU.GPR[rt]._u32[3] = CPU.GPR[ra]._u32[3] & CPU.GPR[rb]._u32[3];
	}
	virtual void CG(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		CPU.GPR[rt]._u32[0] = ((CPU.GPR[ra]._u32[0] + CPU.GPR[rb]._u32[0]) < CPU.GPR[ra]._u32[0]) ? 1 : 0;
		CPU.GPR[rt]._u32[1] = ((CPU.GPR[ra]._u32[1] + CPU.GPR[rb]._u32[1]) < CPU.GPR[ra]._u32[1]) ? 1 : 0;
		CPU.GPR[rt]._u32[2] = ((CPU.GPR[ra]._u32[2] + CPU.GPR[rb]._u32[2]) < CPU.GPR[ra]._u32[2]) ? 1 : 0;
		CPU.GPR[rt]._u32[3] = ((CPU.GPR[ra]._u32[3] + CPU.GPR[rb]._u32[3]) < CPU.GPR[ra]._u32[3]) ? 1 : 0;
	}
	virtual void AH(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = CPU.GPR[ra]._u16[h] + CPU.GPR[rb]._u16[h];
	}
	virtual void NAND(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		CPU.GPR[rt]._u32[0] = ~(CPU.GPR[ra]._u32[0] & CPU.GPR[rb]._u32[0]);
		CPU.GPR[rt]._u32[1] = ~(CPU.GPR[ra]._u32[1] & CPU.GPR[rb]._u32[1]);
		CPU.GPR[rt]._u32[2] = ~(CPU.GPR[ra]._u32[2] & CPU.GPR[rb]._u32[2]);
		CPU.GPR[rt]._u32[3] = ~(CPU.GPR[ra]._u32[3] & CPU.GPR[rb]._u32[3]);
	}
	virtual void AVGB(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = (CPU.GPR[ra]._u8[b] + CPU.GPR[rb]._u8[b] + 1) >> 1;
	}
	virtual void MTSPR(OP_REG rt, OP_REG sa)
	{
		// TODO
	}
	virtual void WRCH(OP_REG ra, OP_REG rt)
	{
		CPU.WriteChannel(ra, CPU.GPR[rt]);
	}
	virtual void BIZ(OP_REG rt, OP_REG ra)
	{
		if(CPU.GPR[rt]._u32[0] == 0)
			CPU.SetBranch(CPU.GPR[ra]._u32[0] & 0xfffffffc);
	}
	virtual void BINZ(OP_REG rt, OP_REG ra)
	{
		if(CPU.GPR[rt]._u32[0] != 0)
			CPU.SetBranch(CPU.GPR[ra]._u32[0] & 0xfffffffc);
	}
	virtual void BIHZ(OP_REG rt, OP_REG ra)
	{
		if(CPU.GPR[rt]._u16[0] == 0)
			CPU.SetBranch(CPU.GPR[ra]._u32[0] & 0xfffffffc);
	}
	virtual void BIHNZ(OP_REG rt, OP_REG ra)
	{
		if(CPU.GPR[rt]._u16[0] != 0)
			CPU.SetBranch(CPU.GPR[ra]._u32[0] & 0xfffffffc);
	}
	virtual void STOPD(OP_REG rc, OP_REG ra, OP_REG rb)
	{
		Emu.Pause();
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
	virtual void IRET(OP_REG ra)
	{
		// TODO
		// SetBranch(SRR0);
	}
	virtual void BISLED(OP_REG rt, OP_REG ra)
	{
		// TODO
	}
	virtual void HBR(OP_REG p, OP_REG ro, OP_REG ra)
	{
		CPU.SetBranch(CPU.GPR[ra]._u32[0]);
	}
	virtual void GB(OP_REG rt, OP_REG ra)
	{
		CPU.GPR[rt].Reset();
		CPU.GPR[rt]._u32[0] =	(CPU.GPR[ra]._u32[0] & 1) |
										((CPU.GPR[ra]._u32[1] & 1) << 1) |
										((CPU.GPR[ra]._u32[2] & 1) << 2) |
										((CPU.GPR[ra]._u32[3] & 1) << 3);
	}
	virtual void GBH(OP_REG rt, OP_REG ra)
	{
		CPU.GPR[rt].Reset();

		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u32[0] |= (CPU.GPR[ra]._u16[h] & 1) << h;
	}
	virtual void GBB(OP_REG rt, OP_REG ra)
	{
		CPU.GPR[rt].Reset();

		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u32[0] |= (CPU.GPR[ra]._u8[b] & 1) << b;
	}
	virtual void FSM(OP_REG rt, OP_REG ra)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = (CPU.GPR[ra]._u32[0] & (8 >> w)) ? ~0 : 0;
	}
	virtual void FSMH(OP_REG rt, OP_REG ra)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = (CPU.GPR[ra]._u32[0] & (128 >> h)) ? ~0 : 0;
	}
	virtual void FSMB(OP_REG rt, OP_REG ra)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = (CPU.GPR[ra]._u32[0] & (32768 >> b)) ? ~0 : 0;
	}
	virtual void FREST(OP_REG rt, OP_REG ra)
	{
		// TODO
	}
	virtual void FRSQEST(OP_REG rt, OP_REG ra)
	{
		// TODO
	}
	virtual void LQX(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		CPU.LSA = CPU.GPR[ra]._u32[0] + CPU.GPR[rb]._u32[0];
		CPU.GPR[rt]._u128 = CPU.ReadLSA128();
	}
	virtual void ROTQBYBI(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		const int nShift = (CPU.GPR[rb]._u32[0] >> 3) & 0xf;

		for (int b = 0; b < 8; b++)
			CPU.GPR[rt]._u8[b] = nShift == 0 ? CPU.GPR[ra]._u8[b] : (CPU.GPR[ra]._u8[b] << nShift) | (CPU.GPR[ra]._u8[b] >> (16 - nShift));
	}
	virtual void ROTQMBYBI(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		const int nShift = ((0 - CPU.GPR[rb]._u32[0]) >> 3) & 0x1f;

		for (int b = 0; b < 16; b++)
		{
			if (b >= nShift)
				CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b - nShift];
			else
				CPU.GPR[rt]._u8[b] = 0;
		}
	}
	virtual void SHLQBYBI(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		const int nShift = (CPU.GPR[rb]._u32[0] >> 3) & 0x1f;

		for (int b = 0; b < 16; b++)
		{
			if ((b + nShift) < 16)
				CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b + nShift];
			else
				CPU.GPR[rt]._u8[b] = 0;
		}
	}
	virtual void CBX(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		int n = (CPU.GPR[rb]._u32[0] + CPU.GPR[ra]._u32[0]) & 0xf;

		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = b == n ? 3 : b | 0x10;
	}
	virtual void CHX(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		int n = ((CPU.GPR[rb]._u32[0] + CPU.GPR[ra]._u32[0]) & 0xf) >> 1;

		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = h == n ? 0x0203 : (h * 2 * 0x0101 + 0x1011);
	}
	virtual void CWX(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		const u32 t = ((CPU.GPR[ra]._u32[0] + CPU.GPR[rb]._u32[0]) & 0xc) / 4;
		for(u32 i=0; i<16; ++i) CPU.GPR[rt]._i8[i] = 0x10 + i;
		CPU.GPR[rt]._u32[t] = 0x10203;
	}
	virtual void CDX(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		int n = ((CPU.GPR[rb]._u32[0] + CPU.GPR[ra]._u32[0]) & 0x8) >> 2;

		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = (w == n) ? 0x00010203 : (w == (n + 1)) ? 0x04050607 : (0x01010101 * (w * 4) + 0x10111213);
	}
	virtual void ROTQBI(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		int nShift = CPU.GPR[rb]._u32[0] & 0x7;

		CPU.GPR[rt]._u32[0] = (CPU.GPR[ra]._u32[0] << nShift) | (CPU.GPR[ra]._u32[1] >> (32 - nShift));
		CPU.GPR[rt]._u32[1] = (CPU.GPR[ra]._u32[1] << nShift) | (CPU.GPR[ra]._u32[2] >> (32 - nShift));
		CPU.GPR[rt]._u32[2] = (CPU.GPR[ra]._u32[2] << nShift) | (CPU.GPR[ra]._u32[3] >> (32 - nShift));
		CPU.GPR[rt]._u32[3] = (CPU.GPR[ra]._u32[3] << nShift) | (CPU.GPR[ra]._u32[0] >> (32 - nShift));
	}
	virtual void ROTQMBI(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		int nShift = (0 - CPU.GPR[rb]._u32[0]) % 8;

		CPU.GPR[rt]._u32[0] = CPU.GPR[ra]._u32[0] >> nShift;
		CPU.GPR[rt]._u32[1] = (CPU.GPR[ra]._u32[1] >> nShift) | (CPU.GPR[ra]._u32[0] << (32 - nShift));
		CPU.GPR[rt]._u32[2] = (CPU.GPR[ra]._u32[2] >> nShift) | (CPU.GPR[ra]._u32[1] << (32 - nShift));
		CPU.GPR[rt]._u32[3] = (CPU.GPR[ra]._u32[3] >> nShift) | (CPU.GPR[ra]._u32[2] << (32 - nShift));
	}
	virtual void SHLQBI(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		const int nShift = CPU.GPR[rb]._u32[0] & 0x7;

		CPU.GPR[rt]._u32[0] = (CPU.GPR[ra]._u32[0] << nShift) | (CPU.GPR[ra]._u32[1] >> (32 - nShift));
		CPU.GPR[rt]._u32[1] = (CPU.GPR[ra]._u32[1] << nShift) | (CPU.GPR[ra]._u32[2] >> (32 - nShift));
		CPU.GPR[rt]._u32[2] = (CPU.GPR[ra]._u32[2] << nShift) | (CPU.GPR[ra]._u32[3] >> (32 - nShift));
		CPU.GPR[rt]._u32[3] = CPU.GPR[ra]._u32[3] << nShift;
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
	virtual void ROTQMBY(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		const int nShift = (0 - CPU.GPR[rb]._u32[0]) % 32;

		for (int b = 0; b < 16; b++)
			if (b >= nShift)
				CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b - nShift];
			else
				CPU.GPR[rt]._u8[b] = 0;
	}
	virtual void SHLQBY(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		const int nShift = CPU.GPR[rb]._u32[0] & 0x1f;

		for (int b = 0; b < 16; b++)
			if (b + nShift < 16)
				CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b + nShift];
			else
				CPU.GPR[rt]._u8[b] = 0;
	}
	virtual void ORX(OP_REG rt, OP_REG ra)
	{
		CPU.GPR[rt].Reset();

		CPU.GPR[rt]._u32[0] = CPU.GPR[ra]._u32[0] | CPU.GPR[ra]._u32[1] | CPU.GPR[ra]._u32[2] | CPU.GPR[ra]._u32[3];
	}
	virtual void CBD(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		const int n = (CPU.GPR[ra]._u32[0] + i7) & 0xf;

		for (int b = 0; b < 16; b++)
			if (b == n)
				CPU.GPR[rt]._u8[b] = 0x3;
			else
				CPU.GPR[rt]._u8[b] = b | 0x10;
	}
	virtual void CHD(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		int n = ((CPU.GPR[ra]._u32[0] + i7) & 0xf) >> 1;

		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = h == n ? 0x0203 : (h * 2 * 0x0101 + 0x1011);
	}
	virtual void CWD(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		const int t = ((CPU.GPR[ra]._u32[0] + i7) & 0xf) >> 2;

		for (int i=0; i<16; ++i) 
			CPU.GPR[rt]._u8[i] = 0x10 + i;

		CPU.GPR[rt]._u32[t] = 0x10203;
	}
	virtual void CDD(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		const int t = ((CPU.GPR[ra]._u32[0] + i7) & 0xf) >> 3;

		for (int i=0; i<16; ++i) 
			CPU.GPR[rt]._u8[i] = 0x10 + i;

		CPU.GPR[rt]._u64[t] = (u64)0x0001020304050607;
	}
	virtual void ROTQBII(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		int nShift = i7 & 0x7;

		CPU.GPR[rt]._u32[0] = (CPU.GPR[ra]._u32[0] << nShift) | (CPU.GPR[ra]._u32[1] >> (32 - nShift));
		CPU.GPR[rt]._u32[1] = (CPU.GPR[ra]._u32[1] << nShift) | (CPU.GPR[ra]._u32[2] >> (32 - nShift));
		CPU.GPR[rt]._u32[2] = (CPU.GPR[ra]._u32[2] << nShift) | (CPU.GPR[ra]._u32[3] >> (32 - nShift));
		CPU.GPR[rt]._u32[3] = (CPU.GPR[ra]._u32[3] << nShift) | (CPU.GPR[ra]._u32[0] >> (32 - nShift));
	}
	virtual void ROTQMBII(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		int nShift = (0 - i7) % 8;

		CPU.GPR[rt]._u32[0] = CPU.GPR[ra]._u32[0] >> nShift;
		CPU.GPR[rt]._u32[1] = (CPU.GPR[ra]._u32[1] >> nShift) | (CPU.GPR[ra]._u32[0] << (32 - nShift));
		CPU.GPR[rt]._u32[2] = (CPU.GPR[ra]._u32[2] >> nShift) | (CPU.GPR[ra]._u32[1] << (32 - nShift));
		CPU.GPR[rt]._u32[3] = (CPU.GPR[ra]._u32[3] >> nShift) | (CPU.GPR[ra]._u32[2] << (32 - nShift));
	}
	virtual void SHLQBII(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		const int nShift = i7 & 0x7;

		CPU.GPR[rt]._u32[0] = (CPU.GPR[ra]._u32[0] << nShift) | (CPU.GPR[ra]._u32[1] >> (32 - nShift));
		CPU.GPR[rt]._u32[1] = (CPU.GPR[ra]._u32[1] << nShift) | (CPU.GPR[ra]._u32[2] >> (32 - nShift));
		CPU.GPR[rt]._u32[2] = (CPU.GPR[ra]._u32[2] << nShift) | (CPU.GPR[ra]._u32[3] >> (32 - nShift));
		CPU.GPR[rt]._u32[3] = CPU.GPR[ra]._u32[3] << nShift;
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
	virtual void ROTQMBYI(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		const int nShift = (0 - i7) % 32;

		for (int b = 0; b < 16; b++)
			if (b >= nShift)
				CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b - nShift];
			else
				CPU.GPR[rt]._u8[b] = 0;
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
	virtual void NOP(OP_REG rt)
	{
	}
	virtual void CGT(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._i32[w] > CPU.GPR[rb]._i32[w] ? 0xffffffff : 0;
	}
	virtual void XOR(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._u32[w] ^ CPU.GPR[rb]._u32[w];
	}
	virtual void CGTH(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = CPU.GPR[ra]._i16[h] > CPU.GPR[rb]._i16[h] ? 0xffff : 0;
	}
	virtual void EQV(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = (CPU.GPR[ra]._u32[w] & CPU.GPR[rb]._u32[w]) | ~(CPU.GPR[ra]._u32[w] | CPU.GPR[rb]._u32[w]);
	}
	virtual void CGTB(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._i8[b] > CPU.GPR[rb]._i8[b] ? 0xff : 0;
	}
	virtual void SUMB(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int w = 0; w < 4; w++)
		{
			CPU.GPR[rt]._u16[w*2] = CPU.GPR[ra]._u8[w*4] + CPU.GPR[ra]._u8[w*4 + 1] + CPU.GPR[ra]._u8[w*4 + 2] + CPU.GPR[ra]._u8[w*4 + 3];
			CPU.GPR[rt]._u16[w*2 + 1] = CPU.GPR[rb]._u8[w*4] + CPU.GPR[rb]._u8[w*4 + 1] + CPU.GPR[rb]._u8[w*4 + 2] + CPU.GPR[rb]._u8[w*4 + 3];
		}
	}
	virtual void HGT(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		// TODO
	}
	virtual void CLZ(OP_REG rt, OP_REG ra)
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
	virtual void XSWD(OP_REG rt, OP_REG ra)
	{
		CPU.GPR[rt]._i64[0] = (s64)CPU.GPR[ra]._i32[1];
		CPU.GPR[rt]._i64[1] = (s64)CPU.GPR[ra]._i32[3];
	}
	virtual void XSHW(OP_REG rt, OP_REG ra)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._i32[w] = (s32)CPU.GPR[ra]._i16[w*2 + 1];
	}
	virtual void CNTB(OP_REG rt, OP_REG ra)
	{
		CPU.GPR[rt].Reset();
		
		for (int b = 0; b < 16; b++)
			for (int i = 0; i < 8; i++)
				CPU.GPR[rt]._u8[b] += (CPU.GPR[ra]._u8[b] & (1 << i)) ? 1 : 0;
	}
	virtual void XSBH(OP_REG rt, OP_REG ra)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._i16[h] = (s16)CPU.GPR[ra]._i8[h*2 + 1];
	}
	virtual void CLGT(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for(u32 i = 0; i < 4; ++i)
		{
			CPU.GPR[rt]._u32[i] = (CPU.GPR[ra]._u32[i] > CPU.GPR[rb]._u32[i]) ? 0xffffffff : 0x00000000;
		}
	}
	virtual void ANDC(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._u32[w] & (~CPU.GPR[rb]._u32[w]);
	}
	virtual void FCGT(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		// TODO
	}
	virtual void DFCGT(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		// TODO
	}
	virtual void FA(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		// TODO
	}
	virtual void FS(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		// TODO
	}
	virtual void FM(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		// TODO
	}
	virtual void CLGTH(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = CPU.GPR[ra]._u16[h] > CPU.GPR[rb]._u16[h] ? 0xffff : 0;
	}
	virtual void ORC(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._u32[w] | (~CPU.GPR[rb]._u32[w]);
	}
	virtual void FCMGT(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		// TODO
	}
	virtual void DFCMGT(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		// TODO
	}
	virtual void DFA(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		// TODO
	}
	virtual void DFS(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		// TODO
	}
	virtual void DFM(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		// TODO
	}
	virtual void CLGTB(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b] > CPU.GPR[rb]._u8[b] ? 0xff : 0;
	}
	virtual void HLGT(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		// TODO
	}
	virtual void DFMA(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		// TODO
	}
	virtual void DFMS(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		// TODO
	}
	virtual void DFNMS(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		// TODO
	}
	virtual void DFNMA(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		// TODO
	}
	virtual void CEQ(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._i32[w] == CPU.GPR[rb]._i32[w] ? 0xffffffff : 0;
	}
	virtual void MPYHHU(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._u16[w*2] * CPU.GPR[rb]._u16[w*2];
	}
	virtual void ADDX(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._u32[w] + CPU.GPR[rb]._u32[w] + (CPU.GPR[rt]._u32[w] & 1);
	}
	virtual void SFX(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[rb]._u32[w] - CPU.GPR[ra]._u32[w] - (1 - (CPU.GPR[rt]._u32[w] & 1));
	}
	virtual void CGX(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = (CPU.GPR[ra]._u32[w] + CPU.GPR[rb]._u32[w] + (CPU.GPR[rt]._u32[w] & 1)) < CPU.GPR[ra]._u32[w] ? 1 : 0;
	}
	virtual void BGX(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		s64 nResult;
		
		for (int w = 0; w < 4; w++)
		{
			nResult = (u64)CPU.GPR[rb]._u32[w] - (u64)CPU.GPR[ra]._u32[w] - (1 - (CPU.GPR[rt]._u32[w] & 1));
			CPU.GPR[rt]._u32[w] = nResult < 0 ? 0 : 1;
		}
	}
	virtual void MPYHHA(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._i32[w] += CPU.GPR[ra]._i16[w*2] * CPU.GPR[rb]._i16[w*2];
	}
	virtual void MPYHHAU(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] += CPU.GPR[ra]._u16[w*2] * CPU.GPR[rb]._u16[w*2];
	}
	virtual void FSCRRD(OP_REG rt)
	{
		// TODO
	}
	virtual void FESD(OP_REG rt, OP_REG ra)
	{
		// TODO
	}
	virtual void FRDS(OP_REG rt, OP_REG ra)
	{
		// TODO
	}
	virtual void FSCRWR(OP_REG rt, OP_REG ra)
	{
		// TODO
	}
	virtual void DFTSV(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		// TODO
	}
	virtual void FCEQ(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		// TODO
	}
	virtual void DFCEQ(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		// TODO
	}
	virtual void MPY(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._i32[w] = CPU.GPR[ra]._i16[w*2 + 1] * CPU.GPR[rb]._i16[w*2 + 1];
	}
	virtual void MPYH(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._i32[w] = ((CPU.GPR[ra]._i32[w] >> 16) * (CPU.GPR[rb]._i32[w] & 0xffff)) << 16;
	}
	virtual void MPYHH(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._i32[w] = CPU.GPR[ra]._i16[w*2] * CPU.GPR[rb]._i16[w*2];
	}
	virtual void MPYS(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._i32[w] = (CPU.GPR[ra]._i16[w*2 + 1] * CPU.GPR[rb]._i16[w*2 + 1]) >> 16;
	}
	virtual void CEQH(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = CPU.GPR[ra]._u16[h] == CPU.GPR[rb]._u16[h] ? 0xffff : 0;
	}
	virtual void FCMEQ(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		// TODO
	}
	virtual void DFCMEQ(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		// TODO
	}
	virtual void MPYU(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._u16[w*2 + 1] * CPU.GPR[rb]._u16[w*2 + 1];
	}
	virtual void CEQB(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b] == CPU.GPR[rb]._u8[b] ? 0xff : 0;
	}
	virtual void FI(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		// TODO
	}
	virtual void HEQ(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		// TODO
	}



	//0 - 9
	virtual void CFLTS(OP_REG rt, OP_REG ra, OP_sIMM i8)
	{
		// TODO
	}
	virtual void CFLTU(OP_REG rt, OP_REG ra, OP_sIMM i8)
	{
		// TODO
	}
	virtual void CSFLT(OP_REG rt, OP_REG ra, OP_sIMM i8)
	{
		// TODO
	}
	virtual void CUFLT(OP_REG rt, OP_REG ra, OP_sIMM i8)
	{
		// TODO
	}



	//0 - 8
	virtual void BRZ(OP_REG rt, OP_sIMM i16)
	{
		if(!CPU.GPR[rt]._u32[0]) CPU.SetBranch(branchTarget(CPU.PC, i16));
	}
	virtual void STQA(OP_REG rt, OP_sIMM i16)
	{
		CPU.LSA = i16 << 2;
		CPU.WriteLSA128(CPU.GPR[rt]._u128);
	}
	virtual void BRNZ(OP_REG rt, OP_sIMM i16)
	{
		if(CPU.GPR[rt]._u32[0] != 0) 
			CPU.SetBranch(branchTarget(CPU.PC, i16));
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
	virtual void BRA(OP_sIMM i16)
	{
		CPU.SetBranch(i16 << 2);
	}
	virtual void LQA(OP_REG rt, OP_sIMM i16)
	{
		CPU.LSA = i16 << 2;
		if(!Memory.IsGoodAddr(CPU.LSA))
		{
			ConLog.Warning("LQA: Bad addr: 0x%x", CPU.LSA);
			return;
		}

		CPU.GPR[rt]._u128 = CPU.ReadLSA128();
	}
	virtual void BRASL(OP_REG rt, OP_sIMM i16)
	{
		CPU.GPR[rt].Reset();
		CPU.GPR[rt]._u32[0] = CPU.PC + 4;
		CPU.SetBranch(i16 << 2);
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
	virtual void IL(OP_REG rt, OP_sIMM i16)
	{
		CPU.GPR[rt]._u32[0] = i16;
		CPU.GPR[rt]._u32[1] = i16;
		CPU.GPR[rt]._u32[2] = i16;
		CPU.GPR[rt]._u32[3] = i16;
	}
	virtual void ILHU(OP_REG rt, OP_sIMM i16)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u16[w*2] = i16;
	}
	virtual void ILH(OP_REG rt, OP_sIMM i16)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = i16;
	}
	virtual void IOHL(OP_REG rt, OP_sIMM i16)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] |= i16;
	}
	

	//0 - 7
	virtual void ORI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		for(u32 i = 0; i < 4; ++i)
		{
			CPU.GPR[rt]._u32[i] = CPU.GPR[ra]._u32[i] | i10;
		}
	}
	virtual void ORHI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = CPU.GPR[ra]._u16[h] | i10;
	}
	virtual void ORBI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b] | (i10 & 0xff);
	}
	virtual void SFI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._i32[w] = i10 - CPU.GPR[ra]._i32[w];
	}
	virtual void SFHI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._i16[h] = i10 - CPU.GPR[ra]._i16[h];
	}
	virtual void ANDI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._u32[w] & i10;
	}
	virtual void ANDHI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = CPU.GPR[ra]._u16[h] & i10;
	}
	virtual void ANDBI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b] & (i10 & 0xff);
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
	virtual void XORI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._u32[w] ^ i10;
	}
	virtual void XORHI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = CPU.GPR[ra]._u16[h] ^ i10;
	}
	virtual void XORBI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b] ^ (i10 & 0xff);
	}
	virtual void CGTI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._i32[w] > i10 ? 0xffffffff : 0;
	}
	virtual void CGTHI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = CPU.GPR[ra]._i16[h] > i10 ? 0xffff : 0;
	}
	virtual void CGTBI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._i8[b] > (s8)(i10 & 0xff) ? 0xff : 0;
	}
	virtual void HGTI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		// TODO
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
	virtual void CLGTBI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b] > (u8)(i10 & 0xff) ? 0xff : 0;
	}
	virtual void HLGTI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		// TODO
	}
	virtual void MPYI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._i32[w] = CPU.GPR[rt]._i16[w*2 + 1] * i10;
	}
	virtual void MPYUI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[rt]._u16[w*2 + 1] * (u16)(i10 & 0xffff);
	}
	virtual void CEQI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		for(u32 i = 0; i < 4; ++i)
		{
			CPU.GPR[rt]._u32[i] = (CPU.GPR[rt]._u32[i] == (u32)i10) ? 0xffffffff : 0x00000000;
		}
	}
	virtual void CEQHI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = CPU.GPR[ra]._i16[h] == (s16)i10 ? 0xffff : 0;
	}
	virtual void CEQBI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b] == (u8)(i10 & 0xff) ? 0xff : 0;
	}
	virtual void HEQI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		// TODO
	}


	//0 - 6
	virtual void HBRA(OP_sIMM ro, OP_sIMM i16)
	{
		// TODO
	}
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
	virtual void MPYA(OP_REG rc, OP_REG ra, OP_REG rb, OP_REG rt)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._i32[w] = CPU.GPR[ra]._i16[w*2 + 1] * CPU.GPR[rb]._i16[w*2 + 1] + CPU.GPR[rc]._i32[w];
	}
	virtual void FNMS(OP_REG rc, OP_REG ra, OP_REG rb, OP_REG rt)
	{
		// TODO
	}
	virtual void FMA(OP_REG rc, OP_REG ra, OP_REG rb, OP_REG rt)
	{
		// TODO
	}
	virtual void FMS(OP_REG rc, OP_REG ra, OP_REG rb, OP_REG rt)
	{
		// TODO
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