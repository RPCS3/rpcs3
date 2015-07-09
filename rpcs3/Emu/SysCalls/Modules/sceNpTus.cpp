#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "sceNp.h"
#include "sceNpTus.h"

extern Module sceNpTus;

struct sceNpTusInternal
{
	bool m_bSceNpTusInitialized;

	sceNpTusInternal()
		: m_bSceNpTusInitialized(false)
	{
	}
};

sceNpTusInternal sceNpTusInstance;

s32 sceNpTusInit()
{
	sceNpTus.Warning("sceNpTusInit()");

	if (sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_ALREADY_INITIALIZED;

	sceNpTusInstance.m_bSceNpTusInitialized = true;

	return CELL_OK;
}

s32 sceNpTusTerm()
{
	sceNpTus.Warning("sceNpTusTerm()");
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;

	sceNpTusInstance.m_bSceNpTusInitialized = false;

	return CELL_OK;
}

s32 sceNpTusCreateTitleCtx()
{
	UNIMPLEMENTED_FUNC(sceNpTus);

	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

s32 sceNpTusDestroyTitleCtx()
{
	UNIMPLEMENTED_FUNC(sceNpTus);

	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

s32 sceNpTusCreateTransactionCtx()
{
	UNIMPLEMENTED_FUNC(sceNpTus);

	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

s32 sceNpTusDestroyTransactionCtx()
{
	UNIMPLEMENTED_FUNC(sceNpTus);

	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

s32 sceNpTusSetTimeout()
{
	UNIMPLEMENTED_FUNC(sceNpTus);

	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

s32 sceNpTusAbortTransaction()
{
	UNIMPLEMENTED_FUNC(sceNpTus);

	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

s32 sceNpTusWaitAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);

	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

s32 sceNpTusPollAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);

	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

s32 sceNpTusSetMultiSlotVariable()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusSetMultiSlotVariableVUser()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusSetMultiSlotVariableAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusSetMultiSlotVariableVUserAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusGetMultiSlotVariable()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusGetMultiSlotVariableVUser()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusGetMultiSlotVariableAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusGetMultiSlotVariableVUserAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusGetMultiUserVariable()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusGetMultiUserVariableVUser()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusGetMultiUserVariableAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusGetMultiUserVariableVUserAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusAddAndGetVariable()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusAddAndGetVariableVUser()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusAddAndGetVariableAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusAddAndGetVariableVUserAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusTryAndSetVariable()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusTryAndSetVariableVUser()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusTryAndSetVariableAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusTryAndSetVariableVUserAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusDeleteMultiSlotVariable()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusDeleteMultiSlotVariableVUser()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusDeleteMultiSlotVariableAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusDeleteMultiSlotVariableVUserAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusSetData()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusSetDataVUser()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusSetDataAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusSetDataVUserAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusGetData()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusGetDataVUser()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusGetDataAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusGetDataVUserAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusGetMultiSlotDataStatus()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
		
	return CELL_OK;
}

s32 sceNpTusGetMultiSlotDataStatusVUser()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusGetMultiSlotDataStatusAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusGetMultiSlotDataStatusVUserAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusGetMultiUserDataStatus()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusGetMultiUserDataStatusVUser()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusGetMultiUserDataStatusAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusGetMultiUserDataStatusVUserAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusDeleteMultiSlotData()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusDeleteMultiSlotDataVUser()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusDeleteMultiSlotDataAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

s32 sceNpTusDeleteMultiSlotDataVUserAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

Module sceNpTus("sceNpTus", []()
{
	sceNpTusInstance.m_bSceNpTusInitialized = false;

	REG_FUNC(sceNpTus, sceNpTusInit);
	REG_FUNC(sceNpTus, sceNpTusTerm);
	REG_FUNC(sceNpTus, sceNpTusCreateTitleCtx);
	REG_FUNC(sceNpTus, sceNpTusDestroyTitleCtx);
	REG_FUNC(sceNpTus, sceNpTusCreateTransactionCtx);
	REG_FUNC(sceNpTus, sceNpTusDestroyTransactionCtx);
	REG_FUNC(sceNpTus, sceNpTusSetTimeout);
	REG_FUNC(sceNpTus, sceNpTusAbortTransaction);
	REG_FUNC(sceNpTus, sceNpTusWaitAsync);
	REG_FUNC(sceNpTus, sceNpTusPollAsync);
	REG_FUNC(sceNpTus, sceNpTusSetMultiSlotVariable);
	REG_FUNC(sceNpTus, sceNpTusSetMultiSlotVariableVUser);
	REG_FUNC(sceNpTus, sceNpTusSetMultiSlotVariableAsync);
	REG_FUNC(sceNpTus, sceNpTusSetMultiSlotVariableVUserAsync);
	REG_FUNC(sceNpTus, sceNpTusGetMultiSlotVariable);
	REG_FUNC(sceNpTus, sceNpTusGetMultiSlotVariableVUser);
	REG_FUNC(sceNpTus, sceNpTusGetMultiSlotVariableAsync);
	REG_FUNC(sceNpTus, sceNpTusGetMultiSlotVariableVUserAsync);
	REG_FUNC(sceNpTus, sceNpTusGetMultiUserVariable);
	REG_FUNC(sceNpTus, sceNpTusGetMultiUserVariableVUser);
	REG_FUNC(sceNpTus, sceNpTusGetMultiUserVariableAsync);
	REG_FUNC(sceNpTus, sceNpTusGetMultiUserVariableVUserAsync);
	REG_FUNC(sceNpTus, sceNpTusAddAndGetVariable);
	REG_FUNC(sceNpTus, sceNpTusAddAndGetVariableVUser);
	REG_FUNC(sceNpTus, sceNpTusAddAndGetVariableAsync);
	REG_FUNC(sceNpTus, sceNpTusAddAndGetVariableVUserAsync);
	REG_FUNC(sceNpTus, sceNpTusTryAndSetVariable);
	REG_FUNC(sceNpTus, sceNpTusTryAndSetVariableVUser);
	REG_FUNC(sceNpTus, sceNpTusTryAndSetVariableAsync);
	REG_FUNC(sceNpTus, sceNpTusTryAndSetVariableVUserAsync);
	REG_FUNC(sceNpTus, sceNpTusDeleteMultiSlotVariable);
	REG_FUNC(sceNpTus, sceNpTusDeleteMultiSlotVariableVUser);
	REG_FUNC(sceNpTus, sceNpTusDeleteMultiSlotVariableAsync);
	REG_FUNC(sceNpTus, sceNpTusDeleteMultiSlotVariableVUserAsync);
	REG_FUNC(sceNpTus, sceNpTusSetData);
	REG_FUNC(sceNpTus, sceNpTusSetDataVUser);
	REG_FUNC(sceNpTus, sceNpTusSetDataAsync);
	REG_FUNC(sceNpTus, sceNpTusSetDataVUserAsync);
	REG_FUNC(sceNpTus, sceNpTusGetData);
	REG_FUNC(sceNpTus, sceNpTusGetDataVUser);
	REG_FUNC(sceNpTus, sceNpTusGetDataAsync);
	REG_FUNC(sceNpTus, sceNpTusGetDataVUserAsync);
	REG_FUNC(sceNpTus, sceNpTusGetMultiSlotDataStatus);
	REG_FUNC(sceNpTus, sceNpTusGetMultiSlotDataStatusVUser);
	REG_FUNC(sceNpTus, sceNpTusGetMultiSlotDataStatusAsync);
	REG_FUNC(sceNpTus, sceNpTusGetMultiSlotDataStatusVUserAsync);
	REG_FUNC(sceNpTus, sceNpTusGetMultiUserDataStatus);
	REG_FUNC(sceNpTus, sceNpTusGetMultiUserDataStatusVUser);
	REG_FUNC(sceNpTus, sceNpTusGetMultiUserDataStatusAsync);
	REG_FUNC(sceNpTus, sceNpTusGetMultiUserDataStatusVUserAsync);
	REG_FUNC(sceNpTus, sceNpTusDeleteMultiSlotData);
	REG_FUNC(sceNpTus, sceNpTusDeleteMultiSlotDataVUser);
	REG_FUNC(sceNpTus, sceNpTusDeleteMultiSlotDataAsync);
	REG_FUNC(sceNpTus, sceNpTusDeleteMultiSlotDataVUserAsync);
});
