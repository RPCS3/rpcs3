#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/CB_FUNC.h"

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

extern Module sceNpTrophy;

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

static sceNpTrophyInternalContext& getContext(u32 context) {
	// The invalid context is 0, so remap contexts 1... to indices 0...
	if (context == 0)
		throw "getContext: context == 0";
	return sceNpTrophyInstance.contexts[context - 1];
}

// Functions
int sceNpTrophyInit(u32 pool_addr, u32 poolSize, u32 containerId, u64 options)
{
	sceNpTrophy.Log("sceNpTrophyInit(pool_addr=0x%x, poolSize=%d, containerId=0x%x, options=0x%llx)", pool_addr, poolSize, containerId, options);

	if (sceNpTrophyInstance.m_bInitialized)
		return SCE_NP_TROPHY_ERROR_ALREADY_INITIALIZED;
	if (options)
		return SCE_NP_TROPHY_ERROR_NOT_SUPPORTED;

	sceNpTrophyInstance.m_bInitialized = true;

	return CELL_OK;
}

int sceNpTrophyCreateContext(vm::ptr<u32> context, vm::cptr<SceNpCommunicationId> commID, vm::cptr<SceNpCommunicationSignature> commSign, u64 options)
{
	sceNpTrophy.Warning("sceNpTrophyCreateContext(context=*0x%x, commID=*0x%x, commSign=*0x%x, options=0x%llx)", context, commID, commSign, options);

	if (!sceNpTrophyInstance.m_bInitialized)
	{
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	}

	if (options & ~1)
	{
		return SCE_NP_TROPHY_ERROR_NOT_SUPPORTED;
	}
	// TODO: There are other possible errors

	// TODO: Is the TROPHY.TRP file necessarily located in this path?
	if (!Emu.GetVFS().ExistsDir("/app_home/../TROPDIR/"))
	{
		return SCE_NP_TROPHY_ERROR_CONF_DOES_NOT_EXIST;
	}

	// TODO: Following method will retrieve the TROPHY.TRP of the first folder that contains such file
	for (const auto entry : vfsDir("/app_home/../TROPDIR/"))
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
				*context = sceNpTrophyInstance.contexts.size(); // contexts start from 1
				return CELL_OK;
			}
		}
	}

	return SCE_NP_TROPHY_ERROR_CONF_DOES_NOT_EXIST;
}

int sceNpTrophyCreateHandle(vm::ptr<u32> handle)
{
	sceNpTrophy.Todo("sceNpTrophyCreateHandle(handle_addr=0x%x)", handle.addr());

	if (!sceNpTrophyInstance.m_bInitialized)
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	// TODO: There are other possible errors

	// TODO: ?

	return CELL_OK;
}

