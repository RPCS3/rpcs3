#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

#include "sceNp.h"
#include "sceNpTrophy.h"
#include "Loader/TRP.h"

void sceNpTrophy_unload();
void sceNpTrophy_init();
Module sceNpTrophy(0xf035, sceNpTrophy_init, nullptr, sceNpTrophy_unload);

// Internal Structs
struct sceNpTrophyInternalContext
{
	// TODO
	std::string trp_name;
	vfsStream* trp_stream;
};

struct sceNpTrophyInternal
{
	bool m_bInitialized;
	std::vector<sceNpTrophyInternalContext> contexts;

	sceNpTrophyInternal()
		: m_bInitialized(false)
	{
	}
};

sceNpTrophyInternal s_npTrophyInstance;

// Functions
int sceNpTrophyInit(u32 pool_addr, u32 poolSize, u32 containerId, u64 options)
{
	sceNpTrophy.Log("sceNpTrophyInit(pool_addr=0x%x, poolSize=%d, containerId=%d, options=0x%llx)", pool_addr, poolSize, containerId, options);

	if (s_npTrophyInstance.m_bInitialized)
		return SCE_NP_TROPHY_ERROR_ALREADY_INITIALIZED;
	if (options)
		return SCE_NP_TROPHY_ERROR_NOT_SUPPORTED;

	s_npTrophyInstance.m_bInitialized = true;
	return CELL_OK;
}

int sceNpTrophyCreateContext(mem32_t context, mem_ptr_t<SceNpCommunicationId> commID, mem_ptr_t<SceNpCommunicationSignature> commSign, u64 options)
{
	sceNpTrophy.Warning("sceNpTrophyCreateContext(context_addr=0x%x, commID_addr=0x%x, commSign_addr=0x%x, options=0x%llx)",
		context.GetAddr(), commID.GetAddr(), commSign.GetAddr(), options);

	if (!s_npTrophyInstance.m_bInitialized)
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	if (!context.IsGood())
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	if (options & (~(u64)1))
		SCE_NP_TROPHY_ERROR_NOT_SUPPORTED;
	// TODO: There are other possible errors

	// TODO: Is the TROPHY.TRP file necessarily located in this path?
	vfsDir dir("/app_home/TROPDIR/");
	if(!dir.IsOpened())
		return SCE_NP_TROPHY_ERROR_CONF_DOES_NOT_EXIST;

	// TODO: Following method will retrieve the TROPHY.TRP of the first folder that contains such file
	for(const DirEntryInfo* entry = dir.Read(); entry; entry = dir.Read())
	{
		if (entry->flags & DirEntry_TypeDir)
		{
			vfsStream* stream = Emu.GetVFS().OpenFile("/app_home/TROPDIR/" + entry->name + "/TROPHY.TRP", vfsRead);

			if (stream && stream->IsOpened())
			{
				sceNpTrophyInternalContext ctxt;
				ctxt.trp_stream = stream;
				ctxt.trp_name = entry->name;
				s_npTrophyInstance.contexts.push_back(ctxt);
				stream = nullptr;
				return CELL_OK;
			}
		}
	}

	return SCE_NP_TROPHY_ERROR_CONF_DOES_NOT_EXIST;
}

int sceNpTrophyCreateHandle(mem32_t handle)
{
	sceNpTrophy.Warning("sceNpTrophyCreateHandle(handle_addr=0x%x)", handle.GetAddr());

	if (!s_npTrophyInstance.m_bInitialized)
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	if (!handle.IsGood())
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	// TODO: There are other possible errors

	// TODO: ?

	return CELL_OK;
}

