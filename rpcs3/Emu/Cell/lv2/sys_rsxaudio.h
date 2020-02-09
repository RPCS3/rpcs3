#pragma once

#include "Emu/Memory/vm_ptr.h"

// SysCalls

error_code sys_rsxaudio_initialize(vm::ptr<u32> handle);
error_code sys_rsxaudio_finalize(u32 handle);
error_code sys_rsxaudio_import_shared_memory(u32 handle, vm::ptr<u64> addr);
error_code sys_rsxaudio_unimport_shared_memory(u32 handle, vm::ptr<u64> addr);
