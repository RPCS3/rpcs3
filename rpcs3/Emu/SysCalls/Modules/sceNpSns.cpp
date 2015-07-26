#include "stdafx.h"
#include "Emu/SysCalls/Modules.h"

#include "sceNpSns.h"

extern Module sceNpSns;

std::unique_ptr<SceNpSnsInternal> g_sceNpSns;

s32 sceNpSnsFbInit(/*const SceNpSnsFbInitParams params*/)
{
	sceNpSns.Todo("sceNpSnsFbInit(params=???)"/*, params*/);

	if (g_sceNpSns->m_bSceNpSnsInitialized)
	{
		return SCE_NP_SNS_FB_ERROR_ALREADY_INITIALIZED;
	}

	g_sceNpSns->m_bSceNpSnsInitialized = true;

	// TODO: Use the initialization parameters somewhere

	return CELL_OK;
}

s32 sceNpSnsFbTerm()
{
	sceNpSns.Warning("sceNpSnsFbTerm()");

	if (!g_sceNpSns->m_bSceNpSnsInitialized)
	{
		return SCE_NP_SNS_FB_ERROR_NOT_INITIALIZED;
	}

	g_sceNpSns->m_bSceNpSnsInitialized = false;

	return CELL_OK;
}

s32 sceNpSnsFbCreateHandle()
{
	UNIMPLEMENTED_FUNC(sceNpSns);

	if (!g_sceNpSns->m_bSceNpSnsInitialized)
	{
		return SCE_NP_SNS_FB_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpSnsFbDestroyHandle()
{
	UNIMPLEMENTED_FUNC(sceNpSns);

	if (!g_sceNpSns->m_bSceNpSnsInitialized)
	{
		return SCE_NP_SNS_FB_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpSnsFbAbortHandle()
{
	UNIMPLEMENTED_FUNC(sceNpSns);

	if (!g_sceNpSns->m_bSceNpSnsInitialized)
	{
		return SCE_NP_SNS_FB_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

s32 sceNpSnsFbGetAccessToken()
{
	UNIMPLEMENTED_FUNC(sceNpSns);

	if (!g_sceNpSns->m_bSceNpSnsInitialized)
	{
		return SCE_NP_SNS_FB_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

Module sceNpSns("sceNpSns", []()
{
	g_sceNpSns = std::make_unique<SceNpSnsInternal>();

	REG_FUNC(sceNpSns, sceNpSnsFbInit);
	//REG_FUNC(sceNpSns, sceNpSnsFbTerm);
	//REG_FUNC(sceNpSns, sceNpSnsFbCreateHandle);
	//REG_FUNC(sceNpSns, sceNpSnsFbDestroyHandle);
	//REG_FUNC(sceNpSns, sceNpSnsFbAbortHandle);
	//REG_FUNC(sceNpSns, sceNpSnsFbGetAccessToken);
});
