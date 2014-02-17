#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

#include "sceNp.h"
#include "Loader/TRP.h"

void sceNpTrophy_unload();
void sceNpTrophy_init();
Module sceNpTrophy(0xf035, sceNpTrophy_init, nullptr, sceNpTrophy_unload);

enum
{
	SCE_NP_TROPHY_ERROR_ALREADY_INITIALIZED      = 0x80022901,
	SCE_NP_TROPHY_ERROR_NOT_INITIALIZED          = 0x80022902,
	SCE_NP_TROPHY_ERROR_NOT_SUPPORTED            = 0x80022903,
	SCE_NP_TROPHY_ERROR_CONTEXT_NOT_REGISTERED   = 0x80022904,
	SCE_NP_TROPHY_ERROR_OUT_OF_MEMORY            = 0x80022905,
	SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT         = 0x80022906,
	SCE_NP_TROPHY_ERROR_EXCEEDS_MAX              = 0x80022907,
	SCE_NP_TROPHY_ERROR_INSUFFICIENT             = 0x80022909,
	SCE_NP_TROPHY_ERROR_UNKNOWN_CONTEXT          = 0x8002290a,
	SCE_NP_TROPHY_ERROR_INVALID_FORMAT           = 0x8002290b,
	SCE_NP_TROPHY_ERROR_BAD_RESPONSE             = 0x8002290c,
	SCE_NP_TROPHY_ERROR_INVALID_GRADE            = 0x8002290d,
	SCE_NP_TROPHY_ERROR_INVALID_CONTEXT          = 0x8002290e,
	SCE_NP_TROPHY_ERROR_PROCESSING_ABORTED       = 0x8002290f,
	SCE_NP_TROPHY_ERROR_ABORT                    = 0x80022910,
	SCE_NP_TROPHY_ERROR_UNKNOWN_HANDLE           = 0x80022911,
	SCE_NP_TROPHY_ERROR_LOCKED                   = 0x80022912,
	SCE_NP_TROPHY_ERROR_HIDDEN                   = 0x80022913,
	SCE_NP_TROPHY_ERROR_CANNOT_UNLOCK_PLATINUM   = 0x80022914,
	SCE_NP_TROPHY_ERROR_ALREADY_UNLOCKED         = 0x80022915,
	SCE_NP_TROPHY_ERROR_INVALID_TYPE             = 0x80022916,
	SCE_NP_TROPHY_ERROR_INVALID_HANDLE           = 0x80022917,
	SCE_NP_TROPHY_ERROR_INVALID_NP_COMM_ID       = 0x80022918,
	SCE_NP_TROPHY_ERROR_UNKNOWN_NP_COMM_ID       = 0x80022919,
	SCE_NP_TROPHY_ERROR_DISC_IO                  = 0x8002291a,
	SCE_NP_TROPHY_ERROR_CONF_DOES_NOT_EXIST      = 0x8002291b,
	SCE_NP_TROPHY_ERROR_UNSUPPORTED_FORMAT       = 0x8002291c,
	SCE_NP_TROPHY_ERROR_ALREADY_INSTALLED        = 0x8002291d,
	SCE_NP_TROPHY_ERROR_BROKEN_DATA              = 0x8002291e,
	SCE_NP_TROPHY_ERROR_VERIFICATION_FAILURE     = 0x8002291f,
	SCE_NP_TROPHY_ERROR_INVALID_TROPHY_ID        = 0x80022920,
	SCE_NP_TROPHY_ERROR_UNKNOWN_TROPHY_ID        = 0x80022921,
	SCE_NP_TROPHY_ERROR_UNKNOWN_TITLE            = 0x80022922,
	SCE_NP_TROPHY_ERROR_UNKNOWN_FILE             = 0x80022923,
	SCE_NP_TROPHY_ERROR_DISC_NOT_MOUNTED         = 0x80022924,
	SCE_NP_TROPHY_ERROR_SHUTDOWN                 = 0x80022925,
	SCE_NP_TROPHY_ERROR_TITLE_ICON_NOT_FOUND     = 0x80022926,
	SCE_NP_TROPHY_ERROR_TROPHY_ICON_NOT_FOUND    = 0x80022927,
	SCE_NP_TROPHY_ERROR_INSUFFICIENT_DISK_SPACE  = 0x80022928,
	SCE_NP_TROPHY_ERROR_ILLEGAL_UPDATE           = 0x8002292a,
	SCE_NP_TROPHY_ERROR_SAVEDATA_USER_DOES_NOT_MATCH = 0x8002292b,
	SCE_NP_TROPHY_ERROR_TROPHY_ID_DOES_NOT_EXIST = 0x8002292c,
	SCE_NP_TROPHY_ERROR_SERVICE_UNAVAILABLE      = 0x8002292d,
	SCE_NP_TROPHY_ERROR_UNKNOWN                  = 0x800229ff,
};


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
			std::shared_ptr<vfsFileBase> f(Emu.GetVFS().OpenFile("/app_home/TROPDIR/" + entry->name + "/TROPHY.TRP", vfsRead));

			if (f && f->IsOpened())
			{
				sceNpTrophyInternalContext ctxt;
				ctxt.trp_stream = f.get();
				ctxt.trp_name = entry->name;
				s_npTrophyInstance.contexts.push_back(ctxt);
				f = nullptr;
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
		SCE_NP_TROPHY_ERROR_NOT_SUPPORTED;
	// TODO: There are other possible errors

	int ret;
	sceNpTrophyInternalContext& ctxt = s_npTrophyInstance.contexts[context];
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

int sceNpTrophyGetRequiredDiskSpace()
{
	UNIMPLEMENTED_FUNC(sceNpTrophy);
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

int sceNpTrophyGetGameInfo()
{
	UNIMPLEMENTED_FUNC(sceNpTrophy);
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

int sceNpTrophyGetTrophyInfo()
{
	UNIMPLEMENTED_FUNC(sceNpTrophy);
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