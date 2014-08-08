#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/RawSPUThread.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/SysCalls.h"
#include "sys_interrupt.h"

static SysCallBase sc_int("sys_interrupt");

s32 sys_interrupt_tag_destroy(u32 intrtag)
{
	sc_int.Warning("sys_interrupt_tag_destroy(intrtag=%d)", intrtag);

	u32 id = intrtag & 0xff;
	u32 class_id = intrtag >> 8;
	RawSPUThread* t = Emu.GetCPU().GetRawSPUThread(id);

	if (!t || class_id > 2 || class_id == 1)
	{
		return CELL_ESRCH;
	}

	if (!t->m_intrtag[class_id].enabled)
	{
		return CELL_ESRCH;
	}

	if (t->m_intrtag[class_id].thread)
	{
		return CELL_EBUSY;
	}

	t->m_intrtag[class_id].enabled = 0;
	return CELL_OK;
}

s32 sys_interrupt_thread_establish(mem32_t ih, u32 intrtag, u64 intrthread, u64 arg)
{
	sc_int.Warning("sys_interrupt_thread_establish(ih_addr=0x%x, intrtag=%d, intrthread=%lld, arg=0x%llx)", ih.GetAddr(), intrtag, intrthread, arg);

	u32 id = intrtag & 0xff;
	u32 class_id = intrtag >> 8;
	RawSPUThread* t = Emu.GetCPU().GetRawSPUThread(id);

	if (!t || class_id > 2 || class_id == 1)
	{
		return CELL_ESRCH;
	}

	if (!t->m_intrtag[class_id].enabled)
	{
		return CELL_ESRCH;
	}

	if (t->m_intrtag[class_id].thread) // ???
	{
		return CELL_ESTAT;
	}

	CPUThread* it = Emu.GetCPU().GetThread(intrthread);
	if (!it)
	{
		return CELL_ESRCH;
	}

	if (it->m_has_interrupt || !it->m_is_interrupt)
	{
		return CELL_EAGAIN;
	}

	ih = (t->m_intrtag[class_id].thread = intrthread);
	it->m_interrupt_arg = arg;
	return CELL_OK;
}

s32 sys_interrupt_thread_disestablish(u32 ih)
{
	sc_int.Todo("sys_interrupt_thread_disestablish(ih=%d)", ih);

	CPUThread* it = Emu.GetCPU().GetThread(ih);
	if (!it)
	{
		return CELL_ESRCH;
	}

	if (!it->m_has_interrupt || !it->m_is_interrupt)
	{
		return CELL_ESRCH;
	}

	// TODO: wait for sys_interrupt_thread_eoi() and destroy interrupt thread

	return CELL_OK;
}

void sys_interrupt_thread_eoi()
{
	sc_int.Log("sys_interrupt_thread_eoi()");

	GetCurrentPPUThread().Stop();
	return;
}
