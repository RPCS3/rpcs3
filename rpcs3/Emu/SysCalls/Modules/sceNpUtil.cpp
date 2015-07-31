#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "sceNp.h"
#include "sceNpUtil.h"

extern Module sceNpUtil;

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

Module sceNpUtil("sceNpUtil", []()
{
	REG_FUNC(sceNpUtil, sceNpUtilBandwidthTestInitStart);
	REG_FUNC(sceNpUtil, sceNpUtilBandwidthTestShutdown);
	REG_FUNC(sceNpUtil, sceNpUtilBandwidthTestGetStatus);
	REG_FUNC(sceNpUtil, sceNpUtilBandwidthTestAbort);
});
