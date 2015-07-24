#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "sceNp.h"

extern Module sceNpUtil;

struct sceNpUtilInternal
{
	bool m_bSceNpUtilBandwidthTestInitialized;

	sceNpUtilInternal()
		: m_bSceNpUtilBandwidthTestInitialized(false)
	{
	}
};

sceNpUtilInternal sceNpUtilInstance;

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

	if (sceNpUtilInstance.m_bSceNpUtilBandwidthTestInitialized)
		return SCE_NP_ERROR_ALREADY_INITIALIZED;

	sceNpUtilInstance.m_bSceNpUtilBandwidthTestInitialized = true;

	return CELL_OK;
}

s32 sceNpUtilBandwidthTestGetStatus()
{
	UNIMPLEMENTED_FUNC(sceNpUtil);

	if (!sceNpUtilInstance.m_bSceNpUtilBandwidthTestInitialized)
		return SCE_NP_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

s32 sceNpUtilBandwidthTestShutdown()
{
	UNIMPLEMENTED_FUNC(sceNpUtil);

	if (!sceNpUtilInstance.m_bSceNpUtilBandwidthTestInitialized)
		return SCE_NP_ERROR_NOT_INITIALIZED;

	sceNpUtilInstance.m_bSceNpUtilBandwidthTestInitialized = false;

	return CELL_OK;
}

s32 sceNpUtilBandwidthTestAbort()
{
	UNIMPLEMENTED_FUNC(sceNpUtil);

	if (!sceNpUtilInstance.m_bSceNpUtilBandwidthTestInitialized)
		return SCE_NP_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

Module sceNpUtil("sceNpUtil", []()
{
	sceNpUtilInstance.m_bSceNpUtilBandwidthTestInitialized = false;

	REG_FUNC(sceNpUtil, sceNpUtilBandwidthTestInitStart);
	REG_FUNC(sceNpUtil, sceNpUtilBandwidthTestShutdown);
	REG_FUNC(sceNpUtil, sceNpUtilCmpNpId);
	REG_FUNC(sceNpUtil, sceNpUtilCmpNpIdInOrder);
	REG_FUNC(sceNpUtil, sceNpUtilBandwidthTestGetStatus);
	REG_FUNC(sceNpUtil, sceNpUtilBandwidthTestAbort);
});