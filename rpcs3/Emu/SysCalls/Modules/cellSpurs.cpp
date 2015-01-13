#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/CB_FUNC.h"
#include "Emu/Memory/atomic_type.h"

#include "Emu/Cell/SPUThread.h"
#include "Emu/SysCalls/lv2/sleep_queue_type.h"
#include "Emu/SysCalls/lv2/sys_lwmutex.h"
#include "Emu/SysCalls/lv2/sys_lwcond.h"
#include "Emu/SysCalls/lv2/sys_spu.h"
#include "Emu/SysCalls/lv2/sys_ppu_thread.h"
#include "Emu/SysCalls/lv2/sys_memory.h"
#include "Emu/SysCalls/lv2/sys_process.h"
#include "Emu/SysCalls/lv2/sys_semaphore.h"
#include "Emu/SysCalls/lv2/sys_event.h"
#include "Emu/Cell/SPURSManager.h"
#include "sysPrxForUser.h"
#include "cellSpurs.h"

Module *cellSpurs = nullptr;

#ifdef PRX_DEBUG
extern u32 libsre;
extern u32 libsre_rtoc;
#endif

s64 spursCreateLv2EventQueue(vm::ptr<CellSpurs> spurs, u32& queue_id, vm::ptr<u8> port, s32 size, u64 name_u64)
{
#ifdef PRX_DEBUG_XXX
	vm::var<be_t<u32>> queue;
	s32 res = cb_call<s32, vm::ptr<CellSpurs>, vm::ptr<u32>, vm::ptr<u8>, s32, u32>(GetCurrentPPUThread(), libsre + 0xB14C, libsre_rtoc,
		spurs, queue, port, size, vm::read32(libsre_rtoc - 0x7E2C));
	queue_id = queue->ToLE();
	return res;
#endif

	queue_id = event_queue_create(SYS_SYNC_FIFO, SYS_PPU_QUEUE, name_u64, 0, size);
	if (!queue_id)
	{
		return CELL_EAGAIN; // rough
	}

	if (s32 res = spursAttachLv2EventQueue(spurs, queue_id, port, 1, true))
	{
		assert(!"spursAttachLv2EventQueue() failed");
	}
	return CELL_OK;
}

s64 spursInit(
	vm::ptr<CellSpurs> spurs,
	const u32 revision,
	const u32 sdkVersion,
	const s32 nSpus,
	const s32 spuPriority,
	const s32 ppuPriority,
	u32 flags, // SpursAttrFlags
	const char prefix[],
	const u32 prefixSize,
	const u32 container,
	const u8 swlPriority[],
	const u32 swlMaxSpu,
	const u32 swlIsPreem)
{
#ifdef PRX_DEBUG_XXX
	return cb_call<s32, vm::ptr<CellSpurs>, u32, u32, s32, s32, s32, u32, u32, u32, u32, u32, u32, u32>(GetCurrentPPUThread(), libsre + 0x74E4, libsre_rtoc,
		spurs, revision, sdkVersion, nSpus, spuPriority, ppuPriority, flags, Memory.RealToVirtualAddr(prefix), prefixSize, container, Memory.RealToVirtualAddr(swlPriority), swlMaxSpu, swlIsPreem);
#endif

	// SPURS initialization (asserts should actually rollback and return the error instead)

	if (!spurs)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}
	if (spurs.addr() % 128)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}
	if (prefixSize > CELL_SPURS_NAME_MAX_LENGTH)
	{
		return CELL_SPURS_CORE_ERROR_INVAL;
	}
	if (process_is_spu_lock_line_reservation_address(spurs.addr(), SYS_MEMORY_ACCESS_RIGHT_SPU_THR) != CELL_OK)
	{
		return CELL_SPURS_CORE_ERROR_PERM;
	}

	const bool isSecond = (flags & SAF_SECOND_VERSION) != 0;
	memset(spurs.get_ptr(), 0, CellSpurs::size1 + isSecond * CellSpurs::size2);
	spurs->m.revision = revision;
	spurs->m.sdkVersion = sdkVersion;
	spurs->m.ppu0 = 0xffffffffull;
	spurs->m.ppu1 = 0xffffffffull;
	spurs->m.flags = flags;
	memcpy(spurs->m.prefix, prefix, prefixSize);
	spurs->m.prefixSize = (u8)prefixSize;

	std::string name(prefix, prefixSize); // initialize name string

	if (!isSecond)
	{
		spurs->m.wklMskA.write_relaxed(be_t<u32>::make(0xffff));
	}
	spurs->m.xCC = 0;
	spurs->m.xCD = 0;
	spurs->m.xCE = 0;
	for (u32 i = 0; i < 8; i++)
	{
		spurs->m.xC0[i] = -1;
	}

	// default or system workload:
#ifdef PRX_DEBUG
	spurs->m.wklSysG.pm.set(be_t<u64>::make(vm::read32(libsre_rtoc - 0x7EA4)));
	spurs->m.wklSysG.size = 0x2200;
#else
	spurs->m.wklSysG.pm.set(be_t<u64>::make(0x100)); // wrong 64-bit address
#endif
	spurs->m.wklSysG.data = 0;
	spurs->m.wklSysG.copy.write_relaxed(0xff);
	u32 sem;
	for (u32 i = 0; i < 0x10; i++)
	{
		sem = semaphore_create(0, 1, SYS_SYNC_PRIORITY, *(u64*)"_spuWkl");
		assert(sem && ~sem); // should rollback if semaphore creation failed and return the error
		spurs->m.wklF1[i].sem = sem;
	}
	if (isSecond)
	{
		for (u32 i = 0; i < 0x10; i++)
		{
			sem = semaphore_create(0, 1, SYS_SYNC_PRIORITY, *(u64*)"_spuWkl");
			assert(sem && ~sem);
			spurs->m.wklF2[i].sem = sem;
		}
	}
	sem = semaphore_create(0, 1, SYS_SYNC_PRIORITY, *(u64*)"_spuPrv");
	assert(sem && ~sem);
	spurs->m.semPrv = sem;
	spurs->m.unk11 = -1;
	spurs->m.unk12 = -1;
	spurs->m.unk13 = 0;
	spurs->m.nSpus = nSpus;
	spurs->m.spuPriority = spuPriority;
#ifdef PRX_DEBUG
	if (s32 res = spu_image_import(spurs->m.spuImg, vm::read32(libsre_rtoc - (isSecond ? 0x7E94 : 0x7E98)), 1))
	{
		assert(!"spu_image_import() failed");
	}
#else
	spurs->m.spuImg.addr = Memory.Alloc(0x40000, 4096);
