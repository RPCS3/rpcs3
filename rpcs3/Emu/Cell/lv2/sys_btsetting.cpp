#include "stdafx.h"

#include "sys_btsetting.h"
#include "Emu/Cell/ErrorCodes.h"

LOG_CHANNEL(sys_btsetting);

error_code sys_btsetting_if(u64 cmd, vm::ptr<void> msg)
{
	sys_btsetting.todo("sys_btsetting_if(cmd=0x%llx, msg=*0x%x)", cmd, msg);

	return CELL_OK;
}
