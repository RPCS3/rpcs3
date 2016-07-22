#include "stdafx.h"
#include "Emu/IdManager.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/Cell/lv2/sys_lwmutex.h"
#include "Emu/Cell/lv2/sys_lwcond.h"
#include "Emu/Cell/lv2/sys_cond.h"
#include "sysPrxForUser.h"

extern logs::channel sysPrxForUser;

s32 sys_lwcond_create(vm::ptr<sys_lwcond_t> lwcond, vm::ptr<sys_lwmutex_t> lwmutex, vm::ptr<sys_lwcond_attribute_t> attr)
{
	sysPrxForUser.warning("sys_lwcond_create(lwcond=*0x%x, lwmutex=*0x%x, attr=*0x%x)", lwcond, lwmutex, attr);

	vm::var<sys_cond_attribute_t> _attr;
	_attr->pshared = SYS_SYNC_NOT_PROCESS_SHARED;
	_attr->flags = 0;
	_attr->ipc_key = 0;
	_attr->name_u64 = (u64&)attr->name;

	return sys_cond_create(lwcond.ptr(&sys_lwcond_t::lwcond_queue), lwmutex->pad, _attr);
}

s32 sys_lwcond_destroy(vm::ptr<sys_lwcond_t> lwcond)
{
	sysPrxForUser.trace("sys_lwcond_destroy(lwcond=*0x%x)", lwcond);

	return sys_cond_destroy(lwcond->lwcond_queue);
}

s32 sys_lwcond_signal(PPUThread& ppu, vm::ptr<sys_lwcond_t> lwcond)
{
	sysPrxForUser.trace("sys_lwcond_signal(lwcond=*0x%x)", lwcond);

	return sys_cond_signal(lwcond->lwcond_queue);
}

s32 sys_lwcond_signal_all(PPUThread& ppu, vm::ptr<sys_lwcond_t> lwcond)
{
	sysPrxForUser.trace("sys_lwcond_signal_all(lwcond=*0x%x)", lwcond);

	return sys_cond_signal_all(lwcond->lwcond_queue);
}

s32 sys_lwcond_signal_to(PPUThread& ppu, vm::ptr<sys_lwcond_t> lwcond, u32 ppu_thread_id)
{
	sysPrxForUser.trace("sys_lwcond_signal_to(lwcond=*0x%x, ppu_thread_id=0x%x)", lwcond, ppu_thread_id);

	return sys_cond_signal_to(lwcond->lwcond_queue, ppu_thread_id);
}

s32 sys_lwcond_wait(PPUThread& ppu, vm::ptr<sys_lwcond_t> lwcond, u64 timeout)
{
	sysPrxForUser.trace("sys_lwcond_wait(lwcond=*0x%x, timeout=0x%llx)", lwcond, timeout);

	return sys_cond_wait(ppu, lwcond->lwcond_queue, timeout);
}

void sysPrxForUser_sys_lwcond_init()
{
	REG_FUNC(sysPrxForUser, sys_lwcond_create);
	REG_FUNC(sysPrxForUser, sys_lwcond_destroy);
	REG_FUNC(sysPrxForUser, sys_lwcond_signal);
	REG_FUNC(sysPrxForUser, sys_lwcond_signal_all);
	REG_FUNC(sysPrxForUser, sys_lwcond_signal_to);
	REG_FUNC(sysPrxForUser, sys_lwcond_wait);
}
