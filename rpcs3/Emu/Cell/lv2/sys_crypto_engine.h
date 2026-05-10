#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/ErrorCodes.h"

// SysCalls

error_code sys_crypto_engine_create(vm::ptr<u32> id);
error_code sys_crypto_engine_destroy(u32 id);
error_code sys_crypto_engine_random_generate(vm::ptr<void> buffer, u64 buffer_size);
