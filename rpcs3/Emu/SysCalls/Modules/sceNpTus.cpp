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

int sceNpTusInit()
{
	sceNpTus.Warning("sceNpTusInit()");

	if (sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_ALREADY_INITIALIZED;

	sceNpTusInstance.m_bSceNpTusInitialized = true;

	return CELL_OK;
}

int sceNpTusTerm()
{
	sceNpTus.Warning("sceNpTusTerm()");
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;

	sceNpTusInstance.m_bSceNpTusInitialized = false;

	return CELL_OK;
}

int sceNpTusCreateTitleCtx()
{
	UNIMPLEMENTED_FUNC(sceNpTus);

	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpTusDestroyTitleCtx()
{
	UNIMPLEMENTED_FUNC(sceNpTus);

	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpTusCreateTransactionCtx()
{
	UNIMPLEMENTED_FUNC(sceNpTus);

	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpTusDestroyTransactionCtx()
{
	UNIMPLEMENTED_FUNC(sceNpTus);

	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpTusSetTimeout()
{
	UNIMPLEMENTED_FUNC(sceNpTus);

	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpTusAbortTransaction()
{
	UNIMPLEMENTED_FUNC(sceNpTus);

	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpTusWaitAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);

	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpTusPollAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);

	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpTusSetMultiSlotVariable()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusSetMultiSlotVariableVUser()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusSetMultiSlotVariableAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusSetMultiSlotVariableVUserAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusGetMultiSlotVariable()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusGetMultiSlotVariableVUser()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusGetMultiSlotVariableAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusGetMultiSlotVariableVUserAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusGetMultiUserVariable()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusGetMultiUserVariableVUser()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusGetMultiUserVariableAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusGetMultiUserVariableVUserAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusAddAndGetVariable()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusAddAndGetVariableVUser()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusAddAndGetVariableAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusAddAndGetVariableVUserAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusTryAndSetVariable()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusTryAndSetVariableVUser()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusTryAndSetVariableAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusTryAndSetVariableVUserAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusDeleteMultiSlotVariable()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusDeleteMultiSlotVariableVUser()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusDeleteMultiSlotVariableAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusDeleteMultiSlotVariableVUserAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusSetData()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusSetDataVUser()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusSetDataAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusSetDataVUserAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusGetData()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusGetDataVUser()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusGetDataAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusGetDataVUserAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusGetMultiSlotDataStatus()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
		
	return CELL_OK;
}

int sceNpTusGetMultiSlotDataStatusVUser()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusGetMultiSlotDataStatusAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusGetMultiSlotDataStatusVUserAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusGetMultiUserDataStatus()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusGetMultiUserDataStatusVUser()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusGetMultiUserDataStatusAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusGetMultiUserDataStatusVUserAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusDeleteMultiSlotData()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusDeleteMultiSlotDataVUser()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusDeleteMultiSlotDataAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

int sceNpTusDeleteMultiSlotDataVUserAsync()
{
	UNIMPLEMENTED_FUNC(sceNpTus);
	
	if (!sceNpTusInstance.m_bSceNpTusInitialized)
		return SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED;
	
	return CELL_OK;
}

void sceNpTus_unload()
{
	sceNpTusInstance.m_bSceNpTusInitialized = false;
}

Module sceNpTus("sceNpTus", []()
{
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
