#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "Crypto/unedat.h"
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

	wxString drm_path = Memory.ReadString(drm_path_addr);
	wxString k_licensee_str;
	u8 k_licensee[0x10];
	for(int i = 0; i < 0x10; i++)
	{
		k_licensee[i] = Memory.Read8(k_licensee_addr + i);
		k_licensee_str += wxString::Format("%02x", k_licensee[i]);
	}

	sceNp.Warning("sceNpDrmIsAvailable: Found DRM license file at %s", drm_path.wx_str());
	sceNp.Warning("sceNpDrmIsAvailable: Using k_licensee 0x%s", k_licensee_str.wx_str());

	// Set the necessary file paths.
	wxString drm_file_name = drm_path.AfterLast('/'); 
	wxString titleID = drm_path.AfterFirst('/').AfterFirst('/').AfterFirst('/').BeforeFirst('/'); 

	wxString enc_drm_path = wxGetCwd() + drm_path;
	wxString dec_drm_path = wxGetCwd() + "/dev_hdd1/" + titleID + "/" + drm_file_name;

	wxString rap_dir_path = wxGetCwd() + "/dev_usb000/";
	wxString rap_file_path = rap_dir_path;
	
	// Search dev_usb000 for a compatible RAP file. 
	vfsDir *raps_dir = new vfsDir(rap_dir_path);
	if (!raps_dir->IsOpened())
		sceNp.Warning("sceNpDrmIsAvailable: Can't find RAP file for DRM!");
	else
	{
		Array<DirEntryInfo> entries = raps_dir->GetEntries();
		for (unsigned int i = 0; i < entries.GetCount(); i++)
		{
			if (entries[i].name.Contains(titleID))
			{
				rap_file_path += entries[i].name;
				break;
			}
		}
	}

	// Create a new directory under dev_hdd1/titleID to hold the decrypted data.
	wxString tmp_dir = wxGetCwd() + "/dev_hdd1/" + titleID;
	if (!wxDir::Exists(tmp_dir))
		wxMkdir(wxGetCwd() + "/dev_hdd1/" + titleID);

	// Decrypt this EDAT using the supplied k_licensee and matching RAP file.
	DecryptEDAT(enc_drm_path.ToStdString(), dec_drm_path.ToStdString(), 8, rap_file_path.ToStdString(), k_licensee, false);

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
