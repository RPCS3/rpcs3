#pragma once

#include "Emu/Cell/SPUOpcodes.h"
#include "Emu/Memory/Memory.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/SysCalls/SysCalls.h"

#define UNIMPLEMENTED() UNK(__FUNCTION__)

/* typedef union _CRT_ALIGN(16) __u32x4 {
	u32 _u32[4];
	__m128i m128i;
	__m128 m128;
	__m128d m128d;
 } __u32x4; */

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
		CPU.SetExitStatus(code); // exit code (not status)

		switch (code)
		{
		case 0x110: /* ===== sys_spu_thread_receive_event ===== */
			{
				u32 spuq = 0;
				if (!CPU.SPU.Out_MBox.Pop(spuq))
				{
					ConLog.Error("sys_spu_thread_receive_event: cannot read Out_MBox");
					CPU.SPU.In_MBox.PushUncond(CELL_EINVAL); // ???
					return;
				}

				if (CPU.SPU.In_MBox.GetCount())
				{
					ConLog.Error("sys_spu_thread_receive_event(spuq=0x%x): In_MBox is not empty", spuq);
					CPU.SPU.In_MBox.PushUncond(CELL_EBUSY); // ???
					return;
				}

				if (Ini.HLELogging.GetValue())
				{
					ConLog.Write("sys_spu_thread_receive_event(spuq=0x%x)", spuq);
				}

				EventQueue* eq;
				if (!CPU.SPUQs.GetEventQueue(FIX_SPUQ(spuq), eq))
				{
					CPU.SPU.In_MBox.PushUncond(CELL_EINVAL); // TODO: check error value
					return;
				}

				u32 tid = GetCurrentSPUThread().GetId();

				eq->sq.push(tid); // add thread to sleep queue

				while (true)
				{
					switch (eq->owner.trylock(tid))
					{
					case SMR_OK:
						if (!eq->events.count())
						{
							eq->owner.unlock(tid);
							break;
						}
						else
						{
							u32 next = (eq->protocol == SYS_SYNC_FIFO) ? eq->sq.pop() : eq->sq.pop_prio();
							if (next != tid)
							{
								eq->owner.unlock(tid, next);
								break;
							}
						}
					case SMR_SIGNAL:
						{
							sys_event_data event;
							eq->events.pop(event);
							eq->owner.unlock(tid);
							CPU.SPU.In_MBox.PushUncond(CELL_OK);
							CPU.SPU.In_MBox.PushUncond(event.data1);
							CPU.SPU.In_MBox.PushUncond(event.data2);
							CPU.SPU.In_MBox.PushUncond(event.data3);
							return;
						}
					case SMR_FAILED: break;
					default: eq->sq.invalidate(tid); CPU.SPU.In_MBox.PushUncond(CELL_ECANCELED); return;
					}

					Sleep(1);
					if (Emu.IsStopped())
					{
						ConLog.Warning("sys_spu_thread_receive_event(spuq=0x%x) aborted", spuq);
						eq->sq.invalidate(tid);
						return;
					}
				}
			}
			break;
		case 0x102:
			if (!CPU.SPU.Out_MBox.GetCount())
			{
				ConLog.Error("sys_spu_thread_exit (no status, code 0x102)");
			}
			else if (Ini.HLELogging.GetValue())
			{
				// the real exit status
				ConLog.Write("sys_spu_thread_exit (status=0x%x)", CPU.SPU.Out_MBox.GetValue());
			}
			CPU.Stop();
			break;
		default:
			if (!CPU.SPU.Out_MBox.GetCount())
			{
				ConLog.Error("Unknown STOP code: 0x%x (no message)", code);
			}
			else
			{
				ConLog.Error("Unknown STOP code: 0x%x (message=0x%x)", code, CPU.SPU.Out_MBox.GetValue());
			}
			CPU.Stop();
			break;
		}
	}
	void LNOP()
	{
	}
	void SYNC(u32 Cbit)
	{
		_mm_mfence();
	}
	void DSYNC()
	{
		_mm_mfence();
	}
	void MFSPR(u32 rt, u32 sa)
	{
		//If register is a dummy register (register labeled 0x0)
		if(sa == 0x0)
		{
			CPU.GPR[rt]._u128.hi = 0x0;
			CPU.GPR[rt]._u128.lo = 0x0;
		}
		else
		{
			CPU.GPR[rt]._u128.hi =  CPU.SPR[sa]._u128.hi;
			CPU.GPR[rt]._u128.lo =  CPU.SPR[sa]._u128.lo;
		}
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
			CPU.GPR[rt]._u16[h] = (CPU.GPR[ra]._u16[h] << (CPU.GPR[rb]._u16[h] & 0xf)) | (CPU.GPR[ra]._u16[h] >> (16 - (CPU.GPR[rb]._u16[h] & 0xf)));
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

		for (u32 j = 0; j < 4; ++j)
			CPU.GPR[rt]._u32[j] = CPU.GPR[ra]._u32[j] << s;
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
		if(sa != 0)
		{
			CPU.SPR[sa]._u128.hi = CPU.GPR[rt]._u128.hi;
			CPU.SPR[sa]._u128.lo = CPU.GPR[rt]._u128.lo;
		}
	}
	void WRCH(u32 ra, u32 rt)
	{
		CPU.WriteChannel(ra, CPU.GPR[rt]);
	}
	void BIZ(u32 rt, u32 ra)
	{
		if(CPU.GPR[rt]._u32[3] == 0)
			CPU.SetBranch(branchTarget(CPU.GPR[ra]._u32[3], 0));
	}
	void BINZ(u32 rt, u32 ra)
	{
		if(CPU.GPR[rt]._u32[3] != 0)
			CPU.SetBranch(branchTarget(CPU.GPR[ra]._u32[3], 0));
	}
	void BIHZ(u32 rt, u32 ra)
	{
		if(CPU.GPR[rt]._u16[6] == 0)
			CPU.SetBranch(branchTarget(CPU.GPR[ra]._u32[3], 0));
	}
	void BIHNZ(u32 rt, u32 ra)
	{
		if(CPU.GPR[rt]._u16[6] != 0)
			CPU.SetBranch(branchTarget(CPU.GPR[ra]._u32[3], 0));
	}
	void STOPD(u32 rc, u32 ra, u32 rb)
	{
		Emu.Pause();
	}
	void STQX(u32 rt, u32 ra, u32 rb)
	{
		u32 lsa = (CPU.GPR[ra]._u32[3] + CPU.GPR[rb]._u32[3]) & 0x3fff0;
		if(!CPU.IsGoodLSA(lsa))
		{
			ConLog.Error("STQX: bad lsa (0x%x)", lsa);
			Emu.Pause();
			return;
		}

		CPU.WriteLS128(lsa, CPU.GPR[rt]._u128);
	}
	void BI(u32 ra)
	{
		CPU.SetBranch(branchTarget(CPU.GPR[ra]._u32[3], 0));
	}
	void BISL(u32 rt, u32 ra)
	{
		const u32 NewPC = CPU.GPR[ra]._u32[3];
		CPU.GPR[rt].Reset();
		CPU.GPR[rt]._u32[3] = CPU.PC + 4;		
		CPU.SetBranch(branchTarget(NewPC, 0));
	}
	void IRET(u32 ra)
	{
		UNIMPLEMENTED();
		//SetBranch(SRR0);
	}
	void BISLED(u32 rt, u32 ra)
	{
		UNIMPLEMENTED();
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
		//CPU.GPR[rt]._m128 = _mm_rcp_ps(CPU.GPR[ra]._m128);
		for (int i = 0; i < 4; i++)
			CPU.GPR[rt]._f[i] = 1 / CPU.GPR[ra]._f[i];
	}
	void FRSQEST(u32 rt, u32 ra)
	{
		//const __u32x4 FloatAbsMask = {0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff};
		//CPU.GPR[rt]._m128 = _mm_rsqrt_ps(_mm_and_ps(CPU.GPR[ra]._m128, FloatAbsMask.m128));
		for (int i = 0; i < 4; i++)
			CPU.GPR[rt]._f[i] = 1 / sqrt(abs(CPU.GPR[ra]._f[i]));
	}
	void LQX(u32 rt, u32 ra, u32 rb)
	{
		u32 a = CPU.GPR[ra]._u32[3], b = CPU.GPR[rb]._u32[3];

		u32 lsa = (a + b) & 0x3fff0;

		if(!CPU.IsGoodLSA(lsa))
		{
			ConLog.Error("LQX: bad lsa (0x%x)", lsa);
			Emu.Pause();
			return;
		}

		CPU.GPR[rt]._u128 = CPU.ReadLS128(lsa);
	}
	void ROTQBYBI(u32 rt, u32 ra, u32 rb)
	{
		const int s = (CPU.GPR[rb]._u32[3] >> 3) & 0xf;
		const SPU_GPR_hdr temp = CPU.GPR[ra];
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = temp._u8[(b - s) & 0xf];
	}
	void ROTQMBYBI(u32 rt, u32 ra, u32 rb)
	{
		const int s = (0 - (CPU.GPR[rb]._u32[3] >> 3)) & 0x1f;
		const SPU_GPR_hdr temp = CPU.GPR[ra];
		CPU.GPR[rt].Reset();
		for (int b = 0; b < 16 - s; b++)
			CPU.GPR[rt]._u8[b] = temp._u8[b + s];
	}
	void SHLQBYBI(u32 rt, u32 ra, u32 rb)
	{
		const int s = (CPU.GPR[rb]._u32[3] >> 3) & 0x1f;
		const SPU_GPR_hdr temp = CPU.GPR[ra];
		CPU.GPR[rt].Reset();
		for (int b = s; b < 16; b++)
			CPU.GPR[rt]._u8[b] = temp._u8[b - s];
	}
	void CBX(u32 rt, u32 ra, u32 rb)
	{
		const u32 t = (CPU.GPR[rb]._u32[3] + CPU.GPR[ra]._u32[3]) & 0xF;

		CPU.GPR[rt]._u64[0] = (u64)0x18191A1B1C1D1E1F;
		CPU.GPR[rt]._u64[1] = (u64)0x1011121314151617;
		CPU.GPR[rt]._u8[15 - t] = 0x03;
	}
	void CHX(u32 rt, u32 ra, u32 rb)
	{
		const u32 t = (CPU.GPR[rb]._u32[3] + CPU.GPR[ra]._u32[3]) & 0xE;

		CPU.GPR[rt]._u64[0] = (u64)0x18191A1B1C1D1E1F;
		CPU.GPR[rt]._u64[1] = (u64)0x1011121314151617;
		CPU.GPR[rt]._u16[7 - (t >> 1)] = 0x0203;
	}
	void CWX(u32 rt, u32 ra, u32 rb)
	{
		const u32 t = (CPU.GPR[ra]._u32[3] + CPU.GPR[rb]._u32[3]) & 0xC;
		
		CPU.GPR[rt]._u64[0] = (u64)0x18191A1B1C1D1E1F;
		CPU.GPR[rt]._u64[1] = (u64)0x1011121314151617;
		CPU.GPR[rt]._u32[3 - (t >> 2)] = 0x00010203;
	}
	void CDX(u32 rt, u32 ra, u32 rb)
	{
		const u32 t = (CPU.GPR[rb]._u32[3] + CPU.GPR[ra]._u32[3]) & 0x8;

		CPU.GPR[rt]._u64[0] = (u64)0x18191A1B1C1D1E1F;
		CPU.GPR[rt]._u64[1] = (u64)0x1011121314151617;
		CPU.GPR[rt]._u64[1 - (t >> 3)] = (u64)0x0001020304050607;
	}
	void ROTQBI(u32 rt, u32 ra, u32 rb)
	{
		const int t = CPU.GPR[rb]._u32[3] & 0x7;
		const SPU_GPR_hdr temp = CPU.GPR[ra];
		CPU.GPR[rt]._u32[0] = (temp._u32[0] << t) | (temp._u32[3] >> (32 - t));
		CPU.GPR[rt]._u32[1] = (temp._u32[1] << t) | (temp._u32[0] >> (32 - t));
		CPU.GPR[rt]._u32[2] = (temp._u32[2] << t) | (temp._u32[1] >> (32 - t));
		CPU.GPR[rt]._u32[3] = (temp._u32[3] << t) | (temp._u32[2] >> (32 - t));
	}
	void ROTQMBI(u32 rt, u32 ra, u32 rb)
	{
		const int t = (0 - CPU.GPR[rb]._u32[3]) & 0x7;
		const SPU_GPR_hdr temp = CPU.GPR[ra];
		CPU.GPR[rt]._u32[0] = (temp._u32[0] >> t) | (temp._u32[1] << (32 - t));
		CPU.GPR[rt]._u32[1] = (temp._u32[1] >> t) | (temp._u32[2] << (32 - t));
		CPU.GPR[rt]._u32[2] = (temp._u32[2] >> t) | (temp._u32[3] << (32 - t));
		CPU.GPR[rt]._u32[3] = (temp._u32[3] >> t);
	}
	void SHLQBI(u32 rt, u32 ra, u32 rb)
	{
		const int t = CPU.GPR[rb]._u32[3] & 0x7;
		const SPU_GPR_hdr temp = CPU.GPR[ra];
		CPU.GPR[rt]._u32[0] = (temp._u32[0] << t);
		CPU.GPR[rt]._u32[1] = (temp._u32[1] << t) | (temp._u32[0] >> (32 - t));
		CPU.GPR[rt]._u32[2] = (temp._u32[2] << t) | (temp._u32[1] >> (32 - t));
		CPU.GPR[rt]._u32[3] = (temp._u32[3] << t) | (temp._u32[2] >> (32 - t));
	}
	void ROTQBY(u32 rt, u32 ra, u32 rb)
	{
		const int s = CPU.GPR[rb]._u32[3] & 0xf;
		const SPU_GPR_hdr temp = CPU.GPR[ra];
		for (int b = 0; b < 16; ++b)
			CPU.GPR[rt]._u8[b] = temp._u8[(b - s) & 0xf];
	}
	void ROTQMBY(u32 rt, u32 ra, u32 rb)
	{
		const int s = (0 - CPU.GPR[rb]._u32[3]) & 0x1f;
		const SPU_GPR_hdr temp = CPU.GPR[ra];
		CPU.GPR[rt].Reset();
		for (int b = 0; b < 16 - s; b++)
			CPU.GPR[rt]._u8[b] = temp._u8[b + s];
	}
	void SHLQBY(u32 rt, u32 ra, u32 rb)
	{
		const int s = CPU.GPR[rb]._u32[3] & 0x1f;
		const SPU_GPR_hdr temp = CPU.GPR[ra];
		CPU.GPR[rt].Reset();
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

		CPU.GPR[rt]._u64[0] = (u64)0x18191A1B1C1D1E1F;
		CPU.GPR[rt]._u64[1] = (u64)0x1011121314151617;
		CPU.GPR[rt]._u8[15 - t] = 0x03;
	}
	void CHD(u32 rt, u32 ra, s32 i7)
	{
		const int t = (CPU.GPR[ra]._u32[3] + i7) & 0xE;

		CPU.GPR[rt]._u64[0] = (u64)0x18191A1B1C1D1E1F;
		CPU.GPR[rt]._u64[1] = (u64)0x1011121314151617;
		CPU.GPR[rt]._u16[7 - (t >> 1)] = 0x0203;
	}
	void CWD(u32 rt, u32 ra, s32 i7)
	{
		const int t = (CPU.GPR[ra]._u32[3] + i7) & 0xC;

		CPU.GPR[rt]._u64[0] = (u64)0x18191A1B1C1D1E1F;
		CPU.GPR[rt]._u64[1] = (u64)0x1011121314151617;
		CPU.GPR[rt]._u32[3 - (t >> 2)] = 0x00010203;
	}
	void CDD(u32 rt, u32 ra, s32 i7)
	{
		const int t = (CPU.GPR[ra]._u32[3] + i7) & 0x8;

		CPU.GPR[rt]._u64[0] = (u64)0x18191A1B1C1D1E1F;
		CPU.GPR[rt]._u64[1] = (u64)0x1011121314151617;
		CPU.GPR[rt]._u64[1 - (t >> 3)] = (u64)0x0001020304050607;
	}
	void ROTQBII(u32 rt, u32 ra, s32 i7)
	{
		const int s = i7 & 0x7;
		const SPU_GPR_hdr temp = CPU.GPR[ra];
		CPU.GPR[rt]._u32[0] = (temp._u32[0] << s) | (temp._u32[3] >> (32 - s));
		CPU.GPR[rt]._u32[1] = (temp._u32[1] << s) | (temp._u32[0] >> (32 - s));
		CPU.GPR[rt]._u32[2] = (temp._u32[2] << s) | (temp._u32[1] >> (32 - s));
		CPU.GPR[rt]._u32[3] = (temp._u32[3] << s) | (temp._u32[2] >> (32 - s));
	}
	void ROTQMBII(u32 rt, u32 ra, s32 i7)
	{
		const int s = (0 - i7) & 0x7;
		const SPU_GPR_hdr temp = CPU.GPR[ra];
		CPU.GPR[rt]._u32[0] = (temp._u32[0] >> s) | (temp._u32[1] << (32 - s));
		CPU.GPR[rt]._u32[1] = (temp._u32[1] >> s) | (temp._u32[2] << (32 - s));
		CPU.GPR[rt]._u32[2] = (temp._u32[2] >> s) | (temp._u32[3] << (32 - s));
		CPU.GPR[rt]._u32[3] = (temp._u32[3] >> s);
	}
	void SHLQBII(u32 rt, u32 ra, s32 i7)
	{
		const int s = i7 & 0x7;
		const SPU_GPR_hdr temp = CPU.GPR[ra];
		CPU.GPR[rt]._u32[0] = (temp._u32[0] << s);
		CPU.GPR[rt]._u32[1] = (temp._u32[1] << s) | (temp._u32[0] >> (32 - s));
		CPU.GPR[rt]._u32[2] = (temp._u32[2] << s) | (temp._u32[1] >> (32 - s));
		CPU.GPR[rt]._u32[3] = (temp._u32[3] << s) | (temp._u32[2] >> (32 - s));
	}
	void ROTQBYI(u32 rt, u32 ra, s32 i7)
	{
		const int s = i7 & 0xf;
		const SPU_GPR_hdr temp = CPU.GPR[ra];
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = temp._u8[(b - s) & 0xf];
	}
	void ROTQMBYI(u32 rt, u32 ra, s32 i7)
	{
		const int s = (0 - i7) & 0x1f;
		const SPU_GPR_hdr temp = CPU.GPR[ra];
		CPU.GPR[rt].Reset();
		for (int b = 0; b < 16 - s; b++)
			CPU.GPR[rt]._u8[b] = temp._u8[b + s];
	}
	void SHLQBYI(u32 rt, u32 ra, s32 i7)
	{
		const int s = i7 & 0x1f;
		const SPU_GPR_hdr temp = CPU.GPR[ra];
		CPU.GPR[rt].Reset();
		for (int b = s; b < 16; b++)
			CPU.GPR[rt]._u8[b] = temp._u8[b - s];
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
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._u32[w] ^ (~CPU.GPR[rb]._u32[w]);
	}
	void CGTB(u32 rt, u32 ra, u32 rb)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._i8[b] > CPU.GPR[rb]._i8[b] ? 0xff : 0;
	}
	void SUMB(u32 rt, u32 ra, u32 rb)
	{
		const SPU_GPR_hdr _a = CPU.GPR[ra];
		const SPU_GPR_hdr _b = CPU.GPR[rb];
		for (int w = 0; w < 4; w++)
		{
			CPU.GPR[rt]._u16[w*2] = _a._u8[w*4] + _a._u8[w*4 + 1] + _a._u8[w*4 + 2] + _a._u8[w*4 + 3];
			CPU.GPR[rt]._u16[w*2 + 1] = _b._u8[w*4] + _b._u8[w*4 + 1] + _b._u8[w*4 + 2] + _b._u8[w*4 + 3];
		}
	}
	//HGT uses signed values.  HLGT uses unsigned values
	void HGT(u32 rt, s32 ra, s32 rb)
	{
		if(CPU.GPR[ra]._i32[3] > CPU.GPR[rb]._i32[3])	CPU.Stop();
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
		CPU.GPR[rt]._i64[0] = (s64)CPU.GPR[ra]._i32[0];
		CPU.GPR[rt]._i64[1] = (s64)CPU.GPR[ra]._i32[2];
	}
	void XSHW(u32 rt, u32 ra)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._i32[w] = (s32)CPU.GPR[ra]._i16[w*2];
	}
	void CNTB(u32 rt, u32 ra)
	{
		const SPU_GPR_hdr temp = CPU.GPR[ra];
		CPU.GPR[rt].Reset();
		for (int b = 0; b < 16; b++)
			for (int i = 0; i < 8; i++)
				CPU.GPR[rt]._u8[b] += (temp._u8[b] & (1 << i)) ? 1 : 0;
	}
	void XSBH(u32 rt, u32 ra)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._i16[h] = (s16)CPU.GPR[ra]._i8[h*2];
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
		CPU.GPR[rt]._u32[0] = CPU.GPR[ra]._f[0] > CPU.GPR[rb]._f[0] ? 0xffffffff : 0;
		CPU.GPR[rt]._u32[1] = CPU.GPR[ra]._f[1] > CPU.GPR[rb]._f[1] ? 0xffffffff : 0;
		CPU.GPR[rt]._u32[2] = CPU.GPR[ra]._f[2] > CPU.GPR[rb]._f[2] ? 0xffffffff : 0;
		CPU.GPR[rt]._u32[3] = CPU.GPR[ra]._f[3] > CPU.GPR[rb]._f[3] ? 0xffffffff : 0;
	}
	void DFCGT(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._u64[0] = CPU.GPR[ra]._d[0] > CPU.GPR[rb]._d[0] ? 0xffffffffffffffff : 0;
		CPU.GPR[rt]._u64[1] = CPU.GPR[ra]._d[1] > CPU.GPR[rb]._d[1] ? 0xffffffffffffffff : 0;
	}
	void FA(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._f[0] = CPU.GPR[ra]._f[0] + CPU.GPR[rb]._f[0];
		CPU.GPR[rt]._f[1] = CPU.GPR[ra]._f[1] + CPU.GPR[rb]._f[1];
		CPU.GPR[rt]._f[2] = CPU.GPR[ra]._f[2] + CPU.GPR[rb]._f[2];
		CPU.GPR[rt]._f[3] = CPU.GPR[ra]._f[3] + CPU.GPR[rb]._f[3];
	}
	void FS(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._f[0] = CPU.GPR[ra]._f[0] - CPU.GPR[rb]._f[0];
		CPU.GPR[rt]._f[1] = CPU.GPR[ra]._f[1] - CPU.GPR[rb]._f[1];
		CPU.GPR[rt]._f[2] = CPU.GPR[ra]._f[2] - CPU.GPR[rb]._f[2];
		CPU.GPR[rt]._f[3] = CPU.GPR[ra]._f[3] - CPU.GPR[rb]._f[3];
	}
	void FM(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._f[0] = CPU.GPR[ra]._f[0] * CPU.GPR[rb]._f[0];
		CPU.GPR[rt]._f[1] = CPU.GPR[ra]._f[1] * CPU.GPR[rb]._f[1];
		CPU.GPR[rt]._f[2] = CPU.GPR[ra]._f[2] * CPU.GPR[rb]._f[2];
		CPU.GPR[rt]._f[3] = CPU.GPR[ra]._f[3] * CPU.GPR[rb]._f[3];
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
		CPU.GPR[rt]._u32[0] = fabs(CPU.GPR[ra]._f[0]) > fabs(CPU.GPR[rb]._f[0]) ? 0xffffffff : 0;
		CPU.GPR[rt]._u32[1] = fabs(CPU.GPR[ra]._f[1]) > fabs(CPU.GPR[rb]._f[1]) ? 0xffffffff : 0;
		CPU.GPR[rt]._u32[2] = fabs(CPU.GPR[ra]._f[2]) > fabs(CPU.GPR[rb]._f[2]) ? 0xffffffff : 0;
		CPU.GPR[rt]._u32[3] = fabs(CPU.GPR[ra]._f[3]) > fabs(CPU.GPR[rb]._f[3]) ? 0xffffffff : 0;
	}
	void DFCMGT(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._u64[0] = fabs(CPU.GPR[ra]._d[0]) > fabs(CPU.GPR[rb]._d[0]) ? 0xffffffffffffffff : 0;
		CPU.GPR[rt]._u64[1] = fabs(CPU.GPR[ra]._d[1]) > fabs(CPU.GPR[rb]._d[1]) ? 0xffffffffffffffff : 0;
	}
	void DFA(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._d[0] = CPU.GPR[ra]._d[0] + CPU.GPR[rb]._d[0];
		CPU.GPR[rt]._d[1] = CPU.GPR[ra]._d[1] + CPU.GPR[rb]._d[1];
	}
	void DFS(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._d[0] = CPU.GPR[ra]._d[0] - CPU.GPR[rb]._d[0];
		CPU.GPR[rt]._d[1] = CPU.GPR[ra]._d[1] - CPU.GPR[rb]._d[1];
	}
	void DFM(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._d[0] = CPU.GPR[ra]._d[0] * CPU.GPR[rb]._d[0];
		CPU.GPR[rt]._d[1] = CPU.GPR[ra]._d[1] * CPU.GPR[rb]._d[1];
	}
	void CLGTB(u32 rt, u32 ra, u32 rb)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._u8[b] = CPU.GPR[ra]._u8[b] > CPU.GPR[rb]._u8[b] ? 0xff : 0;
	}
	void HLGT(u32 rt, u32 ra, u32 rb)
	{
		if(CPU.GPR[ra]._u32[3] > CPU.GPR[rb]._u32[3])	CPU.Stop();
	}
	void DFMA(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._d[0] += CPU.GPR[ra]._d[0] * CPU.GPR[rb]._d[0];
		CPU.GPR[rt]._d[1] += CPU.GPR[ra]._d[1] * CPU.GPR[rb]._d[1];
	}
	void DFMS(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._d[0] = CPU.GPR[ra]._d[0] * CPU.GPR[rb]._d[0] - CPU.GPR[rt]._d[0];
		CPU.GPR[rt]._d[1] = CPU.GPR[ra]._d[1] * CPU.GPR[rb]._d[1] - CPU.GPR[rt]._d[1];
	}
	void DFNMS(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._d[0] -= CPU.GPR[ra]._d[0] * CPU.GPR[rb]._d[0];
		CPU.GPR[rt]._d[1] -= CPU.GPR[ra]._d[1] * CPU.GPR[rb]._d[1];
	}
	void DFNMA(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._d[0] = -(CPU.GPR[ra]._d[0] * CPU.GPR[rb]._d[0] + CPU.GPR[rt]._d[0]);
		CPU.GPR[rt]._d[1] = -(CPU.GPR[ra]._d[1] * CPU.GPR[rb]._d[1] + CPU.GPR[rt]._d[1]);
	}
	void CEQ(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._i32[w] == CPU.GPR[rb]._i32[w] ? 0xffffffff : 0;
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
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = ((u64)CPU.GPR[ra]._u32[w] + (u64)CPU.GPR[rb]._u32[w] + (u64)(CPU.GPR[rt]._u32[w] & 1)) >> 32;
	}
	void BGX(u32 rt, u32 ra, u32 rb)
	{
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
			CPU.GPR[rt]._i32[w] += CPU.GPR[ra]._i16[w*2+1] * CPU.GPR[rb]._i16[w*2+1];
	}
	void MPYHHAU(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] += CPU.GPR[ra]._u16[w*2+1] * CPU.GPR[rb]._u16[w*2+1];
	}
	//Forced bits to 0, hence the shift:
	
	void FSCRRD(u32 rt)
	{
		/*CPU.GPR[rt]._u128.lo = 
			CPU.FPSCR.Exception0 << 20 &
			CPU.FPSCR.*/
		UNIMPLEMENTED();
	}
	void FESD(u32 rt, u32 ra)
	{
		CPU.GPR[rt]._d[0] = (double)CPU.GPR[ra]._f[1];
		CPU.GPR[rt]._d[1] = (double)CPU.GPR[ra]._f[3];
	}
	void FRDS(u32 rt, u32 ra)
	{
		CPU.GPR[rt]._f[1] = (float)CPU.GPR[ra]._d[0];
		CPU.GPR[rt]._u32[0] = 0x00000000;
		CPU.GPR[rt]._f[3] = (float)CPU.GPR[ra]._d[1];
		CPU.GPR[rt]._u32[2] = 0x00000000;
	}
	void FSCRWR(u32 rt, u32 ra)
	{
		UNIMPLEMENTED();
	}
	void DFTSV(u32 rt, u32 ra, s32 i7)
	{
		const u64 DoubleExpMask = 0x7ff0000000000000;
		const u64 DoubleFracMask = 0x000fffffffffffff;
		const u64 DoubleSignMask = 0x8000000000000000;
		const SPU_GPR_hdr temp = CPU.GPR[ra];
		CPU.GPR[rt].Reset();
		if (i7 & 1) //Negative Denorm Check (-, exp is zero, frac is non-zero)
			for (int i = 0; i < 2; i++)
			{
				if (temp._u64[i] & DoubleFracMask)
					if ((temp._u64[i] & (DoubleSignMask | DoubleExpMask)) == DoubleSignMask)
						CPU.GPR[rt]._u64[i] = 0xffffffffffffffff;
			}
		if (i7 & 2) //Positive Denorm Check (+, exp is zero, frac is non-zero)
			for (int i = 0; i < 2; i++)
			{
				if (temp._u64[i] & DoubleFracMask)
					if ((temp._u64[i] & (DoubleSignMask | DoubleExpMask)) == 0)
						CPU.GPR[rt]._u64[i] = 0xffffffffffffffff;
			}
		if (i7 & 4) //Negative Zero Check (-, exp is zero, frac is zero)
			for (int i = 0; i < 2; i++)
			{
				if (temp._u64[i] == DoubleSignMask)
					CPU.GPR[rt]._u64[i] = 0xffffffffffffffff;
			}
		if (i7 & 8) //Positive Zero Check (+, exp is zero, frac is zero)
			for (int i = 0; i < 2; i++)
			{
				if (temp._u64[i] == 0)
					CPU.GPR[rt]._u64[i] = 0xffffffffffffffff;
			}
		if (i7 & 16) //Negative Infinity Check (-, exp is 0x7ff, frac is zero)
			for (int i = 0; i < 2; i++)
			{
				if (temp._u64[i] == (DoubleSignMask | DoubleExpMask))
					CPU.GPR[rt]._u64[i] = 0xffffffffffffffff;
			}
		if (i7 & 32) //Positive Infinity Check (+, exp is 0x7ff, frac is zero)
			for (int i = 0; i < 2; i++)
			{
				if (temp._u64[i] == DoubleExpMask)
					CPU.GPR[rt]._u64[i] = 0xffffffffffffffff;
			}
		if (i7 & 64) //Not-a-Number Check (any sign, exp is 0x7ff, frac is non-zero)
			for (int i = 0; i < 2; i++)
			{
				if (temp._u64[i] & DoubleFracMask)
					if ((temp._u64[i] & DoubleExpMask) == DoubleExpMask)
						CPU.GPR[rt]._u64[i] = 0xffffffffffffffff;
			}
	}
	void FCEQ(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._u32[0] = CPU.GPR[ra]._f[0] == CPU.GPR[rb]._f[0] ? 0xffffffff : 0;
		CPU.GPR[rt]._u32[1] = CPU.GPR[ra]._f[1] == CPU.GPR[rb]._f[1] ? 0xffffffff : 0;
		CPU.GPR[rt]._u32[2] = CPU.GPR[ra]._f[2] == CPU.GPR[rb]._f[2] ? 0xffffffff : 0;
		CPU.GPR[rt]._u32[3] = CPU.GPR[ra]._f[3] == CPU.GPR[rb]._f[3] ? 0xffffffff : 0;
	}
	void DFCEQ(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._u64[0] = CPU.GPR[ra]._d[0] == CPU.GPR[rb]._d[0] ? 0xffffffffffffffff : 0;
		CPU.GPR[rt]._u64[1] = CPU.GPR[ra]._d[1] == CPU.GPR[rb]._d[1] ? 0xffffffffffffffff : 0;
	}
	void MPY(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._i32[w] = CPU.GPR[ra]._i16[w*2] * CPU.GPR[rb]._i16[w*2];
	}
	void MPYH(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._i32[w] = (CPU.GPR[ra]._i16[w*2+1] * CPU.GPR[rb]._i16[w*2]) << 16;
	}
	void MPYHH(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._i32[w] = CPU.GPR[ra]._i16[w*2+1] * CPU.GPR[rb]._i16[w*2+1];
	}
	void MPYS(u32 rt, u32 ra, u32 rb)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._i32[w] = (CPU.GPR[ra]._i16[w*2] * CPU.GPR[rb]._i16[w*2]) >> 16;
	}
	void CEQH(u32 rt, u32 ra, u32 rb)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = CPU.GPR[ra]._u16[h] == CPU.GPR[rb]._u16[h] ? 0xffff : 0;
	}
	void FCMEQ(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._u32[0] = fabs(CPU.GPR[ra]._f[0]) == fabs(CPU.GPR[rb]._f[0]) ? 0xffffffff : 0;
		CPU.GPR[rt]._u32[1] = fabs(CPU.GPR[ra]._f[1]) == fabs(CPU.GPR[rb]._f[1]) ? 0xffffffff : 0;
		CPU.GPR[rt]._u32[2] = fabs(CPU.GPR[ra]._f[2]) == fabs(CPU.GPR[rb]._f[2]) ? 0xffffffff : 0;
		CPU.GPR[rt]._u32[3] = fabs(CPU.GPR[ra]._f[3]) == fabs(CPU.GPR[rb]._f[3]) ? 0xffffffff : 0;
	}
	void DFCMEQ(u32 rt, u32 ra, u32 rb)
	{
		CPU.GPR[rt]._u64[0] = fabs(CPU.GPR[ra]._d[0]) == fabs(CPU.GPR[rb]._d[0]) ? 0xffffffffffffffff : 0;
		CPU.GPR[rt]._u64[1] = fabs(CPU.GPR[ra]._d[1]) == fabs(CPU.GPR[rb]._d[1]) ? 0xffffffffffffffff : 0;
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
		//Floating Interpolation: ra will be ignored.
		//It should work correctly if result of preceding FREST or FRSQEST is sufficiently exact
		CPU.GPR[rt] = CPU.GPR[rb];
	}
	void HEQ(u32 rt, u32 ra, u32 rb)
	{
		if(CPU.GPR[ra]._i32[3] == CPU.GPR[rb]._i32[3])	CPU.Stop();
	}

	//0 - 9
	void CFLTS(u32 rt, u32 ra, s32 i8)
	{
		const u32 scale = 173 - (i8 & 0xff); //unsigned immediate
		for (int i = 0; i < 4; i++)
		{
			u32 exp = ((CPU.GPR[ra]._u32[i] >> 23) & 0xff) + scale;

			if (exp > 255) 
				exp = 255;

			CPU.GPR[rt]._u32[i] = (CPU.GPR[ra]._u32[i] & 0x807fffff) | (exp << 23);

			CPU.GPR[rt]._u32[i] = (u32)CPU.GPR[rt]._f[i]; //trunc
		}
		//CPU.GPR[rt]._m128i = _mm_cvttps_epi32(CPU.GPR[rt]._m128);
	}
	void CFLTU(u32 rt, u32 ra, s32 i8)
	{
		const u32 scale = 173 - (i8 & 0xff); //unsigned immediate
		for (int i = 0; i < 4; i++)
		{
			u32 exp = ((CPU.GPR[ra]._u32[i] >> 23) & 0xff) + scale;

			if (exp > 255) 
				exp = 255;

			if (CPU.GPR[ra]._u32[i] & 0x80000000) //if negative, result = 0
				CPU.GPR[rt]._u32[i] = 0;
			else
			{
				CPU.GPR[rt]._u32[i] = (CPU.GPR[ra]._u32[i] & 0x807fffff) | (exp << 23);

				if (CPU.GPR[rt]._f[i] > 0xffffffff) //if big, result = max
					CPU.GPR[rt]._u32[i] = 0xffffffff;
				else
					CPU.GPR[rt]._u32[i] = floor(CPU.GPR[rt]._f[i]);
			}
		}
	}
	void CSFLT(u32 rt, u32 ra, s32 i8)
	{
		//CPU.GPR[rt]._m128 = _mm_cvtepi32_ps(CPU.GPR[ra]._m128i);
		const u32 scale = 155 - (i8 & 0xff); //unsigned immediate
		for (int i = 0; i < 4; i++)
		{
			CPU.GPR[rt]._f[i] = (s32)CPU.GPR[ra]._i32[i];

			u32 exp = ((CPU.GPR[rt]._u32[i] >> 23) & 0xff) - scale;

			if (exp > 255) //< 0
				exp = 0;

			CPU.GPR[rt]._u32[i] = (CPU.GPR[rt]._u32[i] & 0x807fffff) | (exp << 23);
		}
	}
	void CUFLT(u32 rt, u32 ra, s32 i8)
	{
		const u32 scale = 155 - (i8 & 0xff); //unsigned immediate
		for (int i = 0; i < 4; i++)
		{
			CPU.GPR[rt]._f[i] = (float)CPU.GPR[ra]._u32[i];
			u32 exp = ((CPU.GPR[rt]._u32[i] >> 23) & 0xff) - scale;

			if (exp > 255) //< 0
				exp = 0;

			CPU.GPR[rt]._u32[i] = (CPU.GPR[rt]._u32[i] & 0x807fffff) | (exp << 23);
		}
	}

	//0 - 8
	void BRZ(u32 rt, s32 i16)
	{
		if (CPU.GPR[rt]._u32[3] == 0)
			CPU.SetBranch(branchTarget(CPU.PC, i16));
	}
	void STQA(u32 rt, s32 i16)
	{
		u32 lsa = (i16 << 2) & 0x3fff0;
		if(!CPU.IsGoodLSA(lsa))
		{
			ConLog.Error("STQA: bad lsa (0x%x)", lsa);
			Emu.Pause();
			return;
		}

		CPU.WriteLS128(lsa, CPU.GPR[rt]._u128);
	}
	void BRNZ(u32 rt, s32 i16)
	{
		if (CPU.GPR[rt]._u32[3] != 0)
			CPU.SetBranch(branchTarget(CPU.PC, i16));
	}
	void BRHZ(u32 rt, s32 i16)
	{
		if (CPU.GPR[rt]._u16[6] == 0) 
			CPU.SetBranch(branchTarget(CPU.PC, i16));
	}
	void BRHNZ(u32 rt, s32 i16)
	{
		if (CPU.GPR[rt]._u16[6] != 0) 
			CPU.SetBranch(branchTarget(CPU.PC, i16));
	}
	void STQR(u32 rt, s32 i16)
	{
		u32 lsa = branchTarget(CPU.PC, i16) & 0x3fff0; 
		if(!CPU.IsGoodLSA(lsa))
		{
			ConLog.Error("STQR: bad lsa (0x%x)", lsa);
			Emu.Pause();
			return;
		}

		CPU.WriteLS128(lsa, CPU.GPR[rt]._u128);
	}
	void BRA(s32 i16)
	{
		CPU.SetBranch(branchTarget(0, i16));
	}
	void LQA(u32 rt, s32 i16)
	{
		u32 lsa = (i16 << 2) & 0x3fff0;
		if(!CPU.IsGoodLSA(lsa))
		{
			ConLog.Error("LQA: bad lsa (0x%x)", lsa);
			Emu.Pause();
			return;
		}

		CPU.GPR[rt]._u128 = CPU.ReadLS128(lsa);
	}
	void BRASL(u32 rt, s32 i16)
	{
		CPU.GPR[rt].Reset();
		CPU.GPR[rt]._u32[3] = CPU.PC + 4;
		CPU.SetBranch(branchTarget(0, i16));
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
		u32 lsa = branchTarget(CPU.PC, i16) & 0x3fff0;
		if(!CPU.IsGoodLSA(lsa))
		{
			ConLog.Error("LQR: bad lsa (0x%x)", lsa);
			Emu.Pause();
			return;
		}

		CPU.GPR[rt]._u128 = CPU.ReadLS128(lsa);
	}
	void IL(u32 rt, s32 i16)
	{
		CPU.GPR[rt]._i32[0] =
			CPU.GPR[rt]._i32[1] =
			CPU.GPR[rt]._i32[2] =
			CPU.GPR[rt]._i32[3] = i16;
	}
	void ILHU(u32 rt, s32 i16)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._i32[w] = i16 << 16;
	}
	void ILH(u32 rt, s32 i16)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._i16[h] = i16;
	}
	void IOHL(u32 rt, s32 i16)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._i32[w] |= (i16 & 0xFFFF);
	}
	

	//0 - 7
	void ORI(u32 rt, u32 ra, s32 i10)
	{
		for(u32 i = 0; i < 4; ++i)
			CPU.GPR[rt]._i32[i] = CPU.GPR[ra]._i32[i] | i10;
	}
	void ORHI(u32 rt, u32 ra, s32 i10)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._i16[h] = CPU.GPR[ra]._i16[h] | i10;
	}
	void ORBI(u32 rt, u32 ra, s32 i10)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._i8[b] = CPU.GPR[ra]._i8[b] | i10;
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
			CPU.GPR[rt]._i32[w] = CPU.GPR[ra]._i32[w] & i10;
	}
	void ANDHI(u32 rt, u32 ra, s32 i10)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._i16[h] = CPU.GPR[ra]._i16[h] & i10;
	}
	void ANDBI(u32 rt, u32 ra, s32 i10)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._i8[b] = CPU.GPR[ra]._i8[b] & i10;
	}
	void AI(u32 rt, u32 ra, s32 i10)
	{
		CPU.GPR[rt]._i32[0] = CPU.GPR[ra]._i32[0] + i10;
		CPU.GPR[rt]._i32[1] = CPU.GPR[ra]._i32[1] + i10;
		CPU.GPR[rt]._i32[2] = CPU.GPR[ra]._i32[2] + i10;
		CPU.GPR[rt]._i32[3] = CPU.GPR[ra]._i32[3] + i10;
	}
	void AHI(u32 rt, u32 ra, s32 i10)
	{
		for(u32 h = 0; h < 8; ++h)
			CPU.GPR[rt]._i16[h] = CPU.GPR[ra]._i16[h] + i10;
	}
	void STQD(u32 rt, s32 i10, u32 ra) //i10 is shifted left by 4 while decoding
	{
		const u32 lsa = (CPU.GPR[ra]._i32[3] + i10) & 0x3fff0;
		if(!CPU.IsGoodLSA(lsa))
		{
			ConLog.Error("STQD: bad lsa (0x%x)", lsa);
			Emu.Pause();
			return;
		}
		CPU.WriteLS128(lsa, CPU.GPR[rt]._u128);
	}
	void LQD(u32 rt, s32 i10, u32 ra) //i10 is shifted left by 4 while decoding
	{
		const u32 lsa = (CPU.GPR[ra]._i32[3] + i10) & 0x3fff0;
		if(!CPU.IsGoodLSA(lsa))
		{
			ConLog.Error("LQD: bad lsa (0x%x)", lsa);
			Emu.Pause();
			return;
		}

		CPU.GPR[rt]._u128 = CPU.ReadLS128(lsa);
	}
	void XORI(u32 rt, u32 ra, s32 i10)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._i32[w] = CPU.GPR[ra]._i32[w] ^ i10;
	}
	void XORHI(u32 rt, u32 ra, s32 i10)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._i16[h] = CPU.GPR[ra]._i16[h] ^ i10;
	}
	void XORBI(u32 rt, u32 ra, s32 i10)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._i8[b] = CPU.GPR[ra]._i8[b] ^ i10;
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
		if(CPU.GPR[ra]._i32[3] > i10)	CPU.Stop();
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
		if(CPU.GPR[ra]._u32[3] > (u32)i10)	CPU.Stop();
	}
	void MPYI(u32 rt, u32 ra, s32 i10)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._i32[w] = CPU.GPR[ra]._i16[w*2] * i10;
	}
	void MPYUI(u32 rt, u32 ra, s32 i10)
	{
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = CPU.GPR[ra]._u16[w*2] * (u16)(i10 & 0xffff);
	}
	void CEQI(u32 rt, u32 ra, s32 i10)
	{
		for(u32 i = 0; i < 4; ++i)
			CPU.GPR[rt]._u32[i] = (CPU.GPR[ra]._i32[i] == i10) ? 0xffffffff : 0x00000000;
	}
	void CEQHI(u32 rt, u32 ra, s32 i10)
	{
		for (int h = 0; h < 8; h++)
			CPU.GPR[rt]._u16[h] = (CPU.GPR[ra]._i16[h] == (s16)i10) ? 0xffff : 0;
	}
	void CEQBI(u32 rt, u32 ra, s32 i10)
	{
		for (int b = 0; b < 16; b++)
			CPU.GPR[rt]._i8[b] = (CPU.GPR[ra]._i8[b] == (s8)(i10 & 0xff)) ? 0xff : 0;
	}
	void HEQI(u32 rt, u32 ra, s32 i10)
	{
		if(CPU.GPR[ra]._i32[3] == i10)	CPU.Stop();
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
		const SPU_GPR_hdr _a = CPU.GPR[ra];
		const SPU_GPR_hdr _b = CPU.GPR[rb];
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
			CPU.GPR[rt]._i32[w] = CPU.GPR[ra]._i16[w*2] * CPU.GPR[rb]._i16[w*2] + CPU.GPR[rc]._i32[w];
	}
	void FNMS(u32 rt, u32 ra, u32 rb, u32 rc)
	{
		CPU.GPR[rt]._f[0] = CPU.GPR[rc]._f[0] - CPU.GPR[ra]._f[0] * CPU.GPR[rb]._f[0];
		CPU.GPR[rt]._f[1] = CPU.GPR[rc]._f[1] - CPU.GPR[ra]._f[1] * CPU.GPR[rb]._f[1];
		CPU.GPR[rt]._f[2] = CPU.GPR[rc]._f[2] - CPU.GPR[ra]._f[2] * CPU.GPR[rb]._f[2];
		CPU.GPR[rt]._f[3] = CPU.GPR[rc]._f[3] - CPU.GPR[ra]._f[3] * CPU.GPR[rb]._f[3];
	}
	void FMA(u32 rt, u32 ra, u32 rb, u32 rc)
	{
		CPU.GPR[rt]._f[0] = CPU.GPR[ra]._f[0] * CPU.GPR[rb]._f[0] + CPU.GPR[rc]._f[0];
		CPU.GPR[rt]._f[1] = CPU.GPR[ra]._f[1] * CPU.GPR[rb]._f[1] + CPU.GPR[rc]._f[1];
		CPU.GPR[rt]._f[2] = CPU.GPR[ra]._f[2] * CPU.GPR[rb]._f[2] + CPU.GPR[rc]._f[2];
		CPU.GPR[rt]._f[3] = CPU.GPR[ra]._f[3] * CPU.GPR[rb]._f[3] + CPU.GPR[rc]._f[3];
	}
	void FMS(u32 rt, u32 ra, u32 rb, u32 rc)
	{
		CPU.GPR[rt]._f[0] = CPU.GPR[ra]._f[0] * CPU.GPR[rb]._f[0] - CPU.GPR[rc]._f[0];
		CPU.GPR[rt]._f[1] = CPU.GPR[ra]._f[1] * CPU.GPR[rb]._f[1] - CPU.GPR[rc]._f[1];
		CPU.GPR[rt]._f[2] = CPU.GPR[ra]._f[2] * CPU.GPR[rb]._f[2] - CPU.GPR[rc]._f[2];
		CPU.GPR[rt]._f[3] = CPU.GPR[ra]._f[3] * CPU.GPR[rb]._f[3] - CPU.GPR[rc]._f[3];
	}

	void UNK(u32 code, u32 opcode, u32 gcode)
	{
		UNK(fmt::Format("Unknown/Illegal opcode! (0x%08x, 0x%x, 0x%x)", code, opcode, gcode));
	}

	void UNK(const std::string& err)
	{
		ConLog.Error(err + fmt::Format(" #pc: 0x%x", CPU.PC));
		Emu.Pause();
		for(uint i=0; i<128; ++i) ConLog.Write("r%d = 0x%s", i, CPU.GPR[i].ToString().c_str());
	}
};
