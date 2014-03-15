#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

#include "sceNp.h"

void sceNp_init();
Module sceNp(0x0016, sceNp_init);

int sceNpInit(u32 mem_size, u32 mem_addr)
{
	sceNp.Log("sceNpInit(mem_size=0x%x, mem_addr=0x%x)", mem_size, mem_addr);
	return CELL_OK;
}

int sceNpTerm()
{
	sceNp.Log("sceNpTerm");
	return CELL_OK;
}

int sceNpDrmIsAvailable(u32 k_licensee_addr, u32 drm_path_addr)
{
	sceNp.Warning("sceNpDrmIsAvailable(k_licensee_addr=0x%x, drm_path_addr=0x%x)", k_licensee_addr, drm_path_addr);

	wxString k_licensee_str;
	wxString drm_path = Memory.ReadString(drm_path_addr);
	u8 k_licensee[0x10];
	for(int i = 0; i < 0x10; i++)
	{
		k_licensee[i] = Memory.Read8(k_licensee_addr + i);
		k_licensee_str += wxString::Format("%02x", k_licensee[i]);
	}

	sceNp.Warning("sceNpDrmIsAvailable: Found DRM license file at %s", drm_path.c_str());
	sceNp.Warning("sceNpDrmIsAvailable: Using k_licensee 0x%s", k_licensee_str);

	return CELL_OK;
}

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
	sceNp.AddFunc(0xbd28fdbf, sceNpInit);
	sceNp.AddFunc(0x4885aa18, sceNpTerm);
	sceNp.AddFunc(0xad218faf, sceNpDrmIsAvailable);
	sceNp.AddFunc(0xa7bff757, sceNpManagerGetStatus);
}
