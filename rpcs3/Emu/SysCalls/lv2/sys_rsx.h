#pragma once

// SysCalls
s32 sys_rsx_device_open();
s32 sys_rsx_device_close();
s32 sys_rsx_memory_allocate();
s32 sys_rsx_memory_free();
s32 sys_rsx_context_allocate();
s32 sys_rsx_context_free();
s32 sys_rsx_context_iomap();
s32 sys_rsx_context_iounmap();
s32 sys_rsx_context_attribute(s32 context_id, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6);
s32 sys_rsx_device_map(mem32_t a1, mem32_t a2, u32 a3);
s32 sys_rsx_device_unmap();
s32 sys_rsx_attribute();
