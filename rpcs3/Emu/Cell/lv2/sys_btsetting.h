#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/ErrorCodes.h"

// SysCalls

error_code sys_btsetting_if(u64 cmd, vm::ptr<void> msg);