#endif

	s32 tgt = SYS_SPU_THREAD_GROUP_TYPE_NORMAL;
	if (flags & SAF_SPU_TGT_EXCLUSIVE_NON_CONTEXT)
	{
		tgt = SYS_SPU_THREAD_GROUP_TYPE_EXCLUSIVE_NON_CONTEXT;
	}
	else if (flags & SAF_UNKNOWN_FLAG_0)
	{
		tgt = 0xC02;
	}
	if (flags & SAF_SPU_MEMORY_CONTAINER_SET) tgt |= SYS_SPU_THREAD_GROUP_TYPE_MEMORY_FROM_CONTAINER;
	if (flags & SAF_SYSTEM_WORKLOAD_ENABLED) tgt |= SYS_SPU_THREAD_GROUP_TYPE_COOPERATE_WITH_SYSTEM;
	if (flags & SAF_UNKNOWN_FLAG_7) tgt |= 0x102;
	if (flags & SAF_UNKNOWN_FLAG_8) tgt |= 0xC02;
	if (flags & SAF_UNKNOWN_FLAG_9) tgt |= 0x800;
	auto tg = spu_thread_group_create(name + "CellSpursKernelGroup", nSpus, spuPriority, tgt, container);
	assert(tg);
	spurs->m.spuTG = tg->m_id;

	name += "CellSpursKernel0";
	for (s32 num = 0; num < nSpus; num++, name[name.size() - 1]++)
	{
		spurs->m.spus[num] = spu_thread_initialize(tg, num, spurs->m.spuImg, name, SYS_SPU_THREAD_OPTION_DEC_SYNC_TB_ENABLE, 0, 0, 0, 0, [spurs, num, isSecond](SPUThread& SPU)
		{
#ifdef PRX_DEBUG_XXX
			SPU.GPR[3]._u32[3] = num;
			SPU.GPR[4]._u64[1] = spurs.addr();
			return SPU.FastCall(SPU.PC);
#endif

			// code replacement:
			{
				const u32 addr = /*SPU.ReadLS32(0x1e0) +*/ 8; //SPU.ReadLS32(0x1e4);
				SPU.WriteLS32(addr + 0, 3); // hack for cellSpursModulePollStatus
				SPU.WriteLS32(addr + 4, 0x35000000); // bi $0
				SPU.WriteLS32(0x1e4, addr);

				SPU.WriteLS32(SPU.ReadLS32(0x1e0), 2); // hack for cellSpursModuleExit
			}

			if (!isSecond) SPU.m_code3_func = [spurs, num](SPUThread& SPU) -> u64 // first kernel
			{
				LV2_LOCK(0); // TODO: lock-free implementation if possible

				const u32 arg1 = SPU.GPR[3]._u32[3];
				u32 var0 = SPU.ReadLS32(0x1d8);
				u32 var1 = SPU.ReadLS32(0x1dc);
				u128 wklA = vm::read128(spurs.addr() + 0x20);
				u128 wklB = vm::read128(spurs.addr() + 0x30);
				u128 savedA = SPU.ReadLS128(0x180);
				u128 savedB = SPU.ReadLS128(0x190);
				u128 vAA = u128::sub8(wklA, savedA);
				u128 vBB = u128::sub8(wklB, savedB);
				u128 vM1 = {}; if (var1 <= 15) vM1.u8r[var1] = 0xff;
				u128 vAABB = (arg1 == 0) ? vAA : u128::add8(vAA, u128::andnot(vM1, vBB));
				
				u32 vNUM = 0x20;
				u64 vRES = 0x20ull << 32;
				u128 vSET = {};

				if (spurs->m.x72.read_relaxed() & (1 << num))
				{
					SPU.WriteLS8(0x1eb, 0); // var4
					if (arg1 == 0 || var1 == 0x20)
					{
						spurs->m.x72._and_not(1 << num);
					}
				}
				else
				{
					u128 wklReadyCount0 = vm::read128(spurs.addr() + 0x0);
					u128 wklReadyCount1 = vm::read128(spurs.addr() + 0x10);
					u128 savedC = SPU.ReadLS128(0x1A0);
					u128 savedD = SPU.ReadLS128(0x1B0);
					u128 vRC = u128::add8(u128::minu8(wklReadyCount0, u128::from8p(8)), u128::minu8(wklReadyCount1, u128::from8p(8)));
					u32 wklFlag = spurs->m.wklFlag.flag.read_relaxed();
					u32 flagRecv = spurs->m.flagRecv.read_relaxed();
					u128 vFM = u128::fromV(g_imm_table.fsmb_table[(wklFlag == 0) && (flagRecv < 16) ? 0x8000 >> flagRecv : 0]);
					u128 wklSet1 = u128::fromV(g_imm_table.fsmb_table[spurs->m.wklSet1.read_relaxed()]);
					u128 vFMS1 = vFM | wklSet1;
					u128 vFMV1 = u128::fromV(g_imm_table.fsmb_table[(var1 < 16) ? 0x8000 >> var1 : 0]);
					u32 var5 = SPU.ReadLS32(0x1ec);
					u128 wklMinCnt = vm::read128(spurs.addr() + 0x40);
					u128 wklMaxCnt = vm::read128(spurs.addr() + 0x50);
					u128 vCC = u128::andnot(vFMS1, u128::eq8(wklReadyCount0, {}) | u128::leu8(vRC, vAABB)) |
						u128::leu8(wklMaxCnt, vAABB) |
						u128::eq8(savedC, {}) |
						u128::fromV(g_imm_table.fsmb_table[(~var5) >> 16]);
					u128 vCCH1 = u128::andnot(vCC,
						u128::from8p(0x80) & (vFMS1 | u128::gtu8(wklReadyCount0, vAABB)) |
						u128::from8p(0x7f) & savedC);
					u128 vCCL1 = u128::andnot(vCC,
						u128::from8p(0x80) & vFMV1 |
						u128::from8p(0x40) & u128::gtu8(vAABB, {}) & u128::gtu8(wklMinCnt, vAABB) |
						u128::from8p(0x3c) & u128::fromV(_mm_slli_epi32(u128::sub8(u128::from8p(8), vAABB).vi, 2)) |
						u128::from8p(0x02) & u128::eq8(savedD, u128::from8p((u8)var0)) |
						u128::from8p(0x01));
					u128 vSTAT =
						u128::from8p(0x01) & u128::gtu8(wklReadyCount0, vAABB) |
						u128::from8p(0x02) & wklSet1 |
						u128::from8p(0x04) & vFM;

					for (s32 i = 0, max = -1; i < 0x10; i++)
					{
						const s32 value = ((s32)vCCH1.u8r[i] << 8) | ((s32)vCCL1.u8r[i]);
						if (value > max && (vCC.u8r[i] & 1) == 0)
						{
							vNUM = i;
							max = value;
						}
					}

					if (vNUM < 0x10)
					{
						vRES = ((u64)vNUM << 32) | vSTAT.u8r[vNUM];
						vSET.u8r[vNUM] = 0x01;
					}

					SPU.WriteLS8(0x1eb, vNUM == 0x20);

					if (!arg1 || var1 == vNUM)
					{
						spurs->m.wklSet1._and_not(be_t<u16>::make((u16)(vNUM < 16 ? 0x8000 >> vNUM : 0)));
						if (vNUM == flagRecv && wklFlag == 0)
						{
							spurs->m.wklFlag.flag.write_relaxed(be_t<u32>::make(-1));
						}
					}
				}

				if (arg1 == 0)
				{
					vm::write128(spurs.addr() + 0x20, u128::add8(vAA, vSET)); // update wklA

					SPU.WriteLS128(0x180, vSET); // update savedA
					SPU.WriteLS32(0x1dc, vNUM); // update var1
				}

				if (arg1 == 1 && vNUM != var1)
				{
					vm::write128(spurs.addr() + 0x30, u128::add8(vBB, vSET)); // update wklB

					SPU.WriteLS128(0x190, vSET); // update savedB
				}
				else
				{
					vm::write128(spurs.addr() + 0x30, vBB); // update wklB

					SPU.WriteLS128(0x190, {}); // update savedB
				}

				return vRES;
			};
			else SPU.m_code3_func = [spurs, num](SPUThread& SPU) -> u64 // second kernel
			{
				LV2_LOCK(0); // TODO: lock-free implementation if possible

				const u32 arg1 = SPU.GPR[3]._u32[3];
				u32 var0 = SPU.ReadLS32(0x1d8);
				u32 var1 = SPU.ReadLS32(0x1dc);
				u128 wklA = vm::read128(spurs.addr() + 0x20);
				u128 wklB = vm::read128(spurs.addr() + 0x30);
				u128 savedA = SPU.ReadLS128(0x180);
				u128 savedB = SPU.ReadLS128(0x190);
				u128 vAA = u128::sub8(wklA, savedA);
				u128 vBB = u128::sub8(wklB, savedB);
				u128 vM1 = {}; if (var1 <= 31) vM1.u8r[var1 & 0xf] = (var1 <= 15) ? 0xf : 0xf0;
				u128 vAABB = (arg1 == 0) ? vAA : u128::add8(vAA, u128::andnot(vM1, vBB));

				u32 vNUM = 0x20;
				u64 vRES = 0x20ull << 32;
				u128 vSET = {};

				if (spurs->m.x72.read_relaxed() & (1 << num))
				{
					SPU.WriteLS8(0x1eb, 0); // var4
					if (arg1 == 0 || var1 == 0x20)
					{
						spurs->m.x72._and_not(1 << num);
					}
				}
				else
				{
					u128 wklReadyCount0 = vm::read128(spurs.addr() + 0x0);
					u128 wklReadyCount1 = vm::read128(spurs.addr() + 0x10);
					u128 savedC = SPU.ReadLS128(0x1A0);
					u128 wklMaxCnt = vm::read128(spurs.addr() + 0x50);
					u32 wklFlag = spurs->m.wklFlag.flag.read_relaxed();
					u32 flagRecv = spurs->m.flagRecv.read_relaxed();
					u128 wklSet1 = u128::fromV(g_imm_table.fsmb_table[spurs->m.wklSet1.read_relaxed()]);
					u128 wklSet2 = u128::fromV(g_imm_table.fsmb_table[spurs->m.wklSet2.read_relaxed()]);
					u128 vABL = vAABB & u128::from8p(0x0f);
					u128 vABH = u128::fromV(_mm_srli_epi32((vAABB & u128::from8p(0xf0)).vi, 4));
					u32 var5 = SPU.ReadLS32(0x1ec);
					u128 v5L = u128::fromV(g_imm_table.fsmb_table[var5 >> 16]);
					u128 v5H = u128::fromV(g_imm_table.fsmb_table[(u16)var5]);
					u128 vFML = u128::fromV(g_imm_table.fsmb_table[(wklFlag == 0) && (flagRecv < 16) ? 0x8000 >> flagRecv : 0]);
					u128 vFMH = u128::fromV(g_imm_table.fsmb_table[(u16)((wklFlag == 0) && (flagRecv < 32) ? 0x80000000 >> flagRecv : 0)]);
					u128 vCL = u128::fromV(_mm_slli_epi32((savedC & u128::from8p(0x0f)).vi, 4));
					u128 vCH = savedC & u128::from8p(0xf0);
					u128 vABRL = u128::gtu8(wklReadyCount0, vABL);
					u128 vABRH = u128::gtu8(wklReadyCount1, vABH);
					u128 vCCL = v5L & u128::gtu8(vCL, {}) & u128::gtu8(wklMaxCnt & u128::from8p(0x0f), vABL) & (wklSet1 | vFML | vABRL);
					u128 vCCH = v5H & u128::gtu8(vCH, {}) & u128::gtu8(u128::fromV(_mm_srli_epi32((wklMaxCnt & u128::from8p(0xf0)).vi, 4)), vABH) & (wklSet2 | vFMH | vABRH);
					u128 v1H = {}; if (var1 <= 31 && var1 > 15) v1H.u8r[var1 & 0xf] = 4;
					u128 v1L = {}; if (var1 <= 15) v1L.u8r[var1] = 4;
					u128 vCH1 = (v1H | vCH & u128::from8p(0xFB)) & vCCH;
					u128 vCL1 = (v1L | vCL & u128::from8p(0xFB)) & vCCL;
					u128 vSTATL = vABRL & u128::from8p(1) | wklSet1 & u128::from8p(2) | vFML & u128::from8p(4);
					u128 vSTATH = vABRH & u128::from8p(1) | wklSet2 & u128::from8p(2) | vFMH & u128::from8p(4);

					s32 max = -1;
					for (u32 i = 0; i < 0x10; i++)
					{
						const s32 value = vCL1.u8r[i];
						if (value > max && (vCCL.u8r[i] & 1))
						{
							vNUM = i;
							max = value;
						}
					}
					for (u32 i = 16; i < 0x20; i++)
					{
						const s32 value = vCH1.u8r[i];
						if (value > max && (vCCH.u8r[i] & 1))
						{
							vNUM = i;
							max = value;
						}
					}

					if (vNUM < 0x10)
					{
						vRES = ((u64)vNUM << 32) | vSTATL.u8r[vNUM];
						vSET.u8r[vNUM] = 0x01;
					}
					else if (vNUM < 0x20)
					{
						vRES = ((u64)vNUM << 32) | vSTATH.u8r[vNUM & 0xf];
						vSET.u8r[vNUM] = 0x10;
					}

					SPU.WriteLS8(0x1eb, vNUM == 0x20);

					if (!arg1 || var1 == vNUM)
					{
						spurs->m.wklSet1._and_not(be_t<u16>::make((u16)(vNUM < 16 ? 0x8000 >> vNUM : 0)));
						spurs->m.wklSet2._and_not(be_t<u16>::make((u16)(0x80000000 >> vNUM)));
						if (vNUM == flagRecv && wklFlag == 0)
						{
							spurs->m.wklFlag.flag.write_relaxed(be_t<u32>::make(-1));
						}
					}
				}

				if (arg1 == 0)
				{
					vm::write128(spurs.addr() + 0x20, u128::add8(vAA, vSET)); // update wklA

					SPU.WriteLS128(0x180, vSET); // update savedA
					SPU.WriteLS32(0x1dc, vNUM); // update var1
				}

				if (arg1 == 1 && vNUM != var1)
				{
					vm::write128(spurs.addr() + 0x30, u128::add8(vBB, vSET)); // update wklB

					SPU.WriteLS128(0x190, vSET); // update savedB
				}
				else
				{
					vm::write128(spurs.addr() + 0x30, vBB); // update wklB

					SPU.WriteLS128(0x190, {}); // update savedB
				}

				return vRES;
			};
			//SPU.m_code3_func = [spurs, num](SPUThread& SPU) -> u64 // test
			//{
			//	LV2_LOCK(0);
			//	SPU.FastCall(0x290);
			//	u64 vRES = SPU.GPR[3]._u64[1];
			//	return vRES;
			//};

			SPU.WriteLS128(0x1c0, u128::from32r(0, spurs.addr(), num, 0x1f));

			u32 wid = 0x20;
			u32 stat = 0;
			while (true)
			{
				if (Emu.IsStopped())
				{
					cellSpurs->Warning("Spurs Kernel aborted");
					return;
				}

				// get current workload info:
				auto& wkl = wid <= 15 ? spurs->m.wklG1[wid] : (wid <= 31 && isSecond ? spurs->m.wklG2[wid & 0xf] : spurs->m.wklSysG);

				if (SPU.ReadLS64(0x1d0) != wkl.pm.addr())
				{
					// load executable code:
					memcpy(vm::get_ptr<void>(SPU.ls_offset + 0xa00), wkl.pm.get_ptr(), wkl.size);
					SPU.WriteLS64(0x1d0, wkl.pm.addr());
					SPU.WriteLS32(0x1d8, wkl.copy.read_relaxed());
				}
				
				if (!isSecond) SPU.WriteLS16(0x1e8, 0);

				// run workload:
				SPU.GPR[1]._u32[3] = 0x3FFB0;
				SPU.GPR[3]._u32[3] = 0x100;
				SPU.GPR[4]._u64[1] = wkl.data;
				SPU.GPR[5]._u32[3] = stat;
				SPU.FastCall(0xa00);

				// check status:
				auto status = SPU.SPU.Status.GetValue();
				if (status == SPU_STATUS_STOPPED_BY_STOP)
				{
					return;
				}
				else
				{
					assert(status == SPU_STATUS_RUNNING);
				}

				// get workload id:
				SPU.GPR[3].clear();
				assert(SPU.m_code3_func);
				u64 res = SPU.m_code3_func(SPU);
				stat = (u32)(res);
				wid = (u32)(res >> 32);
			}
			
		})->GetId();
	}

	if (flags & SAF_SPU_PRINTF_ENABLED)
	{
		// spu_printf: attach group
		if (!spu_printf_agcb || spu_printf_agcb(tg->m_id) != CELL_OK)
		{
			// remove flag if failed
			spurs->m.flags &= ~SAF_SPU_PRINTF_ENABLED;
		}
	}

	if (s32 res = lwmutex_create(spurs->m.mutex, SYS_SYNC_PRIORITY, SYS_SYNC_NOT_RECURSIVE, *(u64*)"_spuPrv"))
	{
		assert(!"lwmutex_create() failed");
	}
	if (s32 res = lwcond_create(spurs->m.cond, spurs->m.mutex, *(u64*)"_spuPrv"))
	{
		assert(!"lwcond_create() failed");
	}

	spurs->m.flags1 = (flags & SAF_EXIT_IF_NO_WORK ? SF1_EXIT_IF_NO_WORK : 0) | (isSecond ? SF1_IS_SECOND : 0);
	spurs->m.flagRecv.write_relaxed(0xff);
	spurs->m.wklFlag.flag.write_relaxed(be_t<u32>::make(-1));
	spurs->_u8[0xD64] = 0;
	spurs->_u8[0xD65] = 0;
	spurs->_u8[0xD66] = 0;
	spurs->m.ppuPriority = ppuPriority;

	u32 queue;
	if (s32 res = spursCreateLv2EventQueue(spurs, queue, vm::ptr<u8>::make(spurs.addr() + 0xc9), 0x2a, *(u64*)"_spuPrv"))
	{
		assert(!"spursCreateLv2EventQueue() failed");
	}
	spurs->m.queue = queue;

	u32 port = event_port_create(0);
	assert(port && ~port);
	spurs->m.port = port;

	if (s32 res = sys_event_port_connect_local(port, queue))
	{
		assert(!"sys_event_port_connect_local() failed");
	}

	name = std::string(prefix, prefixSize);

	spurs->m.ppu0 = ppu_thread_create(0, 0, ppuPriority, 0x4000, true, false, name + "SpursHdlr0", [spurs](PPUThread& CPU)
	{
#ifdef PRX_DEBUG_XXX
		return cb_call<void, vm::ptr<CellSpurs>>(CPU, libsre + 0x9214, libsre_rtoc, spurs);
#endif

		if (spurs->m.flags & SAF_UNKNOWN_FLAG_30)
		{
			return;
		}

		while (true)
		{
			if (Emu.IsStopped())
			{
				cellSpurs->Warning("SPURS Handler Thread 0 aborted");
				return;
			}

			if (spurs->m.flags1 & SF1_EXIT_IF_NO_WORK)
			{
				if (s32 res = sys_lwmutex_lock(CPU, spurs->get_lwmutex(), 0))
				{
					assert(!"sys_lwmutex_lock() failed");
				}
				if (spurs->m.xD66.read_relaxed())
				{
					if (s32 res = sys_lwmutex_unlock(CPU, spurs->get_lwmutex()))
					{
						assert(!"sys_lwmutex_unlock() failed");
					}
					return;
				}
				else while (true)
				{
					if (Emu.IsStopped()) break;

					spurs->m.xD64.exchange(0);
					if (spurs->m.exception.data() == 0)
					{
						bool do_break = false;
						for (u32 i = 0; i < 16; i++)
						{
							if (spurs->m.wklStat1[i].read_relaxed() == 2 &&
								spurs->m.wklG1[i].priority.data() != 0 &&
								spurs->m.wklMaxCnt[i].read_relaxed() & 0xf
								)
							{
								if (spurs->m.wklReadyCount[i].read_relaxed() ||
									spurs->m.wklSet1.read_relaxed() & (0x8000u >> i) ||
									(spurs->m.wklFlag.flag.read_relaxed() == 0 &&
									spurs->m.flagRecv.read_relaxed() == (u8)i
									))
								{
									do_break = true;
									break;
								}
							}
						}
						if (spurs->m.flags1 & SF1_IS_SECOND) for (u32 i = 0; i < 16; i++)
						{
							if (spurs->m.wklStat2[i].read_relaxed() == 2 &&
								spurs->m.wklG2[i].priority.data() != 0 &&
								spurs->m.wklMaxCnt[i].read_relaxed() & 0xf0
								)
							{
								if (spurs->m.wklReadyCount[i + 0x10].read_relaxed() ||
									spurs->m.wklSet2.read_relaxed() & (0x8000u >> i) ||
									(spurs->m.wklFlag.flag.read_relaxed() == 0 &&
									spurs->m.flagRecv.read_relaxed() == (u8)i + 0x10
									))
								{
									do_break = true;
									break;
								}
							}
						}
						if (do_break) break; // from while
					}

					spurs->m.xD65.exchange(1);
					if (spurs->m.xD64.read_relaxed() == 0)
					{
						if (s32 res = sys_lwcond_wait(CPU, spurs->get_lwcond(), 0))
						{
							assert(!"sys_lwcond_wait() failed");
						}
					}
					spurs->m.xD65.exchange(0);
					if (spurs->m.xD66.read_relaxed())
					{
						if (s32 res = sys_lwmutex_unlock(CPU, spurs->get_lwmutex()))
						{
							assert(!"sys_lwmutex_unlock() failed");
						}
						return;
					}
				}

				if (Emu.IsStopped()) continue;

				if (s32 res = sys_lwmutex_unlock(CPU, spurs->get_lwmutex()))
				{
					assert(!"sys_lwmutex_unlock() failed");
				}
			}

			if (Emu.IsStopped()) continue;

			if (s32 res = sys_spu_thread_group_start(spurs->m.spuTG))
			{
				assert(!"sys_spu_thread_group_start() failed");
			}
			if (s32 res = sys_spu_thread_group_join(spurs->m.spuTG, vm::ptr<u32>::make(0), vm::ptr<u32>::make(0)))
			{
				if (res == CELL_ESTAT)
				{
					return;
				}
				assert(!"sys_spu_thread_group_join() failed");
			}

			if (Emu.IsStopped()) continue;

			if ((spurs->m.flags1 & SF1_EXIT_IF_NO_WORK) == 0)
			{
				assert(spurs->m.xD66.read_relaxed() == 1 || Emu.IsStopped());
				return;
			}
		}
	})->GetId();

	spurs->m.ppu1 = ppu_thread_create(0, 0, ppuPriority, 0x8000, true, false, name + "SpursHdlr1", [spurs](PPUThread& CPU)
	{
#ifdef PRX_DEBUG
		return cb_call<void, vm::ptr<CellSpurs>>(CPU, libsre + 0xB40C, libsre_rtoc, spurs);
#endif

	})->GetId();

	// enable exception event handler
	if (spurs->m.enableEH.compare_and_swap_test(be_t<u32>::make(0), be_t<u32>::make(1)))
	{
		if (s32 res = sys_spu_thread_group_connect_event(spurs->m.spuTG, spurs->m.queue, SYS_SPU_THREAD_GROUP_EVENT_EXCEPTION))
		{
			assert(!"sys_spu_thread_group_connect_event() failed");
		}
	}
	
	spurs->m.unk22 = 0;
	// can also use cellLibprof if available (omitted)

	// some unknown subroutine
	spurs->m.sub3.unk1 = spurs.addr() + 0xc9;
	spurs->m.sub3.unk2 = 3; // unknown const
	spurs->m.sub3.port = (u64)spurs->m.port;

	if (flags & SAF_SYSTEM_WORKLOAD_ENABLED) // initialize system workload
	{
		s32 res = CELL_OK;
#ifdef PRX_DEBUG
		res = cb_call<s32, vm::ptr<CellSpurs>, u32, u32, u32>(GetCurrentPPUThread(), libsre + 0x10428, libsre_rtoc,
			spurs, Memory.RealToVirtualAddr(swlPriority), swlMaxSpu, swlIsPreem);
#endif
		assert(res == CELL_OK);
	}
	else if (flags & SAF_EXIT_IF_NO_WORK) // wakeup
	{
		return spursWakeUp(GetCurrentPPUThread(), spurs);
	}

	return CELL_OK;
}

