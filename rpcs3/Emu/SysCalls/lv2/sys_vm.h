#pragma pack

#define SYS_VM_TEST_INVALID                 0x0000ULL
#define SYS_VM_TEST_UNUSED                  0x0001ULL
#define SYS_VM_TEST_ALLOCATED               0x0002ULL
#define SYS_VM_TEST_STORED                  0x0004ULL

struct sys_vm_statistics {
	u64 vm_crash_ppu;
	u64 vm_crash_spu;
	u64 vm_read;
	u64 vm_write;
	u32 physical_mem_size;
	u32 physical_mem_used;
	u64 timestamp;
};

// SysCalls
s32 sys_vm_memory_map(u32 vsize, u32 psize, u32 cid, u64 flag, u64 policy, u32 addr);
s32 sys_vm_unmap(u32 addr);
s32 sys_vm_append_memory(u32 addr, u32 size);
s32 sys_vm_return_memory(u32 addr, u32 size);
s32 sys_vm_lock(u32 addr, u32 size);
s32 sys_vm_unlock(u32 addr, u32 size);
s32 sys_vm_touch(u32 addr, u32 size);
s32 sys_vm_flush(u32 addr, u32 size);
s32 sys_vm_invalidate(u32 addr, u32 size);
s32 sys_vm_store(u32 addr, u32 size);
s32 sys_vm_sync(u32 addr, u32 size);
s32 sys_vm_test(u32 addr, u32 size, u32 result_addr);
s32 sys_vm_get_statistics(u32 addr, u32 stat_addr);
