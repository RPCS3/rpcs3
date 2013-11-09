#include "stdafx.h"
#include "SC_Rwlock.h"
#include "Emu/SysCalls/SysCalls.h"

SysCallBase sys_rwlock("sys_rwlock");

int sys_rwlock_create(mem32_t rw_lock_id, mem_struct_ptr_t<sys_rwlock_attribute_t> attr)
{
	sys_rwlock.Warning("Unimplemented function: sys_rwlock_create(rw_lock_id_addr=0x%x, attr_addr=0x%x)", rw_lock_id.GetAddr(), attr.GetAddr());
	return CELL_OK;
}

int sys_rwlock_destroy(u32 rw_lock_id)
{
	sys_rwlock.Warning("Unimplemented function: sys_rwlock_destroy(rw_lock_id=%d)", rw_lock_id);
	return CELL_OK;
}

int sys_rwlock_rlock(u32 rw_lock_id, u64 timeout)
{
	sys_rwlock.Warning("Unimplemented function: sys_rwlock_rlock(rw_lock_id=%d, timeout=%llu)", rw_lock_id, timeout);
	return CELL_OK;
}

int sys_rwlock_tryrlock(u32 rw_lock_id)
{
	sys_rwlock.Warning("Unimplemented function: sys_rwlock_tryrlock(rw_lock_id=%d)", rw_lock_id);
	return CELL_OK;
}

int sys_rwlock_runlock(u32 rw_lock_id)
{
	sys_rwlock.Warning("Unimplemented function: sys_rwlock_runlock(rw_lock_id=%d)", rw_lock_id);
	return CELL_OK;
}

int sys_rwlock_wlock(u32 rw_lock_id, u64 timeout)
{
	sys_rwlock.Warning("Unimplemented function: sys_rwlock_wlock(rw_lock_id=%d, timeout=%llu)", rw_lock_id, timeout);
	return CELL_OK;
}

int sys_rwlock_trywlock(u32 rw_lock_id)
{
	sys_rwlock.Warning("Unimplemented function: sys_rwlock_trywlock(rw_lock_id=%d)", rw_lock_id);
	return CELL_OK;
}

int sys_rwlock_wunlock(u32 rw_lock_id)
{
	sys_rwlock.Warning("Unimplemented function: sys_rwlock_wunlock(rw_lock_id=%d)", rw_lock_id);
	return CELL_OK;
}
