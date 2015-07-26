#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "sceNp.h"
#include "sceNpUtil.h"

extern Module sceNpUtil;

std::unique_ptr<SceNpUtilInternal> g_sceNpUtil;

s32 sceNpUtilCmpNpId()
{
	UNIMPLEMENTED_FUNC(sceNpUtil);
	return CELL_OK;
}

s32 sceNpUtilCmpNpIdInOrder()
{
	UNIMPLEMENTED_FUNC(sceNpUtil);
	return CELL_OK;
}

s32 sceNpUtilBandwidthTestInitStart(u32 prio, size_t stack)
{
	UNIMPLEMENTED_FUNC(sceNpUtil);

	if (g_sceNpUtil->m_bSceNpUtilBandwidthTestInitialized)
	{
		return SCE_NP_ERROR_ALREADY_INITIALIZED;
	}

	g_sceNpUtil->m_bSceNpUtilBandwidthTestInitialized = true;

	return CELL_OK;
}

s32 sceNpUtilBandwidthTestGetStatus()
{
	UNIMPLEMENTED_FUNC(sceNpUtil);

	if (!g_sceNpUtil->m_bSceNpUtilBandwidthTestInitialized)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpUtilBandwidthTestShutdown()
{
	UNIMPLEMENTED_FUNC(sceNpUtil);

	if (!g_sceNpUtil->m_bSceNpUtilBandwidthTestInitialized)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	g_sceNpUtil->m_bSceNpUtilBandwidthTestInitialized = false;

	return CELL_OK;
}

s32 sceNpUtilBandwidthTestAbort()
{
	UNIMPLEMENTED_FUNC(sceNpUtil);

	if (!g_sceNpUtil->m_bSceNpUtilBandwidthTestInitialized)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

Module sceNpUtil("sceNpUtil", []()
{
	g_sceNpUtil = std::make_unique<SceNpUtilInternal>();

	REG_FUNC(sceNpUtil, sceNpUtilBandwidthTestInitStart);
	REG_FUNC(sceNpUtil, sceNpUtilBandwidthTestShutdown);
	REG_FUNC(sceNpUtil, sceNpUtilCmpNpId);
	REG_FUNC(sceNpUtil, sceNpUtilCmpNpIdInOrder);
	REG_FUNC(sceNpUtil, sceNpUtilBandwidthTestGetStatus);
	REG_FUNC(sceNpUtil, sceNpUtilBandwidthTestAbort);
});