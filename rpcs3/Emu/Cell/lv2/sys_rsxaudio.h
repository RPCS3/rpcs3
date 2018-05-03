#pragma once

s32 sys_rsxaudio_initialize(vm::ptr<u8> args);
s32 sys_rsxaudio_finalize(u32);
s32 sys_rsxaudio_import_shared_memory(u32, u32);
s32 sys_rsxaudio_unimport_shared_memory(u32, u32);
s32 sys_rsxaudio_create_connection(u32);
u32 sys_rsxaudio_close_connection(u32);
u32 sys_rsxaudio_prepare_process(u32);
u32 sys_rsxaudio_start_process(u32);
u32 sys_rsxaudio_stop_process(u32);
u32 sys_rsxaudio_();