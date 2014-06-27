#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/GS/GCM.h"

extern Module cellGcmSys;
extern gcmInfo gcm_info;

int cellGcmCallback(u32 context_addr, u32 count)
{
	GSLockCurrent gslock(GS_LOCK_WAIT_FLUSH);

	CellGcmContextData& ctx = (CellGcmContextData&)Memory[context_addr];
	CellGcmControl& ctrl = (CellGcmControl&)Memory[gcm_info.control_addr];

	const s32 res = ctx.current - ctx.begin - ctrl.put;

	if(res > 0) Memory.Copy(ctx.begin, ctx.current - res, res);

	ctx.current = ctx.begin + res;

	//InterlockedExchange64((volatile long long*)((u8*)&ctrl + offsetof(CellGcmControl, put)), (u64)(u32)re(res));
	ctrl.put = res;
	ctrl.get = 0;
	
	return CELL_OK;
}
