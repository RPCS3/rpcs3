#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/IdManager.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/Cell/lv2/sys_lwmutex.h"
#include "Emu/Cell/lv2/sys_mutex.h"
#include "sysPrxForUser.h"

extern logs::channel sysPrxForUser;

s32 sys_lwmutex_create(vm::ptr<sys_lwmutex_t> lwmutex, vm::ptr<sys_lwmutex_attribute_t> attr)
{
	sysPrxForUser.warning("sys_lwmutex_create(lwmutex=*0x%x, attr=*0x%x)", lwmutex, attr);

	vm::var<sys_mutex_attribute_t> _attr;
	_attr->protocol = SYS_SYNC_FIFO;
	_attr->recursive = attr->recursive;
	_attr->pshared = SYS_SYNC_NOT_PROCESS_SHARED;
	_attr->adaptive = SYS_SYNC_NOT_ADAPTIVE;
	_attr->ipc_key = 0;
	_attr->flags = 0;
	_attr->pad = 0;
	_attr->name_u64 = (u64&)attr->name;

	return sys_mutex_create(lwmutex.ptr(&sys_lwmutex_t::pad), _attr);
}

s32 sys_lwmutex_destroy(PPUThread& ppu, vm::ptr<sys_lwmutex_t> lwmutex)
{
	sysPrxForUser.trace("sys_lwmutex_destroy(lwmutex=*0x%x)", lwmutex);

	return sys_mutex_destroy(lwmutex->pad);
}

s32 sys_lwmutex_lock(PPUThread& ppu, vm::ptr<sys_lwmutex_t> lwmutex, u64 timeout)
{
	sysPrxForUser.trace("sys_lwmutex_lock(lwmutex=*0x%x, timeout=0x%llx)", lwmutex, timeout);

	return sys_mutex_lock(ppu, lwmutex->pad, timeout);
}

s32 sys_lwmutex_trylock(PPUThread& ppu, vm::ptr<sys_lwmutex_t> lwmutex)
{
	sysPrxForUser.trace("sys_lwmutex_trylock(lwmutex=*0x%x)", lwmutex);

	return sys_mutex_trylock(ppu, lwmutex->pad);
}

s32 sys_lwmutex_unlock(PPUThread& ppu, vm::ptr<sys_lwmutex_t> lwmutex)
{
	sysPrxForUser.trace("sys_lwmutex_unlock(lwmutex=*0x%x)", lwmutex);

	return sys_mutex_unlock(ppu, lwmutex->pad);
}

void sysPrxForUser_sys_lwmutex_init()
{
	REG_FUNC(sysPrxForUser, sys_lwmutex_create);
	REG_FUNC(sysPrxForUser, sys_lwmutex_destroy);
	REG_FUNC(sysPrxForUser, sys_lwmutex_lock);
	REG_FUNC(sysPrxForUser, sys_lwmutex_trylock);
	REG_FUNC(sysPrxForUser, sys_lwmutex_unlock);
}
