#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "sceNp.h"
#include "sceNpUtil.h"

logs::channel sceNpUtil("sceNpUtil");

s32 sceNpUtilBandwidthTestInitStart(u32 prio, size_t stack)
{
	UNIMPLEMENTED_FUNC(sceNpUtil);
	return CELL_OK;
}

s32 sceNpUtilBandwidthTestGetStatus()
{
	UNIMPLEMENTED_FUNC(sceNpUtil);
	return CELL_OK;
}

s32 sceNpUtilBandwidthTestShutdown()
{
	UNIMPLEMENTED_FUNC(sceNpUtil);
	return CELL_OK;
}

s32 sceNpUtilBandwidthTestAbort()
{
	UNIMPLEMENTED_FUNC(sceNpUtil);
	return CELL_OK;
}

DECLARE(ppu_module_manager::sceNpUtil)("sceNpUtil", []()
{
	REG_FUNC(sceNpUtil, sceNpUtilBandwidthTestInitStart);
	REG_FUNC(sceNpUtil, sceNpUtilBandwidthTestShutdown);
	REG_FUNC(sceNpUtil, sceNpUtilBandwidthTestGetStatus);
	REG_FUNC(sceNpUtil, sceNpUtilBandwidthTestAbort);
});