int sceNpTrophyRegisterContext(u32 context, u32 handle, u32 statusCb_addr, u32 arg_addr, u64 options)
{
	sceNpTrophy.Warning("sceNpTrophyRegisterContext(context=%d, handle=%d, statusCb_addr=0x%x, arg_addr=0x%x, options=0x%llx)",
		context, handle, statusCb_addr, arg_addr, options);

	if (!(s_npTrophyInstance.m_bInitialized))
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	if (!Memory.IsGoodAddr(statusCb_addr))
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	if (options & (~(u64)1))
		return SCE_NP_TROPHY_ERROR_NOT_SUPPORTED;
	if (context >= s_npTrophyInstance.contexts.size())
		return SCE_NP_TROPHY_ERROR_UNKNOWN_CONTEXT;
	// TODO: There are other possible errors

	sceNpTrophyInternalContext& ctxt = s_npTrophyInstance.contexts[context];

	if (!ctxt.trp_stream)
		return SCE_NP_TROPHY_ERROR_CONF_DOES_NOT_EXIST;

	int ret;
	TRPLoader trp(*(ctxt.trp_stream));

	// TODO: Get the path of the current user
	if (trp.Install("/dev_hdd0/home/00000001/trophy/" + ctxt.trp_name))
		ret = CELL_OK;
	else
		ret = SCE_NP_TROPHY_ERROR_ILLEGAL_UPDATE;
	
	// TODO: Callbacks
	
	trp.Close();
	return ret;
}

int sceNpTrophyGetGameProgress()
{
	UNIMPLEMENTED_FUNC(sceNpTrophy);
	return CELL_OK;
}

int sceNpTrophySetSoundLevel()
{
	UNIMPLEMENTED_FUNC(sceNpTrophy);
	return CELL_OK;
}

int sceNpTrophyGetRequiredDiskSpace(u32 context, u32 handle, mem64_t reqspace, u64 options)
{
	sceNpTrophy.Warning("sceNpTrophyGetRequiredDiskSpace(context=%d, handle=%d, reqspace_addr=0x%x, options=0x%llx)",
		context, handle, reqspace.GetAddr(), options);

	if (!s_npTrophyInstance.m_bInitialized)
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	if (!reqspace.IsGood())
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	if (context >= s_npTrophyInstance.contexts.size())
		return SCE_NP_TROPHY_ERROR_UNKNOWN_CONTEXT;
	// TODO: There are other possible errors

	sceNpTrophyInternalContext& ctxt = s_npTrophyInstance.contexts[context];

	if (!ctxt.trp_stream)
		return SCE_NP_TROPHY_ERROR_CONF_DOES_NOT_EXIST;

	reqspace = ctxt.trp_stream->GetSize(); // TODO: This is not accurate. It's just an approximation of the real value
	return CELL_OK;
}

int sceNpTrophyDestroyContext()
{
	UNIMPLEMENTED_FUNC(sceNpTrophy);
	return CELL_OK;
}

int sceNpTrophyAbortHandle()
{
	UNIMPLEMENTED_FUNC(sceNpTrophy);
	return CELL_OK;
}

int sceNpTrophyGetGameInfo(u32 context, u32 handle, mem_ptr_t<SceNpTrophyGameDetails> details, mem_ptr_t<SceNpTrophyGameData> data)
{
	sceNpTrophy.Warning("sceNpTrophyGetGameInfo(context=%d, handle=%d, details_addr=0x%x, data_addr=0x%x)",
		context, handle, details.GetAddr(), data.GetAddr());

	if (!s_npTrophyInstance.m_bInitialized)
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	if (!details.IsGood() || !data.IsGood())
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	// TODO: There are other possible errors

	// sceNpTrophyInternalContext& ctxt = s_npTrophyInstance.contexts[context];

	// TODO: This data is faked, implement a XML reader and get it from TROP.SFM
	memcpy(details->title, "Trophy Set", SCE_NP_TROPHY_NAME_MAX_SIZE);
	memcpy(details->description, "Hey! Implement a XML reader, and load the description from TROP.SFM", SCE_NP_TROPHY_DESCR_MAX_SIZE);
	details->numTrophies = 0;
	details->numPlatinum = 0;
	details->numGold = 0;
	details->numSilver = 0;
	details->numBronze = 0;
	data->unlockedTrophies = 0;
	data->unlockedPlatinum = 0;
	data->unlockedGold = 0;
	data->unlockedSilver = 0;
	data->unlockedBronze = 0;
	return CELL_OK;
}

