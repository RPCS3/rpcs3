#pragma once

// SysCalls
s32 sys_rsx_device_open();
s32 sys_rsx_device_close();
s32 sys_rsx_memory_allocate(mem32_t mem_handle, mem32_t mem_addr, u32 size, u64 flags, u64 a5, u64 a6, u64 a7);
s32 sys_rsx_memory_free(u32 mem_handle);
s32 sys_rsx_context_allocate(mem32_t context_id, mem32_t lpar_dma_control, mem32_t lpar_driver_info, mem32_t lpar_reports, u64 mem_ctx, u64 system_mode);
s32 sys_rsx_context_free(u32 context_id);
s32 sys_rsx_context_iomap(u32 context_id, u32 io, u32 ea, u32 size, u64 flags);
s32 sys_rsx_context_iounmap(u32 context_id, u32 a2, u32 io_addr, u32 size);
s32 sys_rsx_context_attribute(s32 context_id, u32 package_id, u64 a3, u64 a4, u64 a5, u64 a6);
s32 sys_rsx_device_map(mem32_t a1, mem32_t a2, u32 dev_id);
s32 sys_rsx_device_unmap(u32 dev_id);
s32 sys_rsx_attribute();
