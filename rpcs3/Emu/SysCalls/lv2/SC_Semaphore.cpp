#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"

SysCallBase sys_sem("sys_semaphore");

struct semaphore_attr
{
	u32 protocol;
	u32 pshared;
	u64 ipc_key;
	int flags;
	u32 pad;
	char name[8];
};

struct semaphore
{
	wxSemaphore sem;
	semaphore_attr attr;
	int sem_count;

	semaphore(int initial_count, int max_count, semaphore_attr attr)
		: sem(initial_count, max_count)
		, attr(attr)
	{
	}
};

int sys_semaphore_create(u32 sem_addr, u32 attr_addr, int initial_count, int max_count)
{
	sys_sem.Warning("sys_semaphore_create(sem_addr=0x%x, attr_addr=0x%x, initial_count=%d, max_count=%d)",
		sem_addr, attr_addr, initial_count, max_count);

	if(!Memory.IsGoodAddr(sem_addr) || !Memory.IsGoodAddr(attr_addr)) return CELL_EFAULT;

	semaphore_attr attr = (semaphore_attr&)Memory[attr_addr];
	attr.protocol = re(attr.protocol);
	attr.pshared = re(attr.pshared);
	attr.ipc_key = re(attr.ipc_key);
	attr.flags = re(attr.flags);
	
	sys_sem.Log("*** protocol = %d", attr.protocol);
	sys_sem.Log("*** pshared = %d", attr.pshared);
	sys_sem.Log("*** ipc_key = 0x%llx", attr.ipc_key);
	sys_sem.Log("*** flags = 0x%x", attr.flags);
	sys_sem.Log("*** name = %s", attr.name);

	Memory.Write32(sem_addr, sys_sem.GetNewId(new semaphore(initial_count, max_count, attr)));

	return CELL_OK;
}

int sys_semaphore_destroy(u32 sem)
{
	sys_sem.Log("sys_semaphore_destroy(sem=%d)", sem);

	if(!sys_sem.CheckId(sem)) return CELL_ESRCH;

	Emu.GetIdManager().RemoveID(sem);
	return CELL_OK;
}

int sys_semaphore_wait(u32 sem, u64 timeout)
{
	sys_sem.Log("sys_semaphore_wait(sem=0x%x, timeout=0x%llx)", sem, timeout);

	semaphore* sem_data = nullptr;
	if(!sys_sem.CheckId(sem, sem_data)) return CELL_ESRCH;

	sem_data->sem_count = 0;  // Reset internal counter for sys_semaphore_get_value.
	sem_data->sem.WaitTimeout(timeout ? timeout : INFINITE);

	return CELL_OK;
}

int sys_semaphore_trywait(u32 sem)
{
	sys_sem.Log("sys_semaphore_trywait(sem=%d)", sem);

	semaphore* sem_data = nullptr;
	if(!sys_sem.CheckId(sem, sem_data)) return CELL_ESRCH;

	sem_data->sem_count = 0;  // Reset internal counter for sys_semaphore_get_value.
	if(sem_data->sem.TryWait()) return 1;

	return CELL_OK;
}

int sys_semaphore_post(u32 sem, int count)
{
	sys_sem.Log("sys_semaphore_post(sem=%d, count=%d)", sem, count);

	semaphore* sem_data = nullptr;
	if(!sys_sem.CheckId(sem, sem_data)) return CELL_ESRCH;

	while(count--)
	{
		sem_data->sem_count++;  // Increment internal counter for sys_semaphore_get_value.
		sem_data->sem.Post();
	}

	return CELL_OK;
}

int sys_semaphore_get_value(u32 sem, u32 count_addr)
{
	sys_sem.Log("sys_semaphore_get_value(sem=%d, count_addr=0x%x)", sem, count_addr);

	semaphore* sem_data = nullptr;
	if(!sys_sem.CheckId(sem, sem_data)) return CELL_ESRCH;

	Memory.Write32(count_addr, sem_data->sem_count);

	return CELL_OK;
}
