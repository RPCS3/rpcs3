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
	sceNpTus.AddFunc(0x8f87a06b, sceNpTusInit);
	sceNpTus.AddFunc(0x225aed26, sceNpTusTerm);
	sceNpTus.AddFunc(0x7caf58ee, sceNpTusCreateTitleCtx);
	sceNpTus.AddFunc(0x2e162a62, sceNpTusDestroyTitleCtx);
	sceNpTus.AddFunc(0x1904435e, sceNpTusCreateTransactionCtx);
	sceNpTus.AddFunc(0x44eca8b4, sceNpTusDestroyTransactionCtx);
	sceNpTus.AddFunc(0x59432970, sceNpTusSetTimeout);
	sceNpTus.AddFunc(0x325c6284, sceNpTusAbortTransaction);
	sceNpTus.AddFunc(0xb8e8ff22, sceNpTusWaitAsync);
	sceNpTus.AddFunc(0x19bce18c, sceNpTusPollAsync);
	sceNpTus.AddFunc(0xcc86a8f6, sceNpTusSetMultiSlotVariable);
	sceNpTus.AddFunc(0xf819be91, sceNpTusSetMultiSlotVariableVUser);
	sceNpTus.AddFunc(0x065b610d, sceNpTusSetMultiSlotVariableAsync);
	sceNpTus.AddFunc(0x96a06212, sceNpTusSetMultiSlotVariableVUserAsync);
	sceNpTus.AddFunc(0x0423e622, sceNpTusGetMultiSlotVariable);
	sceNpTus.AddFunc(0x2357ba9e, sceNpTusGetMultiSlotVariableVUser);
	sceNpTus.AddFunc(0xbb2877f2, sceNpTusGetMultiSlotVariableAsync);
	sceNpTus.AddFunc(0xfc7d346e, sceNpTusGetMultiSlotVariableVUserAsync);
	sceNpTus.AddFunc(0x0d15043b, sceNpTusGetMultiUserVariable);
	sceNpTus.AddFunc(0x6c511024, sceNpTusGetMultiUserVariableVUser);
	sceNpTus.AddFunc(0xcc7a31cd, sceNpTusGetMultiUserVariableAsync);
	sceNpTus.AddFunc(0x9549d22c, sceNpTusGetMultiUserVariableVUserAsync);
	sceNpTus.AddFunc(0x94989003, sceNpTusAddAndGetVariable);
	sceNpTus.AddFunc(0xf60be06f, sceNpTusAddAndGetVariableVUser);
	sceNpTus.AddFunc(0x1fa5c87d, sceNpTusAddAndGetVariableAsync);
	sceNpTus.AddFunc(0xa7993bf3, sceNpTusAddAndGetVariableVUserAsync);
	sceNpTus.AddFunc(0x47e9424a, sceNpTusTryAndSetVariable);
	sceNpTus.AddFunc(0x3602bc80, sceNpTusTryAndSetVariableVUser);
	sceNpTus.AddFunc(0xbbb244b7, sceNpTusTryAndSetVariableAsync);
	sceNpTus.AddFunc(0x17db7aa7, sceNpTusTryAndSetVariableVUserAsync);
	sceNpTus.AddFunc(0xaf985783, sceNpTusDeleteMultiSlotVariable);
	sceNpTus.AddFunc(0xc4e51fbf, sceNpTusDeleteMultiSlotVariableVUser);
	sceNpTus.AddFunc(0xf5363608, sceNpTusDeleteMultiSlotVariableAsync);
	sceNpTus.AddFunc(0xc2e18da8, sceNpTusDeleteMultiSlotVariableVUserAsync);
	sceNpTus.AddFunc(0x7d5f0f0e, sceNpTusSetData);
	sceNpTus.AddFunc(0x0835deb2, sceNpTusSetDataVUser);
	sceNpTus.AddFunc(0xe847341f, sceNpTusSetDataAsync);
	sceNpTus.AddFunc(0x9cc0cf44, sceNpTusSetDataVUserAsync);
	sceNpTus.AddFunc(0x8ddd0d85, sceNpTusGetData);
	sceNpTus.AddFunc(0xae4e590e, sceNpTusGetDataVUser);
	sceNpTus.AddFunc(0x5175abb9, sceNpTusGetDataAsync);
	sceNpTus.AddFunc(0x38f364b0, sceNpTusGetDataVUserAsync);
	sceNpTus.AddFunc(0xc848d425, sceNpTusGetMultiSlotDataStatus);
	sceNpTus.AddFunc(0xa3abfadb, sceNpTusGetMultiSlotDataStatusVUser);
	sceNpTus.AddFunc(0x651fd79f, sceNpTusGetMultiSlotDataStatusAsync);
	sceNpTus.AddFunc(0x2ab21ea9, sceNpTusGetMultiSlotDataStatusVUserAsync);
	sceNpTus.AddFunc(0x348dbcb4, sceNpTusGetMultiUserDataStatus);
	sceNpTus.AddFunc(0x2d1b9f1a, sceNpTusGetMultiUserDataStatusVUser);
	sceNpTus.AddFunc(0xc66ba67e, sceNpTusGetMultiUserDataStatusAsync);
	sceNpTus.AddFunc(0x368fec59, sceNpTusGetMultiUserDataStatusVUserAsync);
	sceNpTus.AddFunc(0xe0719847, sceNpTusDeleteMultiSlotData);
	sceNpTus.AddFunc(0x01711e81, sceNpTusDeleteMultiSlotDataVUser);
	sceNpTus.AddFunc(0x3175af23, sceNpTusDeleteMultiSlotDataAsync);
	sceNpTus.AddFunc(0xc815b219, sceNpTusDeleteMultiSlotDataVUserAsync);
});