int sceNpTrophyDestroyHandle()
{
	UNIMPLEMENTED_FUNC(sceNpTrophy);
	return CELL_OK;
}

int sceNpTrophyUnlockTrophy()
{
	UNIMPLEMENTED_FUNC(sceNpTrophy);
	return CELL_OK;
}

int sceNpTrophyTerm()
{
	UNIMPLEMENTED_FUNC(sceNpTrophy);
	return CELL_OK;
}

int sceNpTrophyGetTrophyUnlockState()
{
	UNIMPLEMENTED_FUNC(sceNpTrophy);
	return CELL_OK;
}

int sceNpTrophyGetTrophyIcon()
{
	UNIMPLEMENTED_FUNC(sceNpTrophy);
	return CELL_OK;
}

int sceNpTrophyGetTrophyInfo(u32 context, u32 handle, s32 trophyId, mem_ptr_t<SceNpTrophyDetails> details, mem_ptr_t<SceNpTrophyData> data)
{
	sceNpTrophy.Warning("sceNpTrophyGetTrophyInfo(context=%u, handle=%u, trophyId=%d, details_addr=0x%x, data_addr=0x%x)",
		context, handle, trophyId, details.GetAddr(), data.GetAddr());

	if (!s_npTrophyInstance.m_bInitialized)
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	if (!details.IsGood() || !data.IsGood())
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	// TODO: There are other possible errors

	// sceNpTrophyInternalContext& ctxt = s_npTrophyInstance.contexts[context];

	// TODO: This data is faked, implement a XML reader and get it from TROP.SFM
	memcpy(details->name, "Some Trophy", SCE_NP_TROPHY_NAME_MAX_SIZE);
	memcpy(details->description, "Hey! Implement a XML reader, and load the description from TROP.SFM", SCE_NP_TROPHY_DESCR_MAX_SIZE);
	details->hidden = false;
	details->trophyId = trophyId;
	details->trophyGrade = SCE_NP_TROPHY_GRADE_GOLD;
	return CELL_OK;
}

int sceNpTrophyGetGameIcon()
{
	UNIMPLEMENTED_FUNC(sceNpTrophy);
	return CELL_OK;
}

void sceNpTrophy_unload()
{
	s_npTrophyInstance.m_bInitialized = false;
}

void sceNpTrophy_init()
{
	sceNpTrophy.AddFunc(0x079f0e87, sceNpTrophyGetGameProgress);
	sceNpTrophy.AddFunc(0x1197b52c, sceNpTrophyRegisterContext);
	sceNpTrophy.AddFunc(0x1c25470d, sceNpTrophyCreateHandle);
	sceNpTrophy.AddFunc(0x27deda93, sceNpTrophySetSoundLevel);
	sceNpTrophy.AddFunc(0x370136fe, sceNpTrophyGetRequiredDiskSpace);
	sceNpTrophy.AddFunc(0x3741ecc7, sceNpTrophyDestroyContext);
	sceNpTrophy.AddFunc(0x39567781, sceNpTrophyInit);
	sceNpTrophy.AddFunc(0x48bd97c7, sceNpTrophyAbortHandle);
	sceNpTrophy.AddFunc(0x49d18217, sceNpTrophyGetGameInfo);
	sceNpTrophy.AddFunc(0x623cd2dc, sceNpTrophyDestroyHandle);
	sceNpTrophy.AddFunc(0x8ceedd21, sceNpTrophyUnlockTrophy);
	sceNpTrophy.AddFunc(0xa7fabf4d, sceNpTrophyTerm);
	sceNpTrophy.AddFunc(0xb3ac3478, sceNpTrophyGetTrophyUnlockState);
	sceNpTrophy.AddFunc(0xbaedf689, sceNpTrophyGetTrophyIcon);
	sceNpTrophy.AddFunc(0xe3bf9a28, sceNpTrophyCreateContext);
	sceNpTrophy.AddFunc(0xfce6d30a, sceNpTrophyGetTrophyInfo);
	sceNpTrophy.AddFunc(0xff299e03, sceNpTrophyGetGameIcon);
}