s64 cellSpursInitialize(vm::ptr<CellSpurs> spurs, s32 nSpus, s32 spuPriority, s32 ppuPriority, bool exitIfNoWork)
{
	cellSpurs->Warning("cellSpursInitialize(spurs_addr=0x%x, nSpus=%d, spuPriority=%d, ppuPriority=%d, exitIfNoWork=%d)",
		spurs.addr(), nSpus, spuPriority, ppuPriority, exitIfNoWork ? 1 : 0);
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x8480, libsre_rtoc);
#endif

	return spursInit(
		spurs,
		0,
		0,
		nSpus,
		spuPriority,
		ppuPriority,
		exitIfNoWork ? SAF_EXIT_IF_NO_WORK : SAF_NONE,
		nullptr,
		0,
		0,
		nullptr,
		0,
		0);
}

s64 cellSpursInitializeWithAttribute(vm::ptr<CellSpurs> spurs, vm::ptr<const CellSpursAttribute> attr)
{
	cellSpurs->Warning("cellSpursInitializeWithAttribute(spurs_addr=0x%x, attr_addr=0x%x)", spurs.addr(), attr.addr());
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x839C, libsre_rtoc);
#endif

	if (!attr)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}
	if (attr.addr() % 8)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}
	if (attr->m.revision > 2)
	{
		return CELL_SPURS_CORE_ERROR_INVAL;
	}
	
	return spursInit(
		spurs,
		attr->m.revision,
		attr->m.sdkVersion,
		attr->m.nSpus,
		attr->m.spuPriority,
		attr->m.ppuPriority,
		attr->m.flags.ToLE() | (attr->m.exitIfNoWork ? SAF_EXIT_IF_NO_WORK : 0),
		attr->m.prefix,
		attr->m.prefixSize,
		attr->m.container,
		attr->m.swlPriority,
		attr->m.swlMaxSpu,
		attr->m.swlIsPreem);
}

