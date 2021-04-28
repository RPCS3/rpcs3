#pragma once

#include "Emu/Memory/vm_ptr.h"

// SysCalls

error_code sys_io_buffer_create(u32 block_count, u32 block_size, u32 blocks, u32 unk1, vm::ptr<u32> handle);
error_code sys_io_buffer_destroy(u32 handle);
error_code sys_io_buffer_allocate(u32 handle, vm::ptr<u32> block);
error_code sys_io_buffer_free(u32 handle, u32 block);