int sceNpTrophyRegisterContext(PPUThread& CPU, u32 context, u32 handle, vm::ptr<SceNpTrophyStatusCallback> statusCb, u32 arg_addr, u64 options)
{
	sceNpTrophy.Warning("sceNpTrophyRegisterContext(context=0x%x, handle=0x%x, statusCb_addr=0x%x, arg_addr=0x%x, options=0x%llx)",
		context, handle, statusCb.addr(), arg_addr, options);

	if (!(sceNpTrophyInstance.m_bInitialized))
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	if (options & (~(u64)1))
		return SCE_NP_TROPHY_ERROR_NOT_SUPPORTED;
	if (context == 0 || context > sceNpTrophyInstance.contexts.size()) {
		sceNpTrophy.Warning("sceNpTrophyRegisterContext: invalid context (%d)", context);
		return SCE_NP_TROPHY_ERROR_UNKNOWN_CONTEXT;
	}
	// TODO: There are other possible errors

	sceNpTrophyInternalContext& ctxt = getContext(context);
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
	statusCb(CPU, context, SCE_NP_TROPHY_STATUS_INSTALLED, 100, 100, arg_addr);
	statusCb(CPU, context, SCE_NP_TROPHY_STATUS_PROCESSING_COMPLETE, 100, 100, arg_addr);
	
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
	sceNpTrophy.Warning("sceNpTrophyGetRequiredDiskSpace(context=0x%x, handle=0x%x, reqspace_addr=0x%x, options=0x%llx)",
		context, handle, reqspace.addr(), options);

	if (!sceNpTrophyInstance.m_bInitialized)
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	if (context == 0 || context > sceNpTrophyInstance.contexts.size()) {
		sceNpTrophy.Warning("sceNpTrophyGetRequiredDiskSpace: invalid context (%d)", context);
		return SCE_NP_TROPHY_ERROR_UNKNOWN_CONTEXT;
	}
	// TODO: There are other possible errors

	const sceNpTrophyInternalContext& ctxt = getContext(context);
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
	sceNpTrophy.Todo("sceNpTrophyAbortHandle(handle=0x%x)", handle);

	// TODO: ?

	if (!sceNpTrophyInstance.m_bInitialized)
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

int sceNpTrophyGetGameInfo(u32 context, u32 handle, vm::ptr<SceNpTrophyGameDetails> details, vm::ptr<SceNpTrophyGameData> data)
{
	sceNpTrophy.Warning("sceNpTrophyGetGameInfo(context=0x%x, handle=0x%x, details_addr=0x%x, data_addr=0x%x)",
		context, handle, details.addr(), data.addr());

	if (!sceNpTrophyInstance.m_bInitialized)
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	// TODO: There are other possible errors

	std::string path;
	rXmlDocument doc;
	const sceNpTrophyInternalContext& ctxt = getContext(context);
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
	sceNpTrophy.Warning("sceNpTrophyUnlockTrophy(context=0x%x, handle=0x%x, trophyId=%d, platinumId_addr=0x%x)",
		context, handle, trophyId, platinumId.addr());
	
	if (!sceNpTrophyInstance.m_bInitialized)
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	// TODO: There are other possible errors

	sceNpTrophyInternalContext& ctxt = getContext(context);
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
	sceNpTrophy.Warning("sceNpTrophyTerm()");

	if (!sceNpTrophyInstance.m_bInitialized)
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;

	sceNpTrophyInstance.m_bInitialized = false;

	return CELL_OK;
}

int sceNpTrophyGetTrophyUnlockState(u32 context, u32 handle, vm::ptr<SceNpTrophyFlagArray> flags, vm::ptr<u32> count)
{
	sceNpTrophy.Warning("sceNpTrophyGetTrophyUnlockState(context=0x%x, handle=0x%x, flags_addr=0x%x, count_addr=0x%x)",
		context, handle, flags.addr(), count.addr());

	if (!sceNpTrophyInstance.m_bInitialized)
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	if (context == 0 || context > sceNpTrophyInstance.contexts.size()) {
		sceNpTrophy.Warning("sceNpTrophyGetTrophyUnlockState: invalid context (%d)", context);
		return SCE_NP_TROPHY_ERROR_UNKNOWN_CONTEXT;
	}
	// TODO: There are other possible errors

	const sceNpTrophyInternalContext& ctxt = getContext(context);
	u32 count_ = ctxt.tropusr->GetTrophiesCount();
	*count = count_;
	if (count_ > 128)
		sceNpTrophy.Warning("sceNpTrophyGetTrophyUnlockState: More than 128 trophies detected!");

	// Pack up to 128 bools in u32 flag_bits[4]
	for (u32 id = 0; id < count_; id++)
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
	sceNpTrophy.Warning("sceNpTrophyGetTrophyInfo(context=0x%x, handle=0x%x, trophyId=%d, details_addr=0x%x, data_addr=0x%x)",
		context, handle, trophyId, details.addr(), data.addr());

	if (!sceNpTrophyInstance.m_bInitialized)
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	// TODO: There are other possible errors
	
	std::string path;
	rXmlDocument doc;
	const sceNpTrophyInternalContext& ctxt = getContext(context);
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

Module sceNpTrophy("sceNpTrophy", []()
{
	sceNpTrophyInstance.m_bInitialized = false;

	REG_FUNC(sceNpTrophy, sceNpTrophyGetGameProgress);
	REG_FUNC(sceNpTrophy, sceNpTrophyRegisterContext);
	REG_FUNC(sceNpTrophy, sceNpTrophyCreateHandle);
	REG_FUNC(sceNpTrophy, sceNpTrophySetSoundLevel);
	REG_FUNC(sceNpTrophy, sceNpTrophyGetRequiredDiskSpace);
	REG_FUNC(sceNpTrophy, sceNpTrophyDestroyContext);
	REG_FUNC(sceNpTrophy, sceNpTrophyInit);
	REG_FUNC(sceNpTrophy, sceNpTrophyAbortHandle);
	REG_FUNC(sceNpTrophy, sceNpTrophyGetGameInfo);
	REG_FUNC(sceNpTrophy, sceNpTrophyDestroyHandle);
	REG_FUNC(sceNpTrophy, sceNpTrophyUnlockTrophy);
	REG_FUNC(sceNpTrophy, sceNpTrophyTerm);
	REG_FUNC(sceNpTrophy, sceNpTrophyGetTrophyUnlockState);
	REG_FUNC(sceNpTrophy, sceNpTrophyGetTrophyIcon);
	REG_FUNC(sceNpTrophy, sceNpTrophyCreateContext);
	REG_FUNC(sceNpTrophy, sceNpTrophyGetTrophyInfo);
	REG_FUNC(sceNpTrophy, sceNpTrophyGetGameIcon);
});