s64 cellSpursInitializeWithAttribute2(vm::ptr<CellSpurs> spurs, vm::ptr<const CellSpursAttribute> attr)
{
	cellSpurs->Warning("cellSpursInitializeWithAttribute2(spurs_addr=0x%x, attr_addr=0x%x)", spurs.addr(), attr.addr());
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x82B4, libsre_rtoc);
#endif

	if (!attr)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}
	if (attr.addr() % 8)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}
	if (attr->m.revision > 2)
	{
		return CELL_SPURS_CORE_ERROR_INVAL;
	}

	return spursInit(
		spurs,
		attr->m.revision,
		attr->m.sdkVersion,
		attr->m.nSpus,
		attr->m.spuPriority,
		attr->m.ppuPriority,
		attr->m.flags.ToLE() | (attr->m.exitIfNoWork ? SAF_EXIT_IF_NO_WORK : 0) | SAF_SECOND_VERSION,
		attr->m.prefix,
		attr->m.prefixSize,
		attr->m.container,
		attr->m.swlPriority,
		attr->m.swlMaxSpu,
		attr->m.swlIsPreem);
}

s64 _cellSpursAttributeInitialize(vm::ptr<CellSpursAttribute> attr, u32 revision, u32 sdkVersion, u32 nSpus, s32 spuPriority, s32 ppuPriority, bool exitIfNoWork)
{
	cellSpurs->Warning("_cellSpursAttributeInitialize(attr_addr=0x%x, revision=%d, sdkVersion=0x%x, nSpus=%d, spuPriority=%d, ppuPriority=%d, exitIfNoWork=%d)",
		attr.addr(), revision, sdkVersion, nSpus, spuPriority, ppuPriority, exitIfNoWork ? 1 : 0);
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x72CC, libsre_rtoc);
#endif

	if (!attr)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}
	if (attr.addr() % 8)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	memset(attr.get_ptr(), 0, attr->size);
	attr->m.revision = revision;
	attr->m.sdkVersion = sdkVersion;
	attr->m.nSpus = nSpus;
	attr->m.spuPriority = spuPriority;
	attr->m.ppuPriority = ppuPriority;
	attr->m.exitIfNoWork = exitIfNoWork;
	return CELL_OK;
}

s64 cellSpursAttributeSetMemoryContainerForSpuThread(vm::ptr<CellSpursAttribute> attr, u32 container)
{
	cellSpurs->Warning("cellSpursAttributeSetMemoryContainerForSpuThread(attr_addr=0x%x, container=%d)", attr.addr(), container);
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x6FF8, libsre_rtoc);
#endif

	if (!attr)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}
	if (attr.addr() % 8)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (attr->m.flags.ToLE() & SAF_SPU_TGT_EXCLUSIVE_NON_CONTEXT)
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	attr->m.container = container;
	attr->m.flags |= SAF_SPU_MEMORY_CONTAINER_SET;
	return CELL_OK;
}

s64 cellSpursAttributeSetNamePrefix(vm::ptr<CellSpursAttribute> attr, vm::ptr<const char> prefix, u32 size)
{
	cellSpurs->Warning("cellSpursAttributeSetNamePrefix(attr_addr=0x%x, prefix_addr=0x%x, size=%d)", attr.addr(), prefix.addr(), size);
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x7234, libsre_rtoc);
#endif

	if (!attr || !prefix)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}
	if (attr.addr() % 8)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (size > CELL_SPURS_NAME_MAX_LENGTH)
	{
		return CELL_SPURS_CORE_ERROR_INVAL;
	}

	memcpy(attr->m.prefix, prefix.get_ptr(), size);
	attr->m.prefixSize = size;
	return CELL_OK;
}

s64 cellSpursAttributeEnableSpuPrintfIfAvailable(vm::ptr<CellSpursAttribute> attr)
{
	cellSpurs->Warning("cellSpursAttributeEnableSpuPrintfIfAvailable(attr_addr=0x%x)", attr.addr());
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x7150, libsre_rtoc);
#endif

	if (!attr)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}
	if (attr.addr() % 8)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	attr->m.flags |= SAF_SPU_PRINTF_ENABLED;
	return CELL_OK;
}

s64 cellSpursAttributeSetSpuThreadGroupType(vm::ptr<CellSpursAttribute> attr, s32 type)
{
	cellSpurs->Warning("cellSpursAttributeSetSpuThreadGroupType(attr_addr=0x%x, type=%d)", attr.addr(), type);
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x70C8, libsre_rtoc);
#endif

	if (!attr)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}
	if (attr.addr() % 8)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (type == SYS_SPU_THREAD_GROUP_TYPE_EXCLUSIVE_NON_CONTEXT)
	{
		if (attr->m.flags.ToLE() & SAF_SPU_MEMORY_CONTAINER_SET)
		{
			return CELL_SPURS_CORE_ERROR_STAT;
		}
		attr->m.flags |= SAF_SPU_TGT_EXCLUSIVE_NON_CONTEXT; // set
	}
	else if (type == SYS_SPU_THREAD_GROUP_TYPE_NORMAL)
	{
		attr->m.flags &= ~SAF_SPU_TGT_EXCLUSIVE_NON_CONTEXT; // clear
	}
	else
	{
		return CELL_SPURS_CORE_ERROR_INVAL;
	}
	return CELL_OK;
}

s64 cellSpursAttributeEnableSystemWorkload(vm::ptr<CellSpursAttribute> attr, vm::ptr<const u8[8]> priority, u32 maxSpu, vm::ptr<const bool[8]> isPreemptible)
{
	cellSpurs->Warning("cellSpursAttributeEnableSystemWorkload(attr_addr=0x%x, priority_addr=0x%x, maxSpu=%d, isPreemptible_addr=0x%x)",
		attr.addr(), priority.addr(), maxSpu, isPreemptible.addr());
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0xF410, libsre_rtoc);
#endif

	if (!attr)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}
	if (attr.addr() % 8)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	const u32 nSpus = attr->_u32[2];
	if (!nSpus)
	{
		return CELL_SPURS_CORE_ERROR_INVAL;
	}
	for (u32 i = 0; i < nSpus; i++)
	{
		if ((*priority)[i] == 1)
		{
			if (!maxSpu)
			{
				return CELL_SPURS_CORE_ERROR_INVAL;
			}
			if (nSpus == 1 || attr->m.exitIfNoWork)
			{
				return CELL_SPURS_CORE_ERROR_PERM;
			}
			if (attr->m.flags.ToLE() & SAF_SYSTEM_WORKLOAD_ENABLED)
			{
				return CELL_SPURS_CORE_ERROR_BUSY;
			}

			attr->m.flags |= SAF_SYSTEM_WORKLOAD_ENABLED; // set flag
			*(u64*)attr->m.swlPriority = *(u64*)*priority; // copy system workload priorities

			u32 isPreem = 0; // generate mask from isPreemptible values
			for (u32 j = 0; j < nSpus; j++)
			{
				if ((*isPreemptible)[j])
				{
					isPreem |= (1 << j);
				}
			}
			attr->m.swlMaxSpu = maxSpu; // write max spu for system workload
			attr->m.swlIsPreem = isPreem; // write isPreemptible mask
			return CELL_OK;
		}
	}
	return CELL_SPURS_CORE_ERROR_INVAL;
}

s64 cellSpursFinalize(vm::ptr<CellSpurs> spurs)
{
	cellSpurs->Todo("cellSpursFinalize(spurs_addr=0x%x)", spurs.addr());
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x8568, libsre_rtoc);
#endif

	if (!spurs)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}
	if (spurs.addr() % 128)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}
	if (spurs->m.xD66.read_relaxed())
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}



	return CELL_OK;
}

s64 spursAttachLv2EventQueue(vm::ptr<CellSpurs> spurs, u32 queue, vm::ptr<u8> port, s32 isDynamic, bool wasCreated)
{
#ifdef PRX_DEBUG_XXX
	return cb_call<s32, vm::ptr<CellSpurs>, u32, vm::ptr<u8>, s32, bool>(GetCurrentPPUThread(), libsre + 0xAE34, libsre_rtoc,
		spurs, queue, port, isDynamic, wasCreated);
#endif

	if (!spurs || !port)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}
	if (spurs.addr() % 128)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}
	if (spurs->m.exception.data())
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	s32 sdk_ver;
	if (s32 res = process_get_sdk_version(process_getpid(), sdk_ver))
	{
		assert(!"process_get_sdk_version() failed");
	}
	if (sdk_ver == -1) sdk_ver = 0x460000;

	u8 _port = 0x3f;
	u64 port_mask = 0;

	if (isDynamic == 0)
	{
		_port = *port;
		if (_port > 0x3f)
		{
			return CELL_SPURS_CORE_ERROR_INVAL;
		}
		if (sdk_ver > 0x17ffff && _port > 0xf)
		{
			return CELL_SPURS_CORE_ERROR_PERM;
		}
	}

	for (u32 i = isDynamic ? 0x10 : _port; i <= _port; i++)
	{
		port_mask |= 1ull << (i);
	}

	assert(port_mask); // zero mask will return CELL_EINVAL
	if (s32 res = sys_spu_thread_group_connect_event_all_threads(spurs->m.spuTG, queue, port_mask, port))
	{
		if (res == CELL_EISCONN)
		{
			return CELL_SPURS_CORE_ERROR_BUSY;
		}
		return res;
	}

	if (!wasCreated)
	{
		spurs->m.spups |= be_t<u64>::make(1ull << *port); // atomic bitwise or
	}
	return CELL_OK;
}

s64 cellSpursAttachLv2EventQueue(vm::ptr<CellSpurs> spurs, u32 queue, vm::ptr<u8> port, s32 isDynamic)
{
	cellSpurs->Warning("cellSpursAttachLv2EventQueue(spurs_addr=0x%x, queue=%d, port_addr=0x%x, isDynamic=%d)",
		spurs.addr(), queue, port.addr(), isDynamic);
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0xAFE0, libsre_rtoc);
#endif

	return spursAttachLv2EventQueue(spurs, queue, port, isDynamic, false);
}

