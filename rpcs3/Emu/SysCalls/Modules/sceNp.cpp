#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

#include "sceNp.h"

void sceNp_init();
Module sceNp(0x0016, sceNp_init);

int sceNpManagerGetStatus(mem32_t status)
{
	sceNp.Log("sceNpManagerGetStatus(status_addr=0x%x)", status.GetAddr());

	// TODO: Check if sceNpInit() was called, if not return SCE_NP_ERROR_NOT_INITIALIZED
	if (!status.IsGood())
		return SCE_NP_ERROR_INVALID_ARGUMENT;

	// TODO: Support different statuses
	status = SCE_NP_MANAGER_STATUS_OFFLINE;
	return CELL_OK;
}

void sceNp_init()
{
	sceNp.AddFunc(0xa7bff757, sceNpManagerGetStatus);
}
