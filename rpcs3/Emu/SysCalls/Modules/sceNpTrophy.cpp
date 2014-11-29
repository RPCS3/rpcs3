#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/Callback.h"

#include "rpcs3/Ini.h"
#include "Utilities/rXml.h"
#include "Loader/TRP.h"
#include "Loader/TROPUSR.h"
#include "Emu/FS/VFS.h"
#include "Emu/FS/vfsDir.h"
#include "Emu/FS/vfsFileBase.h"
#include "Emu/SysCalls/lv2/sys_time.h"
#include "sceNp.h"
#include "sceNpTrophy.h"

Module *sceNpTrophy = nullptr;

// Internal Structs
struct sceNpTrophyInternalContext
{
	// TODO
	std::string trp_name;
	std::unique_ptr<vfsStream> trp_stream;

	std::unique_ptr<TROPUSRLoader> tropusr;

// TODO: remove the following code when Visual C++ no longer generates
//       compiler errors for it. All of this should be auto-generated
#if defined(_MSC_VER) && _MSC_VER <= 1800
	sceNpTrophyInternalContext()
		: trp_stream(),
		tropusr()
	{
	}

	sceNpTrophyInternalContext(sceNpTrophyInternalContext&& other)
	{
		std::swap(trp_stream,other.trp_stream);
		std::swap(tropusr, other.tropusr);
		std::swap(trp_name, other.trp_name);
	}

	sceNpTrophyInternalContext& operator =(sceNpTrophyInternalContext&& other)
	{
		std::swap(trp_stream, other.trp_stream);
		std::swap(tropusr, other.tropusr);
		std::swap(trp_name, other.trp_name);
		return *this;
	}

