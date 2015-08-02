#include "stdafx.h"
#include "Emu/SysCalls/Modules.h"

#include "sceNpSns.h"

extern Module sceNpSns;

s32 sceNpSnsFbInit(vm::ptr<const SceNpSnsFbInitParams> params)
{
	sceNpSns.Todo("sceNpSnsFbInit(params=*0x%x)", params);

	// TODO: Use the initialization parameters somewhere

	return CELL_OK;
}

s32 sceNpSnsFbTerm()
{
	sceNpSns.Warning("sceNpSnsFbTerm()");

	return CELL_OK;
}

s32 sceNpSnsFbCreateHandle()
{
	UNIMPLEMENTED_FUNC(sceNpSns);
	return CELL_OK;
}

s32 sceNpSnsFbDestroyHandle()
{
	UNIMPLEMENTED_FUNC(sceNpSns);
	return CELL_OK;
}

s32 sceNpSnsFbAbortHandle()
{
	UNIMPLEMENTED_FUNC(sceNpSns);
	return CELL_OK;
}

s32 sceNpSnsFbGetAccessToken()
{
	UNIMPLEMENTED_FUNC(sceNpSns);
	return CELL_OK;
}

s32 sceNpSnsFbStreamPublish()
{
	throw EXCEPTION("");
}

s32 sceNpSnsFbCheckThrottle()
{
	throw EXCEPTION("");
}

s32 sceNpSnsFbCheckConfig()
{
	throw EXCEPTION("");
}

s32 sceNpSnsFbLoadThrottle()
{
	throw EXCEPTION("");
}


Module sceNpSns("sceNpSns", []()
{
	REG_FUNC(sceNpSns, sceNpSnsFbInit);
	REG_FUNC(sceNpSns, sceNpSnsFbTerm);
	REG_FUNC(sceNpSns, sceNpSnsFbCreateHandle);
	REG_FUNC(sceNpSns, sceNpSnsFbDestroyHandle);
	REG_FUNC(sceNpSns, sceNpSnsFbAbortHandle);
	REG_FUNC(sceNpSns, sceNpSnsFbGetAccessToken);

	REG_FUNC(sceNpSns, sceNpSnsFbStreamPublish);
	REG_FUNC(sceNpSns, sceNpSnsFbCheckThrottle);
	REG_FUNC(sceNpSns, sceNpSnsFbCheckConfig);
	REG_FUNC(sceNpSns, sceNpSnsFbLoadThrottle);
});
