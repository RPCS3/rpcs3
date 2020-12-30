#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/IdManager.h"

#include "sceNp.h"
#include "sceNpUtil.h"

LOG_CHANNEL(sceNpUtil);

error_code sceNpUtilBandwidthTestInitStart(u32 prio, u64 stack)
{
	sceNpUtil.todo("sceNpUtilBandwidthTestInitStart(prio=%d, stack=%d)", prio, stack);

	const auto util_manager = g_fxo->get<sce_np_util_manager>();

	if (util_manager->is_initialized)
	{
		return SCE_NP_ERROR_ALREADY_INITIALIZED;
	}

	util_manager->is_initialized = true;

	return CELL_OK;
}

error_code sceNpUtilBandwidthTestGetStatus()
{
	sceNpUtil.todo("sceNpUtilBandwidthTestGetStatus()");

	if (!g_fxo->get<sce_np_util_manager>()->is_initialized)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	return not_an_error(SCE_NP_UTIL_BANDWIDTH_TEST_STATUS_FINISHED);
}

error_code sceNpUtilBandwidthTestShutdown(vm::ptr<SceNpUtilBandwidthTestResult> result)
{
	sceNpUtil.todo("sceNpUtilBandwidthTestShutdown(result=*0x%x)", result);

	const auto util_manager = g_fxo->get<sce_np_util_manager>();

	if (!util_manager->is_initialized)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	util_manager->is_initialized = false;

	return CELL_OK;
}

error_code sceNpUtilBandwidthTestAbort()
{
	sceNpUtil.todo("sceNpUtilBandwidthTestAbort()");

	if (!g_fxo->get<sce_np_util_manager>()->is_initialized)
	{
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

DECLARE(ppu_module_manager::sceNpUtil)("sceNpUtil", []()
{
	REG_FUNC(sceNpUtil, sceNpUtilBandwidthTestInitStart);
	REG_FUNC(sceNpUtil, sceNpUtilBandwidthTestShutdown);
	REG_FUNC(sceNpUtil, sceNpUtilBandwidthTestGetStatus);
	REG_FUNC(sceNpUtil, sceNpUtilBandwidthTestAbort);
});