	sceNpTrophyInternalContext(sceNpTrophyInternalContext& other) = delete;
	sceNpTrophyInternalContext& operator =(sceNpTrophyInternalContext& other) = delete;
#endif

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

sceNpTrophyInternal sceNpTrophyInstance;

// Functions
int sceNpTrophyInit(u32 pool_addr, u32 poolSize, u32 containerId, u64 options)
{
	sceNpTrophy->Log("sceNpTrophyInit(pool_addr=0x%x, poolSize=%d, containerId=%d, options=0x%llx)", pool_addr, poolSize, containerId, options);

	if (sceNpTrophyInstance.m_bInitialized)
		return SCE_NP_TROPHY_ERROR_ALREADY_INITIALIZED;
	if (options)
		return SCE_NP_TROPHY_ERROR_NOT_SUPPORTED;

	sceNpTrophyInstance.m_bInitialized = true;

	return CELL_OK;
}

int sceNpTrophyCreateContext(vm::ptr<u32> context, vm::ptr<SceNpCommunicationId> commID, vm::ptr<SceNpCommunicationSignature> commSign, u64 options)
{
	sceNpTrophy->Warning("sceNpTrophyCreateContext(context_addr=0x%x, commID_addr=0x%x, commSign_addr=0x%x, options=0x%llx)",
		context.addr(), commID.addr(), commSign.addr(), options);

	if (!sceNpTrophyInstance.m_bInitialized)
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	if (options & (~(u64)1))
		return SCE_NP_TROPHY_ERROR_NOT_SUPPORTED;
	// TODO: There are other possible errors

	// TODO: Is the TROPHY.TRP file necessarily located in this path?
	vfsDir dir("/app_home/../TROPDIR/");
	if(!dir.IsOpened())
		return SCE_NP_TROPHY_ERROR_CONF_DOES_NOT_EXIST;

	// TODO: Following method will retrieve the TROPHY.TRP of the first folder that contains such file
	for(const DirEntryInfo* entry = dir.Read(); entry; entry = dir.Read())
	{
		if (entry->flags & DirEntry_TypeDir)
		{
			vfsStream* stream = Emu.GetVFS().OpenFile("/app_home/../TROPDIR/" + entry->name + "/TROPHY.TRP", vfsRead);

			if (stream && stream->IsOpened())
			{
				sceNpTrophyInstance.contexts.emplace_back();
				sceNpTrophyInternalContext& ctxt = sceNpTrophyInstance.contexts.back();
				ctxt.trp_stream.reset(stream);
				ctxt.trp_name = entry->name;
				stream = nullptr;
				return CELL_OK;
			}
		}
	}

	return SCE_NP_TROPHY_ERROR_CONF_DOES_NOT_EXIST;
}

int sceNpTrophyCreateHandle(vm::ptr<u32> handle)
{
	sceNpTrophy->Warning("sceNpTrophyCreateHandle(handle_addr=0x%x)", handle.addr());

	if (!sceNpTrophyInstance.m_bInitialized)
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	// TODO: There are other possible errors

	// TODO: ?

	return CELL_OK;
}

int sceNpTrophyRegisterContext(u32 context, u32 handle, vm::ptr<SceNpTrophyStatusCallback> statusCb, u32 arg_addr, u64 options)
{
	sceNpTrophy->Warning("sceNpTrophyRegisterContext(context=%d, handle=%d, statusCb_addr=0x%x, arg_addr=0x%x, options=0x%llx)",
		context, handle, statusCb.addr(), arg_addr, options);

	if (!(sceNpTrophyInstance.m_bInitialized))
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	if (options & (~(u64)1))
		return SCE_NP_TROPHY_ERROR_NOT_SUPPORTED;
	if (context >= sceNpTrophyInstance.contexts.size())
		return SCE_NP_TROPHY_ERROR_UNKNOWN_CONTEXT;
	// TODO: There are other possible errors

	sceNpTrophyInternalContext& ctxt = sceNpTrophyInstance.contexts[context];
	if (!ctxt.trp_stream)
		return SCE_NP_TROPHY_ERROR_CONF_DOES_NOT_EXIST;

	TRPLoader trp(*(ctxt.trp_stream));
	if (!trp.LoadHeader())
		return SCE_NP_TROPHY_ERROR_ILLEGAL_UPDATE;

	// Rename or discard certain entries based on the files found
	const size_t kTargetBufferLength = 31;
	char target[kTargetBufferLength+1];
	target[kTargetBufferLength] = 0;
	strcpy_trunc(target, fmt::Format("TROP_%02d.SFM", Ini.SysLanguage.GetValue()));

	if (trp.ContainsEntry(target)) {
		trp.RemoveEntry("TROPCONF.SFM");
		trp.RemoveEntry("TROP.SFM");
		trp.RenameEntry(target, "TROPCONF.SFM");
	}
	else if (trp.ContainsEntry("TROP.SFM")) {
		trp.RemoveEntry("TROPCONF.SFM");
		trp.RenameEntry("TROP.SFM", "TROPCONF.SFM");
	}
	else if (!trp.ContainsEntry("TROPCONF.SFM")) {
		return SCE_NP_TROPHY_ERROR_ILLEGAL_UPDATE;
	}

	// Discard unnecessary TROP_XX.SFM files
	for (int i=0; i<=18; i++) {
		strcpy_trunc(target, fmt::Format("TROP_%02d.SFM", i));
		if (i != Ini.SysLanguage.GetValue())
			trp.RemoveEntry(target);
	}

	// TODO: Get the path of the current user
	std::string trophyPath = "/dev_hdd0/home/00000001/trophy/" + ctxt.trp_name;
	if (!trp.Install(trophyPath))
		return SCE_NP_TROPHY_ERROR_ILLEGAL_UPDATE;
	
	TROPUSRLoader* tropusr = new TROPUSRLoader();
	std::string trophyUsrPath = trophyPath + "/TROPUSR.DAT";
	std::string trophyConfPath = trophyPath + "/TROPCONF.SFM";
	tropusr->Load(trophyUsrPath, trophyConfPath);
	ctxt.tropusr.reset(tropusr);

	// TODO: Callbacks
	statusCb(context, SCE_NP_TROPHY_STATUS_INSTALLED, 100, 100, arg_addr);
	statusCb(context, SCE_NP_TROPHY_STATUS_PROCESSING_COMPLETE, 100, 100, arg_addr);
	
	return CELL_OK;
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

int sceNpTrophyGetRequiredDiskSpace(u32 context, u32 handle, vm::ptr<u64> reqspace, u64 options)
{
	sceNpTrophy->Warning("sceNpTrophyGetRequiredDiskSpace(context=%d, handle=%d, reqspace_addr=0x%x, options=0x%llx)",
		context, handle, reqspace.addr(), options);

	if (!sceNpTrophyInstance.m_bInitialized)
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	if (context >= sceNpTrophyInstance.contexts.size())
		return SCE_NP_TROPHY_ERROR_UNKNOWN_CONTEXT;
	// TODO: There are other possible errors

	sceNpTrophyInternalContext& ctxt = sceNpTrophyInstance.contexts[context];
	if (!ctxt.trp_stream)
		return SCE_NP_TROPHY_ERROR_CONF_DOES_NOT_EXIST;

	*reqspace = ctxt.trp_stream->GetSize(); // TODO: This is not accurate. It's just an approximation of the real value
	return CELL_OK;
}

int sceNpTrophyDestroyContext()
{
	UNIMPLEMENTED_FUNC(sceNpTrophy);
	return CELL_OK;
}

int sceNpTrophyAbortHandle(u32 handle)
{
	sceNpTrophy->Todo("sceNpTrophyAbortHandle(handle=%d)", handle);

	// TODO: ?

	if (!sceNpTrophyInstance.m_bInitialized)
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpTrophyGetGameInfo(u32 context, u32 handle, vm::ptr<SceNpTrophyGameDetails> details, vm::ptr<SceNpTrophyGameData> data)
{
	sceNpTrophy->Warning("sceNpTrophyGetGameInfo(context=%d, handle=%d, details_addr=0x%x, data_addr=0x%x)",
		context, handle, details.addr(), data.addr());

	if (!sceNpTrophyInstance.m_bInitialized)
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	// TODO: There are other possible errors

	std::string path;
	rXmlDocument doc;
	sceNpTrophyInternalContext& ctxt = sceNpTrophyInstance.contexts[context];
	Emu.GetVFS().GetDevice("/dev_hdd0/home/00000001/trophy/" + ctxt.trp_name + "/TROPCONF.SFM", path);  // TODO: Get the path of the current user
	doc.Load(path);

	std::string titleName;
	std::string titleDetail;
	for (std::shared_ptr<rXmlNode> n = doc.GetRoot()->GetChildren(); n; n = n->GetNext()) {
		if (n->GetName() == "title-name")
			titleName = n->GetNodeContent();
		if (n->GetName() == "title-detail")
			titleDetail = n->GetNodeContent();
		if (n->GetName() == "trophy")
		{
			u32 trophy_id = atoi(n->GetAttribute("id").c_str());
			
			details->numTrophies++;
			switch (n->GetAttribute("ttype")[0]) {
			case 'B': details->numBronze++;   break;
			case 'S': details->numSilver++;   break;
			case 'G': details->numGold++;     break;
			case 'P': details->numPlatinum++; break;
			}
			
			if (ctxt.tropusr->GetTrophyUnlockState(trophy_id))
			{
				data->unlockedTrophies++;
				switch (n->GetAttribute("ttype")[0]) {
				case 'B': data->unlockedBronze++;   break;
				case 'S': data->unlockedSilver++;   break;
				case 'G': data->unlockedGold++;     break;
				case 'P': data->unlockedPlatinum++; break;
				}
			}
		}
	}

	memcpy(details->title, titleName.c_str(), std::min((size_t) SCE_NP_TROPHY_NAME_MAX_SIZE, titleName.length() + 1));
	memcpy(details->description, titleDetail.c_str(), std::min((size_t) SCE_NP_TROPHY_DESCR_MAX_SIZE, titleDetail.length() + 1));
	return CELL_OK;
}

int sceNpTrophyDestroyHandle()
{
	UNIMPLEMENTED_FUNC(sceNpTrophy);
	return CELL_OK;
}

int sceNpTrophyUnlockTrophy(u32 context, u32 handle, s32 trophyId, vm::ptr<u32> platinumId)
{
	sceNpTrophy->Warning("sceNpTrophyUnlockTrophy(context=%d, handle=%d, trophyId=%d, platinumId_addr=0x%x)",
		context, handle, trophyId, platinumId.addr());
	
	if (!sceNpTrophyInstance.m_bInitialized)
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	// TODO: There are other possible errors

	sceNpTrophyInternalContext& ctxt = sceNpTrophyInstance.contexts[context];
	if (trophyId >= (s32)ctxt.tropusr->GetTrophiesCount())
		return SCE_NP_TROPHY_ERROR_INVALID_TROPHY_ID;
	if (ctxt.tropusr->GetTrophyUnlockState(trophyId))
		return SCE_NP_TROPHY_ERROR_ALREADY_UNLOCKED;

	u64 timestamp1 = get_system_time(); // TODO: Either timestamp1 or timestamp2 is wrong
	u64 timestamp2 = get_system_time(); // TODO: Either timestamp1 or timestamp2 is wrong
	ctxt.tropusr->UnlockTrophy(trophyId, timestamp1, timestamp2);
	std::string trophyPath = "/dev_hdd0/home/00000001/trophy/" + ctxt.trp_name + "/TROPUSR.DAT";
	ctxt.tropusr->Save(trophyPath);

	*platinumId = SCE_NP_TROPHY_INVALID_TROPHY_ID; // TODO
	return CELL_OK;
}

int sceNpTrophyTerm()
{
	sceNpTrophy->Warning("sceNpTrophyTerm()");

	if (!sceNpTrophyInstance.m_bInitialized)
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;

	sceNpTrophyInstance.m_bInitialized = false;

	return CELL_OK;
}

int sceNpTrophyGetTrophyUnlockState(u32 context, u32 handle, vm::ptr<SceNpTrophyFlagArray> flags, vm::ptr<u32> count)
{
	sceNpTrophy->Warning("sceNpTrophyGetTrophyUnlockState(context=%d, handle=%d, flags_addr=0x%x, count_addr=0x%x)",
		context, handle, flags.addr(), count.addr());

	if (!sceNpTrophyInstance.m_bInitialized)
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	// TODO: There are other possible errors

	sceNpTrophyInternalContext& ctxt = sceNpTrophyInstance.contexts[context];
	*count = ctxt.tropusr->GetTrophiesCount();
	if (*count > 128)
		sceNpTrophy->Warning("sceNpTrophyGetTrophyUnlockState: More than 128 trophies detected!");

	// Pack up to 128 bools in u32 flag_bits[4]
	for (u32 id=0; id<*count; id++)
	{
		if (ctxt.tropusr->GetTrophyUnlockState(id))
			flags->flag_bits[id/32] |= 1<<(id%32);
		else
			flags->flag_bits[id/32] &= ~(1<<(id%32));
	}

	return CELL_OK;
}

int sceNpTrophyGetTrophyIcon()
{
	UNIMPLEMENTED_FUNC(sceNpTrophy);
	return CELL_OK;
}

int sceNpTrophyGetTrophyInfo(u32 context, u32 handle, s32 trophyId, vm::ptr<SceNpTrophyDetails> details, vm::ptr<SceNpTrophyData> data)
{
	sceNpTrophy->Warning("sceNpTrophyGetTrophyInfo(context=%u, handle=%u, trophyId=%d, details_addr=0x%x, data_addr=0x%x)",
		context, handle, trophyId, details.addr(), data.addr());

	if (!sceNpTrophyInstance.m_bInitialized)
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	// TODO: There are other possible errors
	
	std::string path;
	rXmlDocument doc;
	sceNpTrophyInternalContext& ctxt = sceNpTrophyInstance.contexts[context];
	Emu.GetVFS().GetDevice("/dev_hdd0/home/00000001/trophy/" + ctxt.trp_name + "/TROPCONF.SFM", path);  // TODO: Get the path of the current user
	doc.Load(path);

	std::string name;
	std::string detail;
	for (std::shared_ptr<rXmlNode> n = doc.GetRoot()->GetChildren(); n; n = n->GetNext()) {
		if (n->GetName() == "trophy" && (trophyId == atoi(n->GetAttribute("id").c_str())))
		{
			details->trophyId = trophyId;
			switch (n->GetAttribute("ttype")[0]) {
			case 'B': details->trophyGrade = SCE_NP_TROPHY_GRADE_BRONZE;   break;
			case 'S': details->trophyGrade = SCE_NP_TROPHY_GRADE_SILVER;   break;
			case 'G': details->trophyGrade = SCE_NP_TROPHY_GRADE_GOLD;     break;
			case 'P': details->trophyGrade = SCE_NP_TROPHY_GRADE_PLATINUM; break;
			}

			switch (n->GetAttribute("ttype")[0]) {
			case 'y': details->hidden = true;  break;
			case 'n': details->hidden = false; break;
			}

			for (std::shared_ptr<rXmlNode> n2 = n->GetChildren(); n2; n2 = n2->GetNext()) {
				if (n2->GetName() == "name")   name = n2->GetNodeContent();
				if (n2->GetName() == "detail") detail = n2->GetNodeContent();
			}

			data->trophyId = trophyId;
			data->unlocked = ctxt.tropusr->GetTrophyUnlockState(trophyId) ? true : false; // ???
			data->timestamp.tick = ctxt.tropusr->GetTrophyTimestamp(trophyId);
		}		
	}

	memcpy(details->name, name.c_str(), std::min((size_t) SCE_NP_TROPHY_NAME_MAX_SIZE, name.length() + 1));
	memcpy(details->description, detail.c_str(), std::min((size_t) SCE_NP_TROPHY_DESCR_MAX_SIZE, detail.length() + 1));
	return CELL_OK;
}

int sceNpTrophyGetGameIcon()
{
	UNIMPLEMENTED_FUNC(sceNpTrophy);
	return CELL_OK;
}

void sceNpTrophy_unload()
{
	sceNpTrophyInstance.m_bInitialized = false;
}

void sceNpTrophy_init(Module *pxThis)
{
	sceNpTrophy = pxThis;

	sceNpTrophy->AddFunc(0x079f0e87, sceNpTrophyGetGameProgress);
	sceNpTrophy->AddFunc(0x1197b52c, sceNpTrophyRegisterContext);
	sceNpTrophy->AddFunc(0x1c25470d, sceNpTrophyCreateHandle);
	sceNpTrophy->AddFunc(0x27deda93, sceNpTrophySetSoundLevel);
	sceNpTrophy->AddFunc(0x370136fe, sceNpTrophyGetRequiredDiskSpace);
	sceNpTrophy->AddFunc(0x3741ecc7, sceNpTrophyDestroyContext);
	sceNpTrophy->AddFunc(0x39567781, sceNpTrophyInit);
	sceNpTrophy->AddFunc(0x48bd97c7, sceNpTrophyAbortHandle);
	sceNpTrophy->AddFunc(0x49d18217, sceNpTrophyGetGameInfo);
	sceNpTrophy->AddFunc(0x623cd2dc, sceNpTrophyDestroyHandle);
	sceNpTrophy->AddFunc(0x8ceedd21, sceNpTrophyUnlockTrophy);
	sceNpTrophy->AddFunc(0xa7fabf4d, sceNpTrophyTerm);
	sceNpTrophy->AddFunc(0xb3ac3478, sceNpTrophyGetTrophyUnlockState);
	sceNpTrophy->AddFunc(0xbaedf689, sceNpTrophyGetTrophyIcon);
	sceNpTrophy->AddFunc(0xe3bf9a28, sceNpTrophyCreateContext);
	sceNpTrophy->AddFunc(0xfce6d30a, sceNpTrophyGetTrophyInfo);
	sceNpTrophy->AddFunc(0xff299e03, sceNpTrophyGetGameIcon);
}