s64 cellSpursDetachLv2EventQueue(vm::ptr<CellSpurs> spurs, u8 port)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursDetachLv2EventQueue(spurs_addr=0x%x, port=%d)", spurs.addr(), port);
	return GetCurrentPPUThread().FastCall2(libsre + 0xB144, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursGetSpuGuid()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xEFB0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursGetSpuThreadGroupId(vm::ptr<CellSpurs> spurs, vm::ptr<u32> group)
{
	cellSpurs->Warning("cellSpursGetSpuThreadGroupId(spurs_addr=0x%x, group_addr=0x%x)", spurs.addr(), group.addr());
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x8B30, libsre_rtoc);
#endif

	if (!spurs || !group)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}
	if (spurs.addr() % 128)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	*group = spurs->m.spuTG;
	return CELL_OK;
}

s64 cellSpursGetNumSpuThread(vm::ptr<CellSpurs> spurs, vm::ptr<u32> nThreads)
{
	cellSpurs->Warning("cellSpursGetNumSpuThread(spurs_addr=0x%x, nThreads_addr=0x%x)", spurs.addr(), nThreads.addr());
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x8B78, libsre_rtoc);
#endif

	if (!spurs || !nThreads)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}
	if (spurs.addr() % 128)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	*nThreads = (u32)spurs->m.nSpus;
	return CELL_OK;
}

s64 cellSpursGetSpuThreadId(vm::ptr<CellSpurs> spurs, vm::ptr<u32> thread, vm::ptr<u32> nThreads)
{
	cellSpurs->Warning("cellSpursGetSpuThreadId(spurs_addr=0x%x, thread_addr=0x%x, nThreads_addr=0x%x)", spurs.addr(), thread.addr(), nThreads.addr());
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x8A98, libsre_rtoc);
#endif

	if (!spurs || !thread || !nThreads)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}
	if (spurs.addr() % 128)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	const u32 count = std::min<u32>(*nThreads, spurs->m.nSpus);
	for (u32 i = 0; i < count; i++)
	{
		thread[i] = spurs->m.spus[i];
	}
	*nThreads = count;
	return CELL_OK;
}

s64 cellSpursSetMaxContention(vm::ptr<CellSpurs> spurs, u32 workloadId, u32 maxContention)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursSetMaxContention(spurs_addr=0x%x, workloadId=%d, maxContention=%d)", spurs.addr(), workloadId, maxContention);
	return GetCurrentPPUThread().FastCall2(libsre + 0x8E90, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursSetPriorities(vm::ptr<CellSpurs> spurs, u32 workloadId, vm::ptr<const u8> priorities)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursSetPriorities(spurs_addr=0x%x, workloadId=%d, priorities_addr=0x%x)", spurs.addr(), workloadId, priorities.addr());
	return GetCurrentPPUThread().FastCall2(libsre + 0x8BC0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursSetPreemptionVictimHints(vm::ptr<CellSpurs> spurs, vm::ptr<const bool> isPreemptible)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursSetPreemptionVictimHints(spurs_addr=0x%x, isPreemptible_addr=0x%x)", spurs.addr(), isPreemptible.addr());
	return GetCurrentPPUThread().FastCall2(libsre + 0xF5A4, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursEnableExceptionEventHandler(vm::ptr<CellSpurs> spurs, bool flag)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursEnableExceptionEventHandler(spurs_addr=0x%x, flag=%d)", spurs.addr(), flag);
	return GetCurrentPPUThread().FastCall2(libsre + 0xDCC0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursSetGlobalExceptionEventHandler(vm::ptr<CellSpurs> spurs, u32 eaHandler_addr, u32 arg_addr)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursSetGlobalExceptionEventHandler(spurs_addr=0x%x, eaHandler_addr=0x%x, arg_addr=0x%x)",
		spurs.addr(), eaHandler_addr, arg_addr);
	return GetCurrentPPUThread().FastCall2(libsre + 0xD6D0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursUnsetGlobalExceptionEventHandler(vm::ptr<CellSpurs> spurs)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursUnsetGlobalExceptionEventHandler(spurs_addr=0x%x)", spurs.addr());
	return GetCurrentPPUThread().FastCall2(libsre + 0xD674, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursGetInfo(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursInfo> info)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursGetInfo(spurs_addr=0x%x, info_addr=0x%x)", spurs.addr(), info.addr());
	return GetCurrentPPUThread().FastCall2(libsre + 0xE540, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 spursWakeUp(PPUThread& CPU, vm::ptr<CellSpurs> spurs)
{
#ifdef PRX_DEBUG_XXX
	return cb_call<s32, vm::ptr<CellSpurs>>(GetCurrentPPUThread(), libsre + 0x84D8, libsre_rtoc, spurs);
#endif

	if (!spurs)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}
	if (spurs.addr() % 128)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}
	if (spurs->m.exception.data())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}

	spurs->m.xD64.exchange(1);
	if (spurs->m.xD65.read_sync())
	{
		if (s32 res = sys_lwmutex_lock(CPU, spurs->get_lwmutex(), 0))
		{
			assert(!"sys_lwmutex_lock() failed");
		}
		if (s32 res = sys_lwcond_signal(spurs->get_lwcond()))
		{
			assert(!"sys_lwcond_signal() failed");
		}
		if (s32 res = sys_lwmutex_unlock(CPU, spurs->get_lwmutex()))
		{
			assert(!"sys_lwmutex_unlock() failed");
		}
	}
	return CELL_OK;
}

s64 cellSpursWakeUp(PPUThread& CPU, vm::ptr<CellSpurs> spurs)
{
	cellSpurs->Warning("%s(spurs_addr=0x%x)", __FUNCTION__, spurs.addr());

	return spursWakeUp(CPU, spurs);
}

s32 spursAddWorkload(
	vm::ptr<CellSpurs> spurs,
	vm::ptr<u32> wid,
	vm::ptr<const void> pm,
	u32 size,
	u64 data,
	const u8 priorityTable[],
	u32 minContention,
	u32 maxContention,
	vm::ptr<const char> nameClass,
	vm::ptr<const char> nameInstance,
	vm::ptr<CellSpursShutdownCompletionEventHook> hook,
	vm::ptr<void> hookArg)
{
#ifdef PRX_DEBUG_XXX
	return cb_call<s32, vm::ptr<CellSpurs>, vm::ptr<u32>, vm::ptr<const void>, u32, u64, u32, u32, u32, u32, u32, u32, u32>(GetCurrentPPUThread(), libsre + 0x96EC, libsre_rtoc,
		spurs, wid, pm, size, data, Memory.RealToVirtualAddr(priorityTable), minContention, maxContention,
		nameClass.addr(), nameInstance.addr(), hook.addr(), hookArg.addr());
#endif

	if (!spurs || !wid || !pm)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}
	if (spurs.addr() % 128 || pm.addr() % 16)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}
	if (minContention == 0 || *(u64*)priorityTable & 0xf0f0f0f0f0f0f0f0ull) // check if some priority > 15
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;
	}
	if (spurs->m.exception.data())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}
	
	u32 wnum;
	const u32 wmax = spurs->m.flags1 & SF1_IS_SECOND ? 0x20u : 0x10u; // TODO: check if can be changed
	spurs->m.wklMskA.atomic_op([spurs, wmax, &wnum](be_t<u32>& value)
	{
		wnum = cntlz32(~(u32)value); // found empty position
		if (wnum < wmax)
		{
			value |= (u32)(0x80000000ull >> wnum); // set workload bit
		}
	});

	*wid = wnum; // store workload id
	if (wnum >= wmax)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_AGAIN;
	}

	u32 index = wnum & 0xf;
	if (wnum <= 15)
	{
		assert((spurs->m.wklA[wnum] & 0xf) == 0);
		assert((spurs->m.wklB[wnum] & 0xf) == 0);
		spurs->m.wklStat1[wnum].write_relaxed(1);
		spurs->m.wklD1[wnum] = 0;
		spurs->m.wklE1[wnum] = 0;
		spurs->m.wklG1[wnum].pm = pm;
		spurs->m.wklG1[wnum].data = data;
		spurs->m.wklG1[wnum].size = size;
		spurs->m.wklG1[wnum].priority = *(be_t<u64>*)priorityTable;
		spurs->m.wklH1[wnum].nameClass = nameClass;
		spurs->m.wklH1[wnum].nameInstance = nameInstance;
		memset(spurs->m.wklF1[wnum].unk0, 0, 0x20); // clear struct preserving semaphore id
		memset(spurs->m.wklF1[wnum].unk1, 0, 0x58);
		if (hook)
		{
			spurs->m.wklF1[wnum].hook = hook;
			spurs->m.wklF1[wnum].hookArg = hookArg;
			spurs->m.wklE1[wnum] |= 2;
		}
		if ((spurs->m.flags1 & SF1_IS_SECOND) == 0)
		{
			spurs->m.wklReadyCount[wnum + 16].write_relaxed(0);
			spurs->m.wklMinCnt[wnum] = minContention > 8 ? 8 : minContention;
		}
	}
	else
	{
		assert((spurs->m.wklA[index] & 0xf0) == 0);
		assert((spurs->m.wklB[index] & 0xf0) == 0);
		spurs->m.wklStat2[index].write_relaxed(1);
		spurs->m.wklD2[index] = 0;
		spurs->m.wklE2[index] = 0;
		spurs->m.wklG2[index].pm = pm;
		spurs->m.wklG2[index].data = data;
		spurs->m.wklG2[index].size = size;
		spurs->m.wklG2[index].priority = *(be_t<u64>*)priorityTable;
		spurs->m.wklH2[index].nameClass = nameClass;
		spurs->m.wklH2[index].nameInstance = nameInstance;
		memset(spurs->m.wklF2[index].unk0, 0, 0x20); // clear struct preserving semaphore id
		memset(spurs->m.wklF2[index].unk1, 0, 0x58);
		if (hook)
		{
			spurs->m.wklF2[index].hook = hook;
			spurs->m.wklF2[index].hookArg = hookArg;
			spurs->m.wklE2[index] |= 2;
		}
	}
	spurs->m.wklReadyCount[wnum].write_relaxed(0);

	if (wnum <= 15)
	{
		spurs->m.wklMaxCnt[wnum].atomic_op([maxContention](u8& v)
		{
			v &= ~0xf;
			v |= (maxContention > 8 ? 8 : maxContention);
		});
		spurs->m.wklSet1._and_not({ be_t<u16>::make(0x8000 >> index) }); // clear bit in wklFlag1
	}
	else
	{
		spurs->m.wklMaxCnt[index].atomic_op([maxContention](u8& v)
		{
			v &= ~0xf0;
			v |= (maxContention > 8 ? 8 : maxContention) << 4;
		});
		spurs->m.wklSet2._and_not({ be_t<u16>::make(0x8000 >> index) }); // clear bit in wklFlag2
	}

	spurs->m.flagRecv.compare_and_swap(wnum, 0xff);

	u32 res_wkl;
	CellSpurs::_sub_str3& wkl = wnum <= 15 ? spurs->m.wklG1[wnum] : spurs->m.wklG2[wnum & 0xf];
	spurs->m.wklMskB.atomic_op_sync([spurs, &wkl, wnum, &res_wkl](be_t<u32>& v)
	{
		const u32 mask = v.ToLE() & ~(0x80000000u >> wnum);
		res_wkl = 0;

		for (u32 i = 0, m = 0x80000000, k = 0; i < 32; i++, m >>= 1)
		{
			if (mask & m)
			{
				CellSpurs::_sub_str3& current = i <= 15 ? spurs->m.wklG1[i] : spurs->m.wklG2[i & 0xf];
				if (current.pm.addr() == wkl.pm.addr())
				{
					// if a workload with identical policy module found
					res_wkl = current.copy.read_relaxed();
					break;
				}
				else
				{
					k |= 0x80000000 >> current.copy.read_relaxed();
					res_wkl = cntlz32(~k);
				}
			}
		}

		wkl.copy.exchange((u8)res_wkl);
		v = mask | (0x80000000u >> wnum);
	});
	assert(res_wkl <= 31);

	spurs->wklStat(wnum).exchange(2);
	spurs->m.xBD.exchange(0xff);
	spurs->m.x72.exchange(0xff);
	return CELL_OK;
}

