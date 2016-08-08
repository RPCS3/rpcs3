#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "sceNpSns.h"

logs::channel sceNpSns("sceNpSns", logs::level::notice);

s32 sceNpSnsFbInit(vm::ptr<const SceNpSnsFbInitParams> params)
{
	sceNpSns.todo("sceNpSnsFbInit(params=*0x%x)", params);

	// TODO: Use the initialization parameters somewhere

	return CELL_OK;
}

s32 sceNpSnsFbTerm()
{
	sceNpSns.warning("sceNpSnsFbTerm()");

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
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpSnsFbCheckThrottle()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpSnsFbCheckConfig()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpSnsFbLoadThrottle()
{
	fmt::throw_exception("Unimplemented" HERE);
}


DECLARE(ppu_module_manager::sceNpSns)("sceNpSns", []()
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
