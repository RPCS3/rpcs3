#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/GS/GCM.h"

extern Module cellGcmSys;
extern gcmInfo gcm_info;

int cellGcmCallback(u32 context_addr, u32 count)
{
	GSLockCurrent gslock(GS_LOCK_WAIT_FLUSH); // could stall on exit

	CellGcmContextData& ctx = (CellGcmContextData&)Memory[context_addr];
	CellGcmControl& ctrl = (CellGcmControl&)Memory[gcm_info.control_addr];

	const s32 res = re(ctx.current) - re(ctx.begin) - re(ctrl.put);

	if(res > 0) memcpy(&Memory[re(ctx.begin)], &Memory[re(ctx.current) - res], res);

	ctx.current = re(re(ctx.begin) + res);

	ctrl.put = re(res);
	ctrl.get = 0;
	
	return CELL_OK;
}