s64 cellSpursAddWorkload(
	vm::ptr<CellSpurs> spurs,
	vm::ptr<u32> wid,
	vm::ptr<const void> pm,
	u32 size,
	u64 data,
	vm::ptr<const u8[8]> priorityTable,
	u32 minContention,
	u32 maxContention)
{
	cellSpurs->Warning("%s(spurs_addr=0x%x, wid_addr=0x%x, pm_addr=0x%x, size=0x%x, data=0x%llx, priorityTable_addr=0x%x, minContention=0x%x, maxContention=0x%x)",
		__FUNCTION__, spurs.addr(), wid.addr(), pm.addr(), size, data, priorityTable.addr(), minContention, maxContention);
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x9ED0, libsre_rtoc);
#endif

	return spursAddWorkload(
		spurs,
		wid,
		pm,
		size,
		data,
		*priorityTable,
		minContention,
		maxContention,
		vm::ptr<const char>::make(0),
		vm::ptr<const char>::make(0),
		vm::ptr<CellSpursShutdownCompletionEventHook>::make(0),
		vm::ptr<void>::make(0));
}

s64 _cellSpursWorkloadAttributeInitialize(
	vm::ptr<CellSpursWorkloadAttribute> attr,
	u32 revision,
	u32 sdkVersion,
	vm::ptr<const void> pm,
	u32 size,
	u64 data,
	vm::ptr<const u8[8]> priorityTable,
	u32 minContention,
	u32 maxContention)
{
	cellSpurs->Warning("%s(attr_addr=0x%x, revision=%d, sdkVersion=0x%x, pm_addr=0x%x, size=0x%x, data=0x%llx, priorityTable_addr=0x%x, minContention=0x%x, maxContention=0x%x)",
		__FUNCTION__, attr.addr(), revision, sdkVersion, pm.addr(), size, data, priorityTable.addr(), minContention, maxContention);
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x9F08, libsre_rtoc);
#endif

	if (!attr)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}
	if (attr.addr() % 8)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}
	if (!pm)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}
	if (pm.addr() % 16)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}
	if (minContention == 0 || *(u64*)*priorityTable & 0xf0f0f0f0f0f0f0f0ull) // check if some priority > 15
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;
	}
	
	memset(attr.get_ptr(), 0, CellSpursWorkloadAttribute::size);
	attr->m.revision = revision;
	attr->m.sdkVersion = sdkVersion;
	attr->m.pm = pm;
	attr->m.size = size;
	attr->m.data = data;
	*(u64*)attr->m.priority = *(u64*)*priorityTable;
	attr->m.minContention = minContention;
	attr->m.maxContention = maxContention;
	return CELL_OK;
}

s64 cellSpursWorkloadAttributeSetName(vm::ptr<CellSpursWorkloadAttribute> attr, vm::ptr<const char> nameClass, vm::ptr<const char> nameInstance)
{
	cellSpurs->Warning("%s(attr_addr=0x%x, nameClass_addr=0x%x, nameInstance_addr=0x%x)", __FUNCTION__, attr.addr(), nameClass.addr(), nameInstance.addr());
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x9664, libsre_rtoc);
#endif

	if (!attr)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}
	if (attr.addr() % 8)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	attr->m.nameClass = nameClass;
	attr->m.nameInstance = nameInstance;
	return CELL_OK;
}

s64 cellSpursWorkloadAttributeSetShutdownCompletionEventHook(vm::ptr<CellSpursWorkloadAttribute> attr, vm::ptr<CellSpursShutdownCompletionEventHook> hook, vm::ptr<void> arg)
{
	cellSpurs->Warning("%s(attr_addr=0x%x, hook_addr=0x%x, arg=0x%x)", __FUNCTION__, attr.addr(), hook.addr(), arg.addr());
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x96A4, libsre_rtoc);
#endif

	if (!attr || !hook)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}
	if (attr.addr() % 8)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	attr->m.hook = hook;
	attr->m.hookArg = arg;
	return CELL_OK;
}

s64 cellSpursAddWorkloadWithAttribute(vm::ptr<CellSpurs> spurs, vm::ptr<u32> wid, vm::ptr<const CellSpursWorkloadAttribute> attr)
{
	cellSpurs->Warning("%s(spurs_addr=0x%x, wid_addr=0x%x, attr_addr=0x%x)", __FUNCTION__, spurs.addr(), wid.addr(), attr.addr());
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x9E14, libsre_rtoc);
#endif

	if (!attr)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}
	if (attr.addr() % 8)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}
	if (attr->m.revision != be_t<u32>::make(1))
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;
	}

	return spursAddWorkload(
		spurs,
		wid,
		vm::ptr<const void>::make(attr->m.pm.addr()),
		attr->m.size,
		attr->m.data,
		attr->m.priority,
		attr->m.minContention,
		attr->m.maxContention,
		vm::ptr<const char>::make(attr->m.nameClass.addr()),
		vm::ptr<const char>::make(attr->m.nameInstance.addr()),
		vm::ptr<CellSpursShutdownCompletionEventHook>::make(attr->m.hook.addr()),
		vm::ptr<void>::make(attr->m.hookArg.addr()));
}

s64 cellSpursRemoveWorkload()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xA414, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursWaitForWorkloadShutdown()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xA20C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursShutdownWorkload()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xA060, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursWorkloadFlagReceiver(vm::ptr<CellSpurs> spurs, u32 wid, u32 is_set)
{
	cellSpurs->Warning("%s(spurs_addr=0x%x, wid=%d, is_set=%d)", __FUNCTION__, spurs.addr(), wid, is_set);
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0xF158, libsre_rtoc);
#endif

	if (!spurs)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}
	if (spurs.addr() % 128)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}
	if (wid >= (spurs->m.flags1 & SF1_IS_SECOND ? 0x20u : 0x10u))
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;
	}
	if ((spurs->m.wklMskA.read_relaxed().ToLE() & (0x80000000u >> wid)) == 0)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_SRCH;
	}
	if (spurs->m.exception.data())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}
	if (s32 res = spurs->m.wklFlag.flag.atomic_op_sync(0, [spurs, wid, is_set](be_t<u32>& flag) -> s32
	{
		if (is_set)
		{
			if (spurs->m.flagRecv.read_relaxed() != 0xff)
			{
				return CELL_SPURS_POLICY_MODULE_ERROR_BUSY;
			}
		}
		else
		{
			if (spurs->m.flagRecv.read_relaxed() != wid)
			{
				return CELL_SPURS_POLICY_MODULE_ERROR_PERM;
			}
		}
		flag = -1;
		return 0;
	}))
	{
		return res;
	}

	spurs->m.flagRecv.atomic_op([wid, is_set](u8& FR)
	{
		if (is_set)
		{
			if (FR == 0xff)
			{
				FR = (u8)wid;
			}
		}
		else
		{
			if (FR == wid)
			{
				FR = 0xff;
			}
		}
	});
	return CELL_OK;
}

s64 cellSpursGetWorkloadFlag(vm::ptr<CellSpurs> spurs, vm::ptr<vm::bptr<CellSpursWorkloadFlag>> flag)
{
	cellSpurs->Warning("%s(spurs_addr=0x%x, flag_addr=0x%x)", __FUNCTION__, spurs.addr(), flag.addr());
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0xEC00, libsre_rtoc);
#endif

	if (!spurs || !flag)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}
	if (spurs.addr() % 128)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	flag->set(Memory.RealToVirtualAddr(&spurs->m.wklFlag));
	return CELL_OK;
}

s64 cellSpursSendWorkloadSignal()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xA658, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursGetWorkloadData()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xA78C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursReadyCountStore(vm::ptr<CellSpurs> spurs, u32 wid, u32 value)
{
	cellSpurs->Warning("%s(spurs_addr=0x%x, wid=%d, value=0x%x)", __FUNCTION__, spurs.addr(), wid, value);
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0xAB2C, libsre_rtoc);
#endif

	if (!spurs)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}
	if (spurs.addr() % 128)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}
	if (wid >= (spurs->m.flags1 & SF1_IS_SECOND ? 0x20u : 0x10u) || value > 0xff)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;
	}
	if ((spurs->m.wklMskA.read_relaxed().ToLE() & (0x80000000u >> wid)) == 0)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_SRCH;
	}
	if (spurs->m.exception.data() || spurs->wklStat(wid).read_relaxed() != 2)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}

	spurs->m.wklReadyCount[wid].exchange((u8)value);
	return CELL_OK;
}

