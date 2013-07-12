#pragma once

#include "Emu/Cell/SPUOpcodes.h"
#include "Emu/Memory/Memory.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/SysCalls/SysCalls.h"

#define UNIMPLEMENTED() UNK(__FUNCTION__)

class SPU_Interpreter : public SPU_Opcodes
{
private:
	SPUThread& CPU;

public:
	SPU_Interpreter(SPUThread& cpu) : CPU(cpu)
	{
	}

private:
	void Exit(){}

	void SysCall()
	{
	}

	//0 - 10
	void STOP(u32 code)
	{
		if(code & 0x2000)
		{
			CPU.SetExitStatus(code & 0xfff);
			CPU.Stop();
		}
	}
	void LNOP()
	{
	}
	void SYNC(u32 Cbit)
	{
		//UNIMPLEMENTED();
	}
	void DSYNC()
	{
		//UNIMPLEMENTED();
	}
	void MFSPR(u32 rt, u32 sa)
	{
		UNIMPLEMENTED();
	}
	void RDCH(u32 rt, u32 ra)
	{
		CPU.ReadChannel(CPU.GPR[rt], ra);
	}
	void RCHCNT(u32 rt, u32 ra)
	{
		CPU.GPR[rt].Reset();
		CPU.GPR[rt]._u32[3] = CPU.GetChannelCount(ra);
	}
	void SF(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._u32[0] = CPU.GPR[rb]._u32[0] + ~CPU.GPR[ra]._u32[0] + 1;
		CPU.GPR[rt]._u32[1] = CPU.GPR[rb]._u32[1] + ~CPU.GPR[ra]._u32[1] + 1;
		CPU.GPR[rt]._u32[2] = CPU.GPR[rb]._u32[2] + ~CPU.GPR[ra]._u32[2] + 1;
		CPU.GPR[rt]._u32[3] = CPU.GPR[rb]._u32[3] + ~CPU.GPR[ra]._u32[3] + 1;
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
		CPU.GPR[rt]._u32[0] = ((0 - CPU.GPR[rb]._u32[0]) % 64) < 32 ? CPU.GPR[ra]._u32[0] >> ((0 - CPU.GPR[rb]._u32[0]) % 64) : 0;
		CPU.GPR[rt]._u32[1] = ((0 - CPU.GPR[rb]._u32[1]) % 64) < 32 ? CPU.GPR[ra]._u32[1] >> ((0 - CPU.GPR[rb]._u32[1]) % 64) : 0;
		CPU.GPR[rt]._u32[2] = ((0 - CPU.GPR[rb]._u32[2]) % 64) < 32 ? CPU.GPR[ra]._u32[2] >> ((0 - CPU.GPR[rb]._u32[2]) % 64) : 0;
		CPU.GPR[rt]._u32[3] = ((0 - CPU.GPR[rb]._u32[3]) % 64) < 32 ? CPU.GPR[ra]._u32[3] >> ((0 - CPU.GPR[rb]._u32[3]) % 64) : 0;
	}
	void ROTMA(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._i32[0] = ((0 - CPU.GPR[rb]._i32[0]) % 64) < 32 ? CPU.GPR[ra]._i32[0] >> ((0 - CPU.GPR[rb]._i32[0]) % 64) : CPU.GPR[ra]._i32[0] >> 31;
		CPU.GPR[rt]._i32[1] = ((0 - CPU.GPR[rb]._i32[1]) % 64) < 32 ? CPU.GPR[ra]._i32[1] >> ((0 - CPU.GPR[rb]._i32[1]) % 64) : CPU.GPR[ra]._i32[1] >> 31;
		CPU.GPR[rt]._i32[2] = ((0 - CPU.GPR[rb]._i32[2]) % 64) < 32 ? CPU.GPR[ra]._i32[2] >> ((0 - CPU.GPR[rb]._i32[2]) % 64) : CPU.GPR[ra]._i32[2] >> 31;
		CPU.GPR[rt]._i32[3] = ((0 - CPU.GPR[rb]._i32[3]) % 64) < 32 ? CPU.GPR[ra]._i32[3] >> ((0 - CPU.GPR[rb]._i32[3]) % 64) : CPU.GPR[ra]._i32[3] >> 31;
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
			CPU.GPR[rt]._u16[h] = (CPU.GPR[ra]._u16[h] << (CPU.GPR[rb]._u16[h] & 0xf)) | (CPU.GPR[ra]._u16[h] >> (16 - (CPU.GPR[rb]._u32[h] & 0xf)));
	}
	void ROTHM(u32 rt, u32 ra, u32 rb)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = ((0 - CPU.GPR[rb]._u16[h]) % 32) < 16 ? CPU.GPR[ra]._u16[h] >> ((0 - CPU.GPR[rb]._u16[h]) % 32) : 0;
	}
	void ROTMAH(u32 rt, u32 ra, u32 rb)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._i16[h] = ((0 - CPU.GPR[rb]._i16[h]) % 32) < 16 ? CPU.GPR[ra]._i16[h] >> ((0 - CPU.GPR[rb]._i16[h]) % 32) : CPU.GPR[ra]._i16[h] >> 15;
	}
	void SHLH(u32 rt, u32 ra, u32 rb)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = (CPU.GPR[rb]._u16[h] & 0x1f) > 15 ? 0 : CPU.GPR[ra]._u16[h] << (CPU.GPR[rb]._u16[h] & 0x3f);
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
		const int nRot = (0 - i7) % 64;
		CPU.GPR[rt]._u32[0] = nRot < 32 ? CPU.GPR[ra]._u32[0] >> nRot : 0;
		CPU.GPR[rt]._u32[1] = nRot < 32 ? CPU.GPR[ra]._u32[1] >> nRot : 0;
		CPU.GPR[rt]._u32[2] = nRot < 32 ? CPU.GPR[ra]._u32[2] >> nRot : 0;
		CPU.GPR[rt]._u32[3] = nRot < 32 ? CPU.GPR[ra]._u32[3] >> nRot : 0;
	}
	void ROTMAI(u32 rt, u32 ra, s32 i7)
	{
		const int nRot = (0 - i7) % 64;
		CPU.GPR[rt]._i32[0] = nRot < 32 ? CPU.GPR[ra]._i32[0] >> nRot : CPU.GPR[ra]._i32[0] >> 31;
		CPU.GPR[rt]._i32[1] = nRot < 32 ? CPU.GPR[ra]._i32[1] >> nRot : CPU.GPR[ra]._i32[1] >> 31;
		CPU.GPR[rt]._i32[2] = nRot < 32 ? CPU.GPR[ra]._i32[2] >> nRot : CPU.GPR[ra]._i32[2] >> 31;
		CPU.GPR[rt]._i32[3] = nRot < 32 ? CPU.GPR[ra]._i32[3] >> nRot : CPU.GPR[ra]._i32[3] >> 31;
	}
	void SHLI(u32 rt, u32 ra, s32 i7)
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
	void ROTHI(u32 rt, u32 ra, s32 i7)
	{
		const int nRot = i7 & 0xf;

		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = (CPU.GPR[ra]._u16[h] << nRot) | (CPU.GPR[ra]._u16[h] >> (16 - nRot));
	}
	void ROTHMI(u32 rt, u32 ra, s32 i7)
	{
		const int nRot = (0 - i7) % 32;

		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = nRot < 16 ? CPU.GPR[ra]._u16[h] >> nRot : 0;
	}
	void ROTMAHI(u32 rt, u32 ra, s32 i7)
	{
		const int nRot = (0 - i7) % 32;

		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._i16[h] = nRot < 16 ? CPU.GPR[ra]._i16[h] >> nRot : CPU.GPR[ra]._i16[h] >> 15;
	}
	void SHLHI(u32 rt, u32 ra, s32 i7)
	{
		const int nRot = i7 & 0x1f;

		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[0] = nRot > 15 ? 0 : CPU.GPR[ra]._u16[0] << nRot;
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
		UNIMPLEMENTED();
	}
	void WRCH(u32 ra, u32 rt)
	{
		CPU.WriteChannel(ra, CPU.GPR[rt]);
	}
	void BIZ(u32 rt, u32 ra)
	{
		if(CPU.GPR[rt]._u32[0] == 0)
			CPU.SetBranch(CPU.GPR[ra]._u32[0] & 0xfffffffc);
	}
	void BINZ(u32 rt, u32 ra)
	{
		if(CPU.GPR[rt]._u32[0] != 0)
			CPU.SetBranch(CPU.GPR[ra]._u32[0] & 0xfffffffc);
	}
	void BIHZ(u32 rt, u32 ra)
	{
		if(CPU.GPR[rt]._u16[0] == 0)
			CPU.SetBranch(CPU.GPR[ra]._u32[0] & 0xfffffffc);
	}
	void BIHNZ(u32 rt, u32 ra)
	{
		if(CPU.GPR[rt]._u16[0] != 0)
			CPU.SetBranch(CPU.GPR[ra]._u32[0] & 0xfffffffc);
	}
	void STOPD(u32 rc, u32 ra, u32 rb)
	{
		Emu.Pause();
	}
	void STQX(u32 rt, u32 ra, u32 rb)
	{
		CPU.WriteLS128(CPU.GPR[ra]._u32[0] + CPU.GPR[rb]._u32[0], CPU.GPR[rt]._u128);
	}
	void BI(u32 ra)
	{
		CPU.SetBranch(CPU.GPR[ra]._u32[3] & 0xfffffffc);
	}
	void BISL(u32 rt, u32 ra)
	{
		CPU.SetBranch(CPU.GPR[ra]._u32[3] & 0xfffffffc);
		CPU.GPR[rt].Reset();
		CPU.GPR[rt]._u32[0] = CPU.PC + 4;
	}
	void IRET(u32 ra)
	{
		UNIMPLEMENTED();
		// SetBranch(SRR0);
	}
	void BISLED(u32 rt, u32 ra)
	{
		UNIMPLEMENTED();
	}
	void HBR(u32 p, u32 ro, u32 ra)
	{
		CPU.SetBranch(CPU.GPR[ra]._u32[0]);
	}
	void GB(u32 rt, u32 ra)
	{
		CPU.GPR[rt].Reset();
		CPU.GPR[rt]._u32[0] =	(CPU.GPR[ra]._u32[0] & 1) |
										((CPU.GPR[ra]._u32[1] & 1) << 1) |
										((CPU.GPR[ra]._u32[2] & 1) << 2) |
										((CPU.GPR[ra]._u32[3] & 1) << 3);
	}
	void GBH(u32 rt, u32 ra)
	{
		CPU.GPR[rt].Reset();

		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u32[0] |= (CPU.GPR[ra]._u16[h] & 1) << h;
	}
	void GBB(u32 rt, u32 ra)
	{
		CPU.GPR[rt].Reset();

		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u32[0] |= (CPU.GPR[ra]._u8[b] & 1) << b;
	}
	void FSM(u32 rt, u32 ra)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = (CPU.GPR[ra]._u32[0] & (8 >> w)) ? ~0 : 0;
	}
	void FSMH(u32 rt, u32 ra)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = (CPU.GPR[ra]._u32[0] & (128 >> h)) ? ~0 : 0;
	}
	void FSMB(u32 rt, u32 ra)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = (CPU.GPR[ra]._u32[0] & (32768 >> b)) ? ~0 : 0;
	}
	void FREST(u32 rt, u32 ra)
	{
		UNIMPLEMENTED();
	}
	void FRSQEST(u32 rt, u32 ra)
	{
		UNIMPLEMENTED();
	}
	void LQX(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._u128 = CPU.ReadLS128(CPU.GPR[ra]._u32[0] + CPU.GPR[rb]._u32[0]);
	}
	void ROTQBYBI(u32 rt, u32 ra, u32 rb)
	{
		const int nShift = (CPU.GPR[rb]._u32[0] >> 3) & 0xf;

		for (int b = 0; b < 8; b++)
			CPU.GPR[rt]._u8[b] = nShift == 0 ? CPU.GPR[ra]._u8[b] : (CPU.GPR[ra]._u8[b] << nShift) | (CPU.GPR[ra]._u8[b] >> (16 - nShift));
	}
	void ROTQMBYBI(u32 rt, u32 ra, u32 rb)
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
	void SHLQBYBI(u32 rt, u32 ra, u32 rb)
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
	void CBX(u32 rt, u32 ra, u32 rb)
	{
		int n = (CPU.GPR[rb]._u32[0] + CPU.GPR[ra]._u32[0]) & 0xf;

		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = b == n ? 3 : b | 0x10;
	}
	void CHX(u32 rt, u32 ra, u32 rb)
	{
		int n = ((CPU.GPR[rb]._u32[0] + CPU.GPR[ra]._u32[0]) & 0xf) >> 1;

		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = h == n ? 0x0203 : (h * 2 * 0x0101 + 0x1011);
	}
	void CWX(u32 rt, u32 ra, u32 rb)
	{
		const u32 t = ((CPU.GPR[ra]._u32[0] + CPU.GPR[rb]._u32[0]) & 0xc) / 4;
		for(u32 i=0; i<16; ++i) CPU.GPR[rt]._i8[i] = 0x10 + i;
		CPU.GPR[rt]._u32[t] = 0x10203;
	}
	void CDX(u32 rt, u32 ra, u32 rb)
	{
		int n = ((CPU.GPR[rb]._u32[0] + CPU.GPR[ra]._u32[0]) & 0x8) >> 2;

		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = (w == n) ? 0x00010203 : (w == (n + 1)) ? 0x04050607 : (0x01010101 * (w * 4) + 0x10111213);
	}
	void ROTQBI(u32 rt, u32 ra, u32 rb)
	{
		int nShift = CPU.GPR[rb]._u32[0] & 0x7;

		CPU.GPR[rt]._u32[0] = (CPU.GPR[ra]._u32[0] << nShift) | (CPU.GPR[ra]._u32[1] >> (32 - nShift));
		CPU.GPR[rt]._u32[1] = (CPU.GPR[ra]._u32[1] << nShift) | (CPU.GPR[ra]._u32[2] >> (32 - nShift));
		CPU.GPR[rt]._u32[2] = (CPU.GPR[ra]._u32[2] << nShift) | (CPU.GPR[ra]._u32[3] >> (32 - nShift));
		CPU.GPR[rt]._u32[3] = (CPU.GPR[ra]._u32[3] << nShift) | (CPU.GPR[ra]._u32[0] >> (32 - nShift));
	}
	void ROTQMBI(u32 rt, u32 ra, u32 rb)
	{
		int nShift = (0 - CPU.GPR[rb]._u32[0]) % 8;

		CPU.GPR[rt]._u32[0] = CPU.GPR[ra]._u32[0] >> nShift;
		CPU.GPR[rt]._u32[1] = (CPU.GPR[ra]._u32[1] >> nShift) | (CPU.GPR[ra]._u32[0] << (32 - nShift));
		CPU.GPR[rt]._u32[2] = (CPU.GPR[ra]._u32[2] >> nShift) | (CPU.GPR[ra]._u32[1] << (32 - nShift));
		CPU.GPR[rt]._u32[3] = (CPU.GPR[ra]._u32[3] >> nShift) | (CPU.GPR[ra]._u32[2] << (32 - nShift));
	}
	void SHLQBI(u32 rt, u32 ra, u32 rb)
	{
		const int nShift = CPU.GPR[rb]._u32[0] & 0x7;

		CPU.GPR[rt]._u32[0] = (CPU.GPR[ra]._u32[0] << nShift) | (CPU.GPR[ra]._u32[1] >> (32 - nShift));
		CPU.GPR[rt]._u32[1] = (CPU.GPR[ra]._u32[1] << nShift) | (CPU.GPR[ra]._u32[2] >> (32 - nShift));
		CPU.GPR[rt]._u32[2] = (CPU.GPR[ra]._u32[2] << nShift) | (CPU.GPR[ra]._u32[3] >> (32 - nShift));
		CPU.GPR[rt]._u32[3] = CPU.GPR[ra]._u32[3] << nShift;
	}
	void ROTQBY(u32 rt, u32 ra, u32 rb)
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
	void ROTQMBY(u32 rt, u32 ra, u32 rb)
	{
		const int nShift = (0 - CPU.GPR[rb]._u32[0]) % 32;

		for (int b = 0; b < 16; b++)
			if (b >= nShift)
				CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b - nShift];
			else
				CPU.GPR[rt]._u8[b] = 0;
	}
	void SHLQBY(u32 rt, u32 ra, u32 rb)
	{
		const int nShift = CPU.GPR[rb]._u32[0] & 0x1f;

		for (int b = 0; b < 16; b++)
			if (b + nShift < 16)
				CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b + nShift];
			else
				CPU.GPR[rt]._u8[b] = 0;
	}
	void ORX(u32 rt, u32 ra)
	{
		CPU.GPR[rt].Reset();

		CPU.GPR[rt]._u32[0] = CPU.GPR[ra]._u32[0] | CPU.GPR[ra]._u32[1] | CPU.GPR[ra]._u32[2] | CPU.GPR[ra]._u32[3];
	}
	void CBD(u32 rt, u32 ra, s32 i7)
	{
		const int n = (CPU.GPR[ra]._u32[0] + i7) & 0xf;

		for (int b = 0; b < 16; b++)
			if (b == n)
				CPU.GPR[rt]._u8[b] = 0x3;
			else
				CPU.GPR[rt]._u8[b] = b | 0x10;
	}
	void CHD(u32 rt, u32 ra, s32 i7)
	{
		int n = ((CPU.GPR[ra]._u32[0] + i7) & 0xf) >> 1;

		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = h == n ? 0x0203 : (h * 2 * 0x0101 + 0x1011);
	}
	void CWD(u32 rt, u32 ra, s32 i7)
	{
		const int t = ((CPU.GPR[ra]._u32[0] + i7) & 0xf) >> 2;

		for (int i=0; i<16; ++i) 
			CPU.GPR[rt]._u8[i] = 0x10 + i;

		CPU.GPR[rt]._u32[t] = 0x10203;
	}
	void CDD(u32 rt, u32 ra, s32 i7)
	{
		const int t = ((CPU.GPR[ra]._u32[0] + i7) & 0xf) >> 3;

		for (int i=0; i<16; ++i) 
			CPU.GPR[rt]._u8[i] = 0x10 + i;

		CPU.GPR[rt]._u64[t] = (u64)0x0001020304050607;
	}
	void ROTQBII(u32 rt, u32 ra, s32 i7)
	{
		int nShift = i7 & 0x7;

		CPU.GPR[rt]._u32[0] = (CPU.GPR[ra]._u32[0] << nShift) | (CPU.GPR[ra]._u32[1] >> (32 - nShift));
		CPU.GPR[rt]._u32[1] = (CPU.GPR[ra]._u32[1] << nShift) | (CPU.GPR[ra]._u32[2] >> (32 - nShift));
		CPU.GPR[rt]._u32[2] = (CPU.GPR[ra]._u32[2] << nShift) | (CPU.GPR[ra]._u32[3] >> (32 - nShift));
		CPU.GPR[rt]._u32[3] = (CPU.GPR[ra]._u32[3] << nShift) | (CPU.GPR[ra]._u32[0] >> (32 - nShift));
	}
	void ROTQMBII(u32 rt, u32 ra, s32 i7)
	{
		int nShift = (0 - i7) % 8;

		CPU.GPR[rt]._u32[0] = CPU.GPR[ra]._u32[0] >> nShift;
		CPU.GPR[rt]._u32[1] = (CPU.GPR[ra]._u32[1] >> nShift) | (CPU.GPR[ra]._u32[0] << (32 - nShift));
		CPU.GPR[rt]._u32[2] = (CPU.GPR[ra]._u32[2] >> nShift) | (CPU.GPR[ra]._u32[1] << (32 - nShift));
		CPU.GPR[rt]._u32[3] = (CPU.GPR[ra]._u32[3] >> nShift) | (CPU.GPR[ra]._u32[2] << (32 - nShift));
	}
	void SHLQBII(u32 rt, u32 ra, s32 i7)
	{
		const int nShift = i7 & 0x7;

		CPU.GPR[rt]._u32[0] = (CPU.GPR[ra]._u32[0] << nShift) | (CPU.GPR[ra]._u32[1] >> (32 - nShift));
		CPU.GPR[rt]._u32[1] = (CPU.GPR[ra]._u32[1] << nShift) | (CPU.GPR[ra]._u32[2] >> (32 - nShift));
		CPU.GPR[rt]._u32[2] = (CPU.GPR[ra]._u32[2] << nShift) | (CPU.GPR[ra]._u32[3] >> (32 - nShift));
		CPU.GPR[rt]._u32[3] = CPU.GPR[ra]._u32[3] << nShift;
	}
	void ROTQBYI(u32 rt, u32 ra, s32 i7)
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
	void ROTQMBYI(u32 rt, u32 ra, s32 i7)
	{
		const int nShift = (0 - i7) % 32;

		for (int b = 0; b < 16; b++)
			if (b >= nShift)
				CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b - nShift];
			else
				CPU.GPR[rt]._u8[b] = 0;
	}
	void SHLQBYI(u32 rt, u32 ra, s32 i7)
	{
		const u16 s = i7 & 0x1f;

		CPU.GPR[rt].Reset();

		for(u32 b = 0; b + s < 16; ++b)
		{
			CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b + s];
		}
	}
	void NOP(u32 rt)
	{
	}
	void CGT(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._i32[w] > CPU.GPR[rb]._i32[w] ? 0xffffffff : 0;
	}
	void XOR(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._u32[w] ^ CPU.GPR[rb]._u32[w];
	}
	void CGTH(u32 rt, u32 ra, u32 rb)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = CPU.GPR[ra]._i16[h] > CPU.GPR[rb]._i16[h] ? 0xffff : 0;
	}
	void EQV(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = (CPU.GPR[ra]._u32[w] & CPU.GPR[rb]._u32[w]) | ~(CPU.GPR[ra]._u32[w] | CPU.GPR[rb]._u32[w]);
	}
	void CGTB(u32 rt, u32 ra, u32 rb)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._i8[b] > CPU.GPR[rb]._i8[b] ? 0xff : 0;
	}
	void SUMB(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
		{
			CPU.GPR[rt]._u16[w*2] = CPU.GPR[ra]._u8[w*4] + CPU.GPR[ra]._u8[w*4 + 1] + CPU.GPR[ra]._u8[w*4 + 2] + CPU.GPR[ra]._u8[w*4 + 3];
			CPU.GPR[rt]._u16[w*2 + 1] = CPU.GPR[rb]._u8[w*4] + CPU.GPR[rb]._u8[w*4 + 1] + CPU.GPR[rb]._u8[w*4 + 2] + CPU.GPR[rb]._u8[w*4 + 3];
		}
	}
	void HGT(u32 rt, u32 ra, u32 rb)
	{
		UNIMPLEMENTED();
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
		CPU.GPR[rt]._i64[0] = (s64)CPU.GPR[ra]._i32[1];
		CPU.GPR[rt]._i64[1] = (s64)CPU.GPR[ra]._i32[3];
	}
	void XSHW(u32 rt, u32 ra)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._i32[w] = (s32)CPU.GPR[ra]._i16[w*2 + 1];
	}
	void CNTB(u32 rt, u32 ra)
	{
		CPU.GPR[rt].Reset();
		
		for (int b = 0; b < 16; b++)
			for (int i = 0; i < 8; i++)
				CPU.GPR[rt]._u8[b] += (CPU.GPR[ra]._u8[b] & (1 << i)) ? 1 : 0;
	}
	void XSBH(u32 rt, u32 ra)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._i16[h] = (s16)CPU.GPR[ra]._i8[h*2 + 1];
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
		UNIMPLEMENTED();
	}
	void DFCGT(u32 rt, u32 ra, u32 rb)
	{
		UNIMPLEMENTED();
	}
	void FA(u32 rt, u32 ra, u32 rb)
	{
		UNIMPLEMENTED();
	}
	void FS(u32 rt, u32 ra, u32 rb)
	{
		UNIMPLEMENTED();
	}
	void FM(u32 rt, u32 ra, u32 rb)
	{
		UNIMPLEMENTED();
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
		UNIMPLEMENTED();
	}
	void DFCMGT(u32 rt, u32 ra, u32 rb)
	{
		UNIMPLEMENTED();
	}
	void DFA(u32 rt, u32 ra, u32 rb)
	{
		UNIMPLEMENTED();
	}
	void DFS(u32 rt, u32 ra, u32 rb)
	{
		UNIMPLEMENTED();
	}
	void DFM(u32 rt, u32 ra, u32 rb)
	{
		UNIMPLEMENTED();
	}
	void CLGTB(u32 rt, u32 ra, u32 rb)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b] > CPU.GPR[rb]._u8[b] ? 0xff : 0;
	}
	void HLGT(u32 rt, u32 ra, u32 rb)
	{
		UNIMPLEMENTED();
	}
	void DFMA(u32 rt, u32 ra, u32 rb)
	{
		UNIMPLEMENTED();
	}
	void DFMS(u32 rt, u32 ra, u32 rb)
	{
		UNIMPLEMENTED();
	}
	void DFNMS(u32 rt, u32 ra, u32 rb)
	{
		UNIMPLEMENTED();
	}
	void DFNMA(u32 rt, u32 ra, u32 rb)
	{
		UNIMPLEMENTED();
	}
	void CEQ(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._i32[w] == CPU.GPR[rb]._i32[w] ? 0xffffffff : 0;
	}
	void MPYHHU(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._u16[w*2] * CPU.GPR[rb]._u16[w*2];
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
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = (CPU.GPR[ra]._u32[w] + CPU.GPR[rb]._u32[w] + (CPU.GPR[rt]._u32[w] & 1)) < CPU.GPR[ra]._u32[w] ? 1 : 0;
	}
	void BGX(u32 rt, u32 ra, u32 rb)
	{
		s64 nResult;
		
		for (int w = 0; w < 4; w++)
		{
			nResult = (u64)CPU.GPR[rb]._u32[w] - (u64)CPU.GPR[ra]._u32[w] - (1 - (CPU.GPR[rt]._u32[w] & 1));
			CPU.GPR[rt]._u32[w] = nResult < 0 ? 0 : 1;
		}
	}
	void MPYHHA(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._i32[w] += CPU.GPR[ra]._i16[w*2] * CPU.GPR[rb]._i16[w*2];
	}
	void MPYHHAU(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] += CPU.GPR[ra]._u16[w*2] * CPU.GPR[rb]._u16[w*2];
	}
	void FSCRRD(u32 rt)
	{
		UNIMPLEMENTED();
	}
	void FESD(u32 rt, u32 ra)
	{
		UNIMPLEMENTED();
	}
	void FRDS(u32 rt, u32 ra)
	{
		UNIMPLEMENTED();
	}
	void FSCRWR(u32 rt, u32 ra)
	{
		UNIMPLEMENTED();
	}
	void DFTSV(u32 rt, u32 ra, s32 i7)
	{
		UNIMPLEMENTED();
	}
	void FCEQ(u32 rt, u32 ra, u32 rb)
	{
		UNIMPLEMENTED();
	}
	void DFCEQ(u32 rt, u32 ra, u32 rb)
	{
		UNIMPLEMENTED();
	}
	void MPY(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._i32[w] = CPU.GPR[ra]._i16[w*2 + 1] * CPU.GPR[rb]._i16[w*2 + 1];
	}
	void MPYH(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._i32[w] = ((CPU.GPR[ra]._i32[w] >> 16) * (CPU.GPR[rb]._i32[w] & 0xffff)) << 16;
	}
	void MPYHH(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._i32[w] = CPU.GPR[ra]._i16[w*2] * CPU.GPR[rb]._i16[w*2];
	}
	void MPYS(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._i32[w] = (CPU.GPR[ra]._i16[w*2 + 1] * CPU.GPR[rb]._i16[w*2 + 1]) >> 16;
	}
	void CEQH(u32 rt, u32 ra, u32 rb)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = CPU.GPR[ra]._u16[h] == CPU.GPR[rb]._u16[h] ? 0xffff : 0;
	}
	void FCMEQ(u32 rt, u32 ra, u32 rb)
	{
		UNIMPLEMENTED();
	}
	void DFCMEQ(u32 rt, u32 ra, u32 rb)
	{
		UNIMPLEMENTED();
	}
	void MPYU(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._u16[w*2 + 1] * CPU.GPR[rb]._u16[w*2 + 1];
	}
	void CEQB(u32 rt, u32 ra, u32 rb)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b] == CPU.GPR[rb]._u8[b] ? 0xff : 0;
	}
	void FI(u32 rt, u32 ra, u32 rb)
	{
		UNIMPLEMENTED();
	}
	void HEQ(u32 rt, u32 ra, u32 rb)
	{
		UNIMPLEMENTED();
	}

	//0 - 9
	void CFLTS(u32 rt, u32 ra, s32 i8)
	{
		UNIMPLEMENTED();
	}
	void CFLTU(u32 rt, u32 ra, s32 i8)
	{
		UNIMPLEMENTED();
	}
	void CSFLT(u32 rt, u32 ra, s32 i8)
	{
		UNIMPLEMENTED();
	}
	void CUFLT(u32 rt, u32 ra, s32 i8)
	{
		UNIMPLEMENTED();
	}

	//0 - 8
	void BRZ(u32 rt, s32 i16)
	{
		if(!CPU.GPR[rt]._u32[3]) CPU.SetBranch(branchTarget(CPU.PC, i16));
	}
	void STQA(u32 rt, s32 i16)
	{
		CPU.WriteLS128(i16 << 2, CPU.GPR[rt]._u128);
	}
	void BRNZ(u32 rt, s32 i16)
	{
		if(CPU.GPR[rt]._u32[3] != 0) 
			CPU.SetBranch(branchTarget(CPU.PC, i16));
	}
	void BRHZ(u32 rt, s32 i16)
	{
		if(!CPU.GPR[rt]._u16[7]) CPU.SetBranch(branchTarget(CPU.PC, i16));
	}
	void BRHNZ(u32 rt, s32 i16)
	{
		if(CPU.GPR[rt]._u16[7]) CPU.SetBranch(branchTarget(CPU.PC, i16));
	}
	void STQR(u32 rt, s32 i16)
	{
		CPU.WriteLS128(branchTarget(CPU.PC, i16), CPU.GPR[rt]._u128);
	}
	void BRA(s32 i16)
	{
		CPU.SetBranch(i16 << 2);
	}
	void LQA(u32 rt, s32 i16)
	{
		CPU.GPR[rt]._u128 = CPU.ReadLS128(i16 << 2);
	}
	void BRASL(u32 rt, s32 i16)
	{
		CPU.GPR[rt].Reset();
		CPU.GPR[rt]._u32[0] = CPU.PC + 4;
		CPU.SetBranch(i16 << 2);
	}
	void BR(s32 i16)
	{
		CPU.SetBranch(branchTarget(CPU.PC, i16));
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
		CPU.GPR[rt].Reset();
		CPU.GPR[rt]._u32[3] = CPU.PC + 4;
		CPU.SetBranch(branchTarget(CPU.PC, i16));
	}
	void LQR(u32 rt, s32 i16)
	{
		CPU.GPR[rt]._u128 = CPU.ReadLS128(branchTarget(CPU.PC, i16));
	}
	void IL(u32 rt, s32 i16)
	{
		CPU.GPR[rt]._u32[0] = i16;
		CPU.GPR[rt]._u32[1] = i16;
		CPU.GPR[rt]._u32[2] = i16;
		CPU.GPR[rt]._u32[3] = i16;
	}
	void ILHU(u32 rt, s32 i16)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u16[w*2 + 1] = i16;
	}
	void ILH(u32 rt, s32 i16)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = i16;
	}
	void IOHL(u32 rt, s32 i16)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] |= i16;
	}
	

	//0 - 7
	void ORI(u32 rt, u32 ra, s32 i10)
	{
		for(u32 i = 0; i < 4; ++i)
		{
			CPU.GPR[rt]._u32[i] = CPU.GPR[ra]._u32[i] | i10;
		}
	}
	void ORHI(u32 rt, u32 ra, s32 i10)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = CPU.GPR[ra]._u16[h] | i10;
	}
	void ORBI(u32 rt, u32 ra, s32 i10)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b] | (i10 & 0xff);
	}
	void SFI(u32 rt, u32 ra, s32 i10)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._i32[w] = i10 - CPU.GPR[ra]._i32[w];
	}
	void SFHI(u32 rt, u32 ra, s32 i10)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._i16[h] = i10 - CPU.GPR[ra]._i16[h];
	}
	void ANDI(u32 rt, u32 ra, s32 i10)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._u32[w] & i10;
	}
	void ANDHI(u32 rt, u32 ra, s32 i10)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = CPU.GPR[ra]._u16[h] & i10;
	}
	void ANDBI(u32 rt, u32 ra, s32 i10)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b] & (i10 & 0xff);
	}
	void AI(u32 rt, u32 ra, s32 i10)
	{
		for(u32 i = 0; i < 4; ++i)
		{
			CPU.GPR[rt]._u32[i] = CPU.GPR[ra]._u32[i] + i10;
		}
	}
	void AHI(u32 rt, u32 ra, s32 i10)
	{
		for(u32 i = 0; i < 8; ++i)
		{
			CPU.GPR[rt]._u16[i] = CPU.GPR[ra]._u16[i] + i10;
		}
	}
	void STQD(u32 rt, s32 i10, u32 ra)
	{
		CPU.WriteLS128(CPU.GPR[ra]._u32[3] + i10, CPU.GPR[rt]._u128);
	}
	void LQD(u32 rt, s32 i10, u32 ra)
	{
		CPU.GPR[rt]._u128 = CPU.ReadLS128(CPU.GPR[ra]._u32[3] + i10);
	}
	void XORI(u32 rt, u32 ra, s32 i10)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._u32[w] ^ i10;
	}
	void XORHI(u32 rt, u32 ra, s32 i10)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = CPU.GPR[ra]._u16[h] ^ i10;
	}
	void XORBI(u32 rt, u32 ra, s32 i10)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b] ^ (i10 & 0xff);
	}
	void CGTI(u32 rt, u32 ra, s32 i10)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._i32[w] > i10 ? 0xffffffff : 0;
	}
	void CGTHI(u32 rt, u32 ra, s32 i10)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = CPU.GPR[ra]._i16[h] > i10 ? 0xffff : 0;
	}
	void CGTBI(u32 rt, u32 ra, s32 i10)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._i8[b] > (s8)(i10 & 0xff) ? 0xff : 0;
	}
	void HGTI(u32 rt, u32 ra, s32 i10)
	{
		UNIMPLEMENTED();
	}
	void CLGTI(u32 rt, u32 ra, s32 i10)
	{
		for(u32 i = 0; i < 4; ++i)
		{
			CPU.GPR[rt]._u32[i] = (CPU.GPR[rt]._u32[i] > (u32)i10) ? 0xffffffff : 0x00000000;
		}
	}
	void CLGTHI(u32 rt, u32 ra, s32 i10)
	{
		for(u32 i = 0; i < 8; ++i)
		{
			CPU.GPR[rt]._u16[i] = (CPU.GPR[rt]._u16[i] > (u16)i10) ? 0xffff : 0x0000;
		}
	}
	void CLGTBI(u32 rt, u32 ra, s32 i10)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b] > (u8)(i10 & 0xff) ? 0xff : 0;
	}
	void HLGTI(u32 rt, u32 ra, s32 i10)
	{
		UNIMPLEMENTED();
	}
	void MPYI(u32 rt, u32 ra, s32 i10)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._i32[w] = CPU.GPR[rt]._i16[w*2 + 1] * i10;
	}
	void MPYUI(u32 rt, u32 ra, s32 i10)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[rt]._u16[w*2 + 1] * (u16)(i10 & 0xffff);
	}
	void CEQI(u32 rt, u32 ra, s32 i10)
	{
		for(u32 i = 0; i < 4; ++i)
		{
			CPU.GPR[rt]._u32[i] = (CPU.GPR[rt]._u32[i] == (u32)i10) ? 0xffffffff : 0x00000000;
		}
	}
	void CEQHI(u32 rt, u32 ra, s32 i10)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = CPU.GPR[ra]._i16[h] == (s16)i10 ? 0xffff : 0;
	}
	void CEQBI(u32 rt, u32 ra, s32 i10)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b] == (u8)(i10 & 0xff) ? 0xff : 0;
	}
	void HEQI(u32 rt, u32 ra, s32 i10)
	{
		UNIMPLEMENTED();
	}


	//0 - 6
	void HBRA(s32 ro, s32 i16)
	{
		UNIMPLEMENTED();
		UNIMPLEMENTED();
	}
	void HBRR(s32 ro, s32 i16)
	{
	}
	void ILA(u32 rt, s32 i18)
	{
		CPU.GPR[rt]._u32[0] = i18;
		CPU.GPR[rt]._u32[1] = i18;
		CPU.GPR[rt]._u32[2] = i18;
		CPU.GPR[rt]._u32[3] = i18;
	}

	//0 - 3
	void SELB(u32 rt, u32 ra, u32 rb, u32 rc)
	{
		for(u32 i = 0; i < 4; ++i)
		{
			CPU.GPR[rt]._u32[i] =
				( CPU.GPR[rc]._u32[i] & CPU.GPR[rb]._u32[i]) |
				(~CPU.GPR[rc]._u32[i] & CPU.GPR[ra]._u32[i]);
		}
	}
	void SHUFB(u32 rc, u32 ra, u32 rb, u32 rt)
	{
		ConLog.Warning("SHUFB");
	}
	void MPYA(u32 rc, u32 ra, u32 rb, u32 rt)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._i32[w] = CPU.GPR[ra]._i16[w*2 + 1] * CPU.GPR[rb]._i16[w*2 + 1] + CPU.GPR[rc]._i32[w];
	}
	void FNMS(u32 rc, u32 ra, u32 rb, u32 rt)
	{
		UNIMPLEMENTED();
	}
	void FMA(u32 rc, u32 ra, u32 rb, u32 rt)
	{
		UNIMPLEMENTED();
	}
	void FMS(u32 rc, u32 ra, u32 rb, u32 rt)
	{
		UNIMPLEMENTED();
	}

	void UNK(u32 code, u32 opcode, u32 gcode)
	{
		UNK(wxString::Format("Unknown/Illegal opcode! (0x%08x, 0x%x, 0x%x)", code, opcode, gcode));
	}

	void UNK(const wxString& err)
	{
		ConLog.Error(err + wxString::Format(" #pc: 0x%x", CPU.PC));
		Emu.Pause();
		for(uint i=0; i<128; ++i) ConLog.Write("r%d = 0x%s", i, CPU.GPR[i].ToString());
	}
};
