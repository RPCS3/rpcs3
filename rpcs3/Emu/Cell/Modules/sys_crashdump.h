#pragma once
#include "Emu/Memory/vm_ptr.h"

enum
{
	SYS_CRASH_DUMP_MAX_LABEL_SIZE = 16,
	SYS_CRASH_DUMP_MAX_LOG_AREA = 127 // not actually defined in CELL
};

struct sys_crash_dump_log_area_info_t
{
	char label[SYS_CRASH_DUMP_MAX_LABEL_SIZE]; // 15 + 1 (0 terminated)
	vm::bptr<void> addr;
	be_t<u32> size;
};