s64 cellSpursReadyCountAdd()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xA868, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursReadyCountCompareAndSwap()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xA9CC, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursReadyCountSwap()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xAC34, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursRequestIdleSpu()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xAD88, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursGetWorkloadInfo()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xE70C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursWorkloadFlagReceiver2()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xF298, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursSetExceptionEventHandler()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xDB54, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursUnsetExceptionEventHandler()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xD77C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursEventFlagInitialize(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, vm::ptr<CellSpursEventFlag> eventFlag, u32 flagClearMode, u32 flagDirection)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("_cellSpursEventFlagInitialize(spurs_addr=0x%x, taskset_addr=0x%x, eventFlag_addr=0x%x, flagClearMode=%d, flagDirection=%d)",
		spurs.addr(), taskset.addr(), eventFlag.addr(), flagClearMode, flagDirection);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1564C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursEventFlagAttachLv2EventQueue(vm::ptr<CellSpursEventFlag> eventFlag)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursEventFlagAttachLv2EventQueue(eventFlag_addr=0x%x)", eventFlag.addr());
	return GetCurrentPPUThread().FastCall2(libsre + 0x157B8, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursEventFlagDetachLv2EventQueue(vm::ptr<CellSpursEventFlag> eventFlag)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursEventFlagDetachLv2EventQueue(eventFlag_addr=0x%x)", eventFlag.addr());
	return GetCurrentPPUThread().FastCall2(libsre + 0x15998, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursEventFlagWait(vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u16> mask, u32 mode)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursEventFlagWait(eventFlag_addr=0x%x, mask_addr=0x%x, mode=%d)", eventFlag.addr(), mask.addr(), mode);
	return GetCurrentPPUThread().FastCall2(libsre + 0x15E68, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursEventFlagClear(vm::ptr<CellSpursEventFlag> eventFlag, u16 bits)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursEventFlagClear(eventFlag_addr=0x%x, bits=0x%x)", eventFlag.addr(), bits);
	return GetCurrentPPUThread().FastCall2(libsre + 0x15E9C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursEventFlagSet(vm::ptr<CellSpursEventFlag> eventFlag, u16 bits)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursEventFlagSet(eventFlag_addr=0x%x, bits=0x%x)", eventFlag.addr(), bits);
	return GetCurrentPPUThread().FastCall2(libsre + 0x15F04, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursEventFlagTryWait(vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u16> mask, u32 mode)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursEventFlagTryWait(eventFlag_addr=0x%x, mask_addr=0x%x, mode=0x%x)", eventFlag.addr(), mask.addr(), mode);
	return GetCurrentPPUThread().FastCall2(libsre + 0x15E70, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursEventFlagGetDirection(vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u32> direction)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursEventFlagGetDirection(eventFlag_addr=0x%x, direction_addr=0x%x)", eventFlag.addr(), direction.addr());
	return GetCurrentPPUThread().FastCall2(libsre + 0x162C4, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursEventFlagGetClearMode(vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u32> clear_mode)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursEventFlagGetClearMode(eventFlag_addr=0x%x, clear_mode_addr=0x%x)", eventFlag.addr(), clear_mode.addr());
	return GetCurrentPPUThread().FastCall2(libsre + 0x16310, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursEventFlagGetTasksetAddress(vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<CellSpursTaskset> taskset)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursEventFlagGetTasksetAddress(eventFlag_addr=0x%x, taskset_addr=0x%x)", eventFlag.addr(), taskset.addr());
	return GetCurrentPPUThread().FastCall2(libsre + 0x1635C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursLFQueueInitialize()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x17028, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursLFQueuePushBody()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x170AC, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursLFQueueDetachLv2EventQueue()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x177CC, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursLFQueueAttachLv2EventQueue()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x173EC, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursLFQueuePopBody()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x17238, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursLFQueueGetTasksetAddress()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x17C34, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursQueueInitialize()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x163B4, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursQueuePopBody()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x16BF0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursQueuePushBody()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x168C4, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursQueueAttachLv2EventQueue()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1666C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursQueueDetachLv2EventQueue()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x16524, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursQueueGetTasksetAddress()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x16F50, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursQueueClear()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1675C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursQueueDepth()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1687C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursQueueGetEntrySize()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x16FE0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursQueueSize()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x167F0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursQueueGetDirection()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x16F98, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursCreateJobChainWithAttribute()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1898C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursCreateJobChain()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x18B84, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursJoinJobChain()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x18DB0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursKickJobChain()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x18E8C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursJobChainAttributeInitialize()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1845C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursGetJobChainId()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x19064, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursJobChainSetExceptionEventHandler()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1A5A0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursJobChainUnsetExceptionEventHandler()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1A614, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursGetJobChainInfo()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1A7A0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursJobChainGetSpursAddress()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1A900, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursCreateTasksetWithAttribute()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x14BEC, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursCreateTaskset(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, u64 args, vm::ptr<const u8> priority, u32 maxContention)
{
	cellSpurs->Warning("cellSpursCreateTaskset(spurs_addr=0x%x, taskset_addr=0x%x, args=0x%llx, priority_addr=0x%x, maxContention=%d)",
		spurs.addr(), taskset.addr(), args, priority.addr(), maxContention);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x14CB8, libsre_rtoc);
#else
	SPURSManagerTasksetAttribute *tattr = new SPURSManagerTasksetAttribute(args, priority, maxContention);
	taskset->taskset = new SPURSManagerTaskset(taskset.addr(), tattr);

	return CELL_OK;
#endif
}

s64 cellSpursJoinTaskset(vm::ptr<CellSpursTaskset> taskset)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursJoinTaskset(taskset_addr=0x%x)", taskset.addr());
	return GetCurrentPPUThread().FastCall2(libsre + 0x152F8, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursGetTasksetId(vm::ptr<CellSpursTaskset> taskset, vm::ptr<u32> workloadId)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursGetTasksetId(taskset_addr=0x%x, workloadId_addr=0x%x)", taskset.addr(), workloadId.addr());
	return GetCurrentPPUThread().FastCall2(libsre + 0x14EA0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursShutdownTaskset(vm::ptr<CellSpursTaskset> taskset)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursShutdownTaskset(taskset_addr=0x%x)", taskset.addr());
	return GetCurrentPPUThread().FastCall2(libsre + 0x14868, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursCreateTask(vm::ptr<CellSpursTaskset> taskset, vm::ptr<u32> taskID, u32 elf_addr, u32 context_addr, u32 context_size, vm::ptr<CellSpursTaskLsPattern> lsPattern,
	vm::ptr<CellSpursTaskArgument> argument)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursCreateTask(taskset_addr=0x%x, taskID_addr=0x%x, elf_addr_addr=0x%x, context_addr_addr=0x%x, context_size=%d, lsPattern_addr=0x%x, argument_addr=0x%x)",
		taskset.addr(), taskID.addr(), elf_addr, context_addr, context_size, lsPattern.addr(), argument.addr());
	return GetCurrentPPUThread().FastCall2(libsre + 0x12414, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursSendSignal(vm::ptr<CellSpursTaskset> taskset, u32 taskID)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("_cellSpursSendSignal(taskset_addr=0x%x, taskID=%d)", taskset.addr(), taskID);
	return GetCurrentPPUThread().FastCall2(libsre + 0x124CC, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursCreateTaskWithAttribute()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x12204, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTasksetAttributeSetName()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x14210, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTasksetAttributeSetTasksetSize()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x14254, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTasksetAttributeEnableClearLS()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x142AC, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursTasksetAttribute2Initialize(vm::ptr<CellSpursTasksetAttribute2> attribute, u32 revision)
{
	cellSpurs->Warning("_cellSpursTasksetAttribute2Initialize(attribute_addr=0x%x, revision=%d)", attribute.addr(), revision);
	
#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x1474C, libsre_rtoc);
#else
	attribute->revision = revision;
	attribute->name_addr = 0;
	attribute->argTaskset = 0;

	for (s32 i = 0; i < 8; i++)
	{
		attribute->priority[i] = 1;
	}

	attribute->maxContention = 8;
	attribute->enableClearLs = 0;
	attribute->CellSpursTaskNameBuffer_addr = 0;

	return CELL_OK;
#endif
}

s64 cellSpursTaskExitCodeGet()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1397C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTaskExitCodeInitialize()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1352C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTaskExitCodeTryGet()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x13974, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTaskGetLoadableSegmentPattern()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x13ED4, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTaskGetReadOnlyAreaPattern()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x13CFC, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTaskGenerateLsPattern()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x13B78, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursTaskAttributeInitialize()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x10C30, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTaskAttributeSetExitCodeContainer()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x10A98, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursTaskAttribute2Initialize(vm::ptr<CellSpursTaskAttribute2> attribute, u32 revision)
{
	cellSpurs->Warning("_cellSpursTaskAttribute2Initialize(attribute_addr=0x%x, revision=%d)", attribute.addr(), revision);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x10B00, libsre_rtoc);
#else
	attribute->revision = revision;
	attribute->sizeContext = 0;
	attribute->eaContext = 0;
	
	for (s32 c = 0; c < 4; c++)
	{
		attribute->lsPattern.u32[c] = 0;
	}

	for (s32 i = 0; i < 2; i++)
	{
		attribute->lsPattern.u64[i] = 0;
	}

	attribute->name_addr = 0;

	return CELL_OK;
#endif
}

s64 cellSpursTaskGetContextSaveAreaSize()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1409C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursCreateTaskset2()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x15108, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursCreateTask2()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x11E54, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursJoinTask2()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x11378, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTryJoinTask2()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x11748, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursDestroyTaskset2()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x14EE8, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursCreateTask2WithBinInfo()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x120E0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTasksetSetExceptionEventHandler()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x13124, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTasksetUnsetExceptionEventHandler()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x13194, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursLookUpTasksetAddress()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x133AC, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTasksetGetSpursAddress()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x14408, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursGetTasksetInfo()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1445C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursTasksetAttributeInitialize()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x142FC, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursJobGuardInitialize()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1807C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursJobChainAttributeSetName()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1861C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursShutdownJobChain()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x18D2C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursJobChainAttributeSetHaltOnError()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x18660, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursJobChainAttributeSetJobTypeMemoryCheck()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x186A4, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursJobGuardNotify()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x17FA4, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursJobGuardReset()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x17F60, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursRunJobChain()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x18F94, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursJobChainGetError()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x190AC, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursGetJobPipelineInfo()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1A954, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursJobSetMaxGrab()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1AC88, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursJobHeaderSetJobbin2Param()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1AD58, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursAddUrgentCommand()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x18160, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursAddUrgentCall()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1823C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursBarrierInitialize()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x17CD8, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursBarrierGetTasksetAddress()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x17DB0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursSemaphoreInitialize()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x17DF8, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursSemaphoreGetTasksetAddress()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x17F18, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

void cellSpurs_init(Module *pxThis)
{
	cellSpurs = pxThis;

	// Core 
	REG_FUNC(cellSpurs, cellSpursInitialize);
	REG_FUNC(cellSpurs, cellSpursInitializeWithAttribute);
	REG_FUNC(cellSpurs, cellSpursInitializeWithAttribute2);
	REG_FUNC(cellSpurs, cellSpursFinalize);
	REG_FUNC(cellSpurs, _cellSpursAttributeInitialize);
	REG_FUNC(cellSpurs, cellSpursAttributeSetMemoryContainerForSpuThread);
	REG_FUNC(cellSpurs, cellSpursAttributeSetNamePrefix);
	REG_FUNC(cellSpurs, cellSpursAttributeEnableSpuPrintfIfAvailable);
	REG_FUNC(cellSpurs, cellSpursAttributeSetSpuThreadGroupType);
	REG_FUNC(cellSpurs, cellSpursAttributeEnableSystemWorkload);
	REG_FUNC(cellSpurs, cellSpursGetSpuThreadGroupId);
	REG_FUNC(cellSpurs, cellSpursGetNumSpuThread);
	REG_FUNC(cellSpurs, cellSpursGetSpuThreadId);
	REG_FUNC(cellSpurs, cellSpursGetInfo);
	REG_FUNC(cellSpurs, cellSpursSetMaxContention);
	REG_FUNC(cellSpurs, cellSpursSetPriorities);
	REG_FUNC(cellSpurs, cellSpursSetPreemptionVictimHints);
	REG_FUNC(cellSpurs, cellSpursAttachLv2EventQueue);
	REG_FUNC(cellSpurs, cellSpursDetachLv2EventQueue);
	REG_FUNC(cellSpurs, cellSpursEnableExceptionEventHandler);
	REG_FUNC(cellSpurs, cellSpursSetGlobalExceptionEventHandler);
	REG_FUNC(cellSpurs, cellSpursUnsetGlobalExceptionEventHandler);

	// Event flag
	REG_FUNC(cellSpurs, _cellSpursEventFlagInitialize);
	REG_FUNC(cellSpurs, cellSpursEventFlagAttachLv2EventQueue);
	REG_FUNC(cellSpurs, cellSpursEventFlagDetachLv2EventQueue);
	REG_FUNC(cellSpurs, cellSpursEventFlagWait);
	REG_FUNC(cellSpurs, cellSpursEventFlagClear);
	REG_FUNC(cellSpurs, cellSpursEventFlagSet);
	REG_FUNC(cellSpurs, cellSpursEventFlagTryWait);
	REG_FUNC(cellSpurs, cellSpursEventFlagGetDirection);
	REG_FUNC(cellSpurs, cellSpursEventFlagGetClearMode);
	REG_FUNC(cellSpurs, cellSpursEventFlagGetTasksetAddress);

	// Taskset
	REG_FUNC(cellSpurs, cellSpursCreateTaskset);
	REG_FUNC(cellSpurs, cellSpursCreateTasksetWithAttribute);
	REG_FUNC(cellSpurs, _cellSpursTasksetAttributeInitialize);
	REG_FUNC(cellSpurs, _cellSpursTasksetAttribute2Initialize);
	REG_FUNC(cellSpurs, cellSpursTasksetAttributeSetName);
	REG_FUNC(cellSpurs, cellSpursTasksetAttributeSetTasksetSize);
	REG_FUNC(cellSpurs, cellSpursTasksetAttributeEnableClearLS);
	REG_FUNC(cellSpurs, cellSpursJoinTaskset);
	REG_FUNC(cellSpurs, cellSpursGetTasksetId);
	REG_FUNC(cellSpurs, cellSpursShutdownTaskset);
	REG_FUNC(cellSpurs, cellSpursCreateTask);
	REG_FUNC(cellSpurs, cellSpursCreateTaskWithAttribute);
	REG_FUNC(cellSpurs, _cellSpursTaskAttributeInitialize);
	REG_FUNC(cellSpurs, _cellSpursTaskAttribute2Initialize);
	REG_FUNC(cellSpurs, cellSpursTaskAttributeSetExitCodeContainer);
	REG_FUNC(cellSpurs, cellSpursTaskExitCodeGet);
	REG_FUNC(cellSpurs, cellSpursTaskExitCodeInitialize);
	REG_FUNC(cellSpurs, cellSpursTaskExitCodeTryGet);
	REG_FUNC(cellSpurs, cellSpursTaskGetLoadableSegmentPattern);
	REG_FUNC(cellSpurs, cellSpursTaskGetReadOnlyAreaPattern);
	REG_FUNC(cellSpurs, cellSpursTaskGenerateLsPattern);
	REG_FUNC(cellSpurs, cellSpursTaskGetContextSaveAreaSize);
	REG_FUNC(cellSpurs, _cellSpursSendSignal);
	REG_FUNC(cellSpurs, cellSpursCreateTaskset2);
	REG_FUNC(cellSpurs, cellSpursCreateTask2);
	REG_FUNC(cellSpurs, cellSpursJoinTask2);
	REG_FUNC(cellSpurs, cellSpursTryJoinTask2);
	REG_FUNC(cellSpurs, cellSpursDestroyTaskset2);
	REG_FUNC(cellSpurs, cellSpursCreateTask2WithBinInfo);
	REG_FUNC(cellSpurs, cellSpursLookUpTasksetAddress);
	REG_FUNC(cellSpurs, cellSpursTasksetGetSpursAddress);
	REG_FUNC(cellSpurs, cellSpursSetExceptionEventHandler);
	REG_FUNC(cellSpurs, cellSpursUnsetExceptionEventHandler);
	REG_FUNC(cellSpurs, cellSpursGetTasksetInfo);
	REG_FUNC(cellSpurs, cellSpursTasksetSetExceptionEventHandler);
	REG_FUNC(cellSpurs, cellSpursTasksetUnsetExceptionEventHandler);

	// Job Chain
	REG_FUNC(cellSpurs, cellSpursCreateJobChain);
	REG_FUNC(cellSpurs, cellSpursCreateJobChainWithAttribute);
	REG_FUNC(cellSpurs, cellSpursShutdownJobChain);
	REG_FUNC(cellSpurs, cellSpursJoinJobChain);
	REG_FUNC(cellSpurs, cellSpursKickJobChain);
	REG_FUNC(cellSpurs, cellSpursRunJobChain);
	REG_FUNC(cellSpurs, cellSpursJobChainGetError);
	REG_FUNC(cellSpurs, _cellSpursJobChainAttributeInitialize);
	REG_FUNC(cellSpurs, cellSpursJobChainAttributeSetName);
	REG_FUNC(cellSpurs, cellSpursJobChainAttributeSetHaltOnError);
	REG_FUNC(cellSpurs, cellSpursJobChainAttributeSetJobTypeMemoryCheck);
	REG_FUNC(cellSpurs, cellSpursGetJobChainId);
	REG_FUNC(cellSpurs, cellSpursJobChainSetExceptionEventHandler);
	REG_FUNC(cellSpurs, cellSpursJobChainUnsetExceptionEventHandler);
	REG_FUNC(cellSpurs, cellSpursGetJobChainInfo);
	REG_FUNC(cellSpurs, cellSpursJobChainGetSpursAddress);

	// Job Guard
	REG_FUNC(cellSpurs, cellSpursJobGuardInitialize);
	REG_FUNC(cellSpurs, cellSpursJobGuardNotify);
	REG_FUNC(cellSpurs, cellSpursJobGuardReset);
	
	// LFQueue
	REG_FUNC(cellSpurs, _cellSpursLFQueueInitialize);
	REG_FUNC(cellSpurs, _cellSpursLFQueuePushBody);
	REG_FUNC(cellSpurs, cellSpursLFQueueAttachLv2EventQueue);
	REG_FUNC(cellSpurs, cellSpursLFQueueDetachLv2EventQueue);
	REG_FUNC(cellSpurs, _cellSpursLFQueuePopBody);
	REG_FUNC(cellSpurs, cellSpursLFQueueGetTasksetAddress);

	// Queue
	REG_FUNC(cellSpurs, _cellSpursQueueInitialize);
	REG_FUNC(cellSpurs, cellSpursQueuePopBody);
	REG_FUNC(cellSpurs, cellSpursQueuePushBody);
	REG_FUNC(cellSpurs, cellSpursQueueAttachLv2EventQueue);
	REG_FUNC(cellSpurs, cellSpursQueueDetachLv2EventQueue);
	REG_FUNC(cellSpurs, cellSpursQueueGetTasksetAddress);
	REG_FUNC(cellSpurs, cellSpursQueueClear);
	REG_FUNC(cellSpurs, cellSpursQueueDepth);
	REG_FUNC(cellSpurs, cellSpursQueueGetEntrySize);
	REG_FUNC(cellSpurs, cellSpursQueueSize);
	REG_FUNC(cellSpurs, cellSpursQueueGetDirection);

	// Workload
	REG_FUNC(cellSpurs, cellSpursWorkloadAttributeSetName);
	REG_FUNC(cellSpurs, cellSpursWorkloadAttributeSetShutdownCompletionEventHook);
	REG_FUNC(cellSpurs, cellSpursAddWorkloadWithAttribute);
	REG_FUNC(cellSpurs, cellSpursAddWorkload);
	REG_FUNC(cellSpurs, cellSpursShutdownWorkload);
	REG_FUNC(cellSpurs, cellSpursWaitForWorkloadShutdown);
	REG_FUNC(cellSpurs, cellSpursRemoveWorkload);
	REG_FUNC(cellSpurs, cellSpursReadyCountStore);
	REG_FUNC(cellSpurs, cellSpursGetWorkloadFlag);
	REG_FUNC(cellSpurs, _cellSpursWorkloadFlagReceiver);
	REG_FUNC(cellSpurs, _cellSpursWorkloadAttributeInitialize);
	REG_FUNC(cellSpurs, cellSpursSendWorkloadSignal);
	REG_FUNC(cellSpurs, cellSpursGetWorkloadData);
	REG_FUNC(cellSpurs, cellSpursReadyCountAdd);
	REG_FUNC(cellSpurs, cellSpursReadyCountCompareAndSwap);
	REG_FUNC(cellSpurs, cellSpursReadyCountSwap);
	REG_FUNC(cellSpurs, cellSpursRequestIdleSpu);
	REG_FUNC(cellSpurs, cellSpursGetWorkloadInfo);
	REG_FUNC(cellSpurs, cellSpursGetSpuGuid);
	REG_FUNC(cellSpurs, _cellSpursWorkloadFlagReceiver2);
	REG_FUNC(cellSpurs, cellSpursGetJobPipelineInfo);
	REG_FUNC(cellSpurs, cellSpursJobSetMaxGrab);
	REG_FUNC(cellSpurs, cellSpursJobHeaderSetJobbin2Param);

	REG_FUNC(cellSpurs, cellSpursWakeUp);
	REG_FUNC(cellSpurs, cellSpursAddUrgentCommand);
	REG_FUNC(cellSpurs, cellSpursAddUrgentCall);

	REG_FUNC(cellSpurs, cellSpursBarrierInitialize);
	REG_FUNC(cellSpurs, cellSpursBarrierGetTasksetAddress);

	REG_FUNC(cellSpurs, _cellSpursSemaphoreInitialize);
	REG_FUNC(cellSpurs, cellSpursSemaphoreGetTasksetAddress);

	// TODO: some trace funcs
}