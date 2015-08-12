#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/Modules.h"

#include "rpcs3/Ini.h"
#include "Utilities/rXml.h"
#include "Loader/TRP.h"
#include "Loader/TROPUSR.h"
#include "Emu/FS/VFS.h"
#include "Emu/FS/vfsDir.h"
#include "Emu/FS/vfsFileBase.h"
#include "sceNp.h"
#include "sceNpTrophy.h"

extern Module sceNpTrophy;

struct trophy_context_t
{
	const u32 id = idm::get_last_id();

	std::string trp_name;
	std::unique_ptr<vfsStream> trp_stream;
	std::unique_ptr<TROPUSRLoader> tropusr;
};

struct trophy_handle_t
{
	const u32 id = idm::get_last_id();
};

// Functions
s32 sceNpTrophyInit(vm::ptr<void> pool, u32 poolSize, u32 containerId, u64 options)
{
	sceNpTrophy.Warning("sceNpTrophyInit(pool=*0x%x, poolSize=0x%x, containerId=0x%x, options=0x%llx)", pool, poolSize, containerId, options);

	return CELL_OK;
}

s32 sceNpTrophyTerm()
{
	sceNpTrophy.Warning("sceNpTrophyTerm()");

	return CELL_OK;
}

s32 sceNpTrophyCreateHandle(vm::ptr<u32> handle)
{
	sceNpTrophy.Warning("sceNpTrophyCreateHandle(handle=*0x%x)", handle);

	if (!handle)
	{
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	}

	*handle = idm::make<trophy_handle_t>();

	return CELL_OK;
}

s32 sceNpTrophyDestroyHandle(u32 handle)
{
	sceNpTrophy.Warning("sceNpTrophyDestroyHandle(handle=0x%x)", handle);

	const auto hndl = idm::get<trophy_handle_t>(handle);

	if (!hndl)
	{
		return SCE_NP_TROPHY_ERROR_UNKNOWN_HANDLE;
	}

	idm::remove<trophy_handle_t>(handle);

	return CELL_OK;
}

s32 sceNpTrophyAbortHandle(u32 handle)
{
	sceNpTrophy.Todo("sceNpTrophyAbortHandle(handle=0x%x)", handle);

	const auto hndl = idm::get<trophy_handle_t>(handle);

	if (!hndl)
	{
		return SCE_NP_TROPHY_ERROR_UNKNOWN_HANDLE;
	}

	return CELL_OK;
}

s32 sceNpTrophyCreateContext(vm::ptr<u32> context, vm::cptr<SceNpCommunicationId> commId, vm::cptr<SceNpCommunicationSignature> commSign, u64 options)
{
	sceNpTrophy.Warning("sceNpTrophyCreateContext(context=*0x%x, commId=*0x%x, commSign=*0x%x, options=0x%llx)", context, commId, commSign, options);

	// rough checks for further fmt::format call
	if (commId->term || commId->num > 99)
	{
		return SCE_NP_TROPHY_ERROR_INVALID_NP_COMM_ID;
	}

	// generate trophy context name
	std::string name = fmt::format("%s_%02d", commId->data, commId->num);

	// open trophy pack file
	std::unique_ptr<vfsStream> stream(Emu.GetVFS().OpenFile("/app_home/../TROPDIR/" + name + "/TROPHY.TRP", fom::read));

	// check if exists and opened
	if (!stream || !stream->IsOpened())
	{
		return SCE_NP_TROPHY_ERROR_CONF_DOES_NOT_EXIST;
	}

	// create trophy context
	const auto ctxt = idm::make_ptr<trophy_context_t>();

	// set trophy context parameters (could be passed to constructor through make_ptr call)
	ctxt->trp_name = std::move(name);
	ctxt->trp_stream = std::move(stream);
	*context = ctxt->id;

	return CELL_OK;
}

s32 sceNpTrophyDestroyContext(u32 context)
{
	sceNpTrophy.Warning("sceNpTrophyDestroyContext(context=0x%x)", context);

	const auto ctxt = idm::get<trophy_context_t>(context);

	if (!ctxt)
	{
		return SCE_NP_TROPHY_ERROR_UNKNOWN_CONTEXT;
	}

	idm::remove<trophy_context_t>(context);

	return CELL_OK;
}

s32 sceNpTrophyRegisterContext(PPUThread& CPU, u32 context, u32 handle, vm::ptr<SceNpTrophyStatusCallback> statusCb, vm::ptr<u32> arg, u64 options)
{
	sceNpTrophy.Error("sceNpTrophyRegisterContext(context=0x%x, handle=0x%x, statusCb=*0x%x, arg=*0x%x, options=0x%llx)", context, handle, statusCb, arg, options);

	const auto ctxt = idm::get<trophy_context_t>(context);

	if (!ctxt)
	{
		sceNpTrophy.Error("sceNpTrophyRegisterContext(): SCE_NP_TROPHY_ERROR_UNKNOWN_CONTEXT");
		return SCE_NP_TROPHY_ERROR_UNKNOWN_CONTEXT;
	}

	const auto hndl = idm::get<trophy_handle_t>(handle);

	if (!hndl)
	{
		sceNpTrophy.Error("sceNpTrophyRegisterContext(): SCE_NP_TROPHY_ERROR_UNKNOWN_HANDLE");
		return SCE_NP_TROPHY_ERROR_UNKNOWN_HANDLE;
	}

	TRPLoader trp(*ctxt->trp_stream);
	if (!trp.LoadHeader())
	{
		sceNpTrophy.Error("sceNpTrophyRegisterContext(): SCE_NP_TROPHY_ERROR_ILLEGAL_UPDATE");
		return SCE_NP_TROPHY_ERROR_ILLEGAL_UPDATE;
	}

	// Rename or discard certain entries based on the files found
	const size_t kTargetBufferLength = 31;
	char target[kTargetBufferLength + 1];
	target[kTargetBufferLength] = 0;
	strcpy_trunc(target, fmt::Format("TROP_%02d.SFM", Ini.SysLanguage.GetValue()));

	if (trp.ContainsEntry(target))
	{
		trp.RemoveEntry("TROPCONF.SFM");
		trp.RemoveEntry("TROP.SFM");
		trp.RenameEntry(target, "TROPCONF.SFM");
	}
	else if (trp.ContainsEntry("TROP.SFM"))
	{
		trp.RemoveEntry("TROPCONF.SFM");
		trp.RenameEntry("TROP.SFM", "TROPCONF.SFM");
	}
	else if (!trp.ContainsEntry("TROPCONF.SFM"))
	{
		return SCE_NP_TROPHY_ERROR_ILLEGAL_UPDATE;
	}

	// Discard unnecessary TROP_XX.SFM files
	for (s32 i = 0; i <= 18; i++)
	{
		strcpy_trunc(target, fmt::Format("TROP_%02d.SFM", i));
		if (i != Ini.SysLanguage.GetValue())
		{
			trp.RemoveEntry(target);
		}
	}

	// TODO: Get the path of the current user
	std::string trophyPath = "/dev_hdd0/home/00000001/trophy/" + ctxt->trp_name;
	if (!trp.Install(trophyPath))
	{
		sceNpTrophy.Error("sceNpTrophyRegisterContext(): SCE_NP_TROPHY_ERROR_ILLEGAL_UPDATE");
		return SCE_NP_TROPHY_ERROR_ILLEGAL_UPDATE;
	}
	
	TROPUSRLoader* tropusr = new TROPUSRLoader();
	std::string trophyUsrPath = trophyPath + "/TROPUSR.DAT";
	std::string trophyConfPath = trophyPath + "/TROPCONF.SFM";
	tropusr->Load(trophyUsrPath, trophyConfPath);
	ctxt->tropusr.reset(tropusr);

	// TODO: Callbacks
	statusCb(CPU, context, SCE_NP_TROPHY_STATUS_INSTALLED, 100, 100, arg);
	statusCb(CPU, context, SCE_NP_TROPHY_STATUS_PROCESSING_COMPLETE, 100, 100, arg);
	
	return CELL_OK;
}

s32 sceNpTrophyGetRequiredDiskSpace(u32 context, u32 handle, vm::ptr<u64> reqspace, u64 options)
{
	sceNpTrophy.Todo("sceNpTrophyGetRequiredDiskSpace(context=0x%x, handle=0x%x, reqspace*=0x%x, options=0x%llx)", context, handle, reqspace, options);

	const auto ctxt = idm::get<trophy_context_t>(context);

	if (!ctxt)
	{
		return SCE_NP_TROPHY_ERROR_UNKNOWN_CONTEXT;
	}

	const auto hndl = idm::get<trophy_handle_t>(handle);

	if (!hndl)
	{
		return SCE_NP_TROPHY_ERROR_UNKNOWN_HANDLE;
	}

	// TODO: This is not accurate. It's just an approximation of the real value
	*reqspace = ctxt->trp_stream->GetSize();

	return CELL_OK;
}

s32 sceNpTrophySetSoundLevel(u32 context, u32 handle, u32 level, u64 options)
{
	sceNpTrophy.Todo("sceNpTrophySetSoundLevel(context=0x%x, handle=0x%x, level=%d, options=0x%llx)", context, handle, level, options);

	return CELL_OK;
}

s32 sceNpTrophyGetGameInfo(u32 context, u32 handle, vm::ptr<SceNpTrophyGameDetails> details, vm::ptr<SceNpTrophyGameData> data)
{
	sceNpTrophy.Error("sceNpTrophyGetGameInfo(context=0x%x, handle=0x%x, details=*0x%x, data=*0x%x)", context, handle, details, data);

	const auto ctxt = idm::get<trophy_context_t>(context);

	if (!ctxt)
	{
		return SCE_NP_TROPHY_ERROR_UNKNOWN_CONTEXT;
	}

	const auto hndl = idm::get<trophy_handle_t>(handle);

	if (!hndl)
	{
		return SCE_NP_TROPHY_ERROR_UNKNOWN_HANDLE;
	}

	std::string path;
	rXmlDocument doc;
	Emu.GetVFS().GetDevice("/dev_hdd0/home/00000001/trophy/" + ctxt->trp_name + "/TROPCONF.SFM", path);  // TODO: Get the path of the current user
	doc.Load(path);

	std::string titleName;
	std::string titleDetail;
	for (std::shared_ptr<rXmlNode> n = doc.GetRoot()->GetChildren(); n; n = n->GetNext())
	{
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
			
			if (ctxt->tropusr->GetTrophyUnlockState(trophy_id))
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

	strcpy_trunc(details->title, titleName);
	strcpy_trunc(details->description, titleDetail);
	return CELL_OK;
}

s32 sceNpTrophyUnlockTrophy(u32 context, u32 handle, s32 trophyId, vm::ptr<u32> platinumId)
{
	sceNpTrophy.Error("sceNpTrophyUnlockTrophy(context=0x%x, handle=0x%x, trophyId=%d, platinumId=*0x%x)", context, handle, trophyId, platinumId);

	const auto ctxt = idm::get<trophy_context_t>(context);

	if (!ctxt)
	{
		return SCE_NP_TROPHY_ERROR_UNKNOWN_CONTEXT;
	}

	const auto hndl = idm::get<trophy_handle_t>(handle);

	if (!hndl)
	{
		return SCE_NP_TROPHY_ERROR_UNKNOWN_HANDLE;
	}

	if (trophyId >= (s32)ctxt->tropusr->GetTrophiesCount())
		return SCE_NP_TROPHY_ERROR_INVALID_TROPHY_ID;
	if (ctxt->tropusr->GetTrophyUnlockState(trophyId))
		return SCE_NP_TROPHY_ERROR_ALREADY_UNLOCKED;

	ctxt->tropusr->UnlockTrophy(trophyId, 0, 0); // TODO
	std::string trophyPath = "/dev_hdd0/home/00000001/trophy/" + ctxt->trp_name + "/TROPUSR.DAT";
	ctxt->tropusr->Save(trophyPath);

	*platinumId = SCE_NP_TROPHY_INVALID_TROPHY_ID; // TODO
	return CELL_OK;
}

s32 sceNpTrophyGetTrophyUnlockState(u32 context, u32 handle, vm::ptr<SceNpTrophyFlagArray> flags, vm::ptr<u32> count)
{
	sceNpTrophy.Error("sceNpTrophyGetTrophyUnlockState(context=0x%x, handle=0x%x, flags=*0x%x, count=*0x%x)", context, handle, flags, count);

	const auto ctxt = idm::get<trophy_context_t>(context);

	if (!ctxt)
	{
		return SCE_NP_TROPHY_ERROR_UNKNOWN_CONTEXT;
	}

	const auto hndl = idm::get<trophy_handle_t>(handle);

	if (!hndl)
	{
		return SCE_NP_TROPHY_ERROR_UNKNOWN_HANDLE;
	}

	u32 count_ = ctxt->tropusr->GetTrophiesCount();
	*count = count_;
	if (count_ > 128)
		sceNpTrophy.Error("sceNpTrophyGetTrophyUnlockState: More than 128 trophies detected!");

	// Pack up to 128 bools in u32 flag_bits[4]
	for (u32 id = 0; id < count_; id++)
	{
		if (ctxt->tropusr->GetTrophyUnlockState(id))
			flags->flag_bits[id/32] |= 1<<(id%32);
		else
			flags->flag_bits[id/32] &= ~(1<<(id%32));
	}

	return CELL_OK;
}

s32 sceNpTrophyGetTrophyInfo(u32 context, u32 handle, s32 trophyId, vm::ptr<SceNpTrophyDetails> details, vm::ptr<SceNpTrophyData> data)
{
	sceNpTrophy.Warning("sceNpTrophyGetTrophyInfo(context=0x%x, handle=0x%x, trophyId=%d, details=*0x%x, data=*0x%x)", context, handle, trophyId, details, data);

	const auto ctxt = idm::get<trophy_context_t>(context);

	if (!ctxt)
	{
		return SCE_NP_TROPHY_ERROR_UNKNOWN_CONTEXT;
	}

	const auto hndl = idm::get<trophy_handle_t>(handle);

	if (!hndl)
	{
		return SCE_NP_TROPHY_ERROR_UNKNOWN_HANDLE;
	}
	
	std::string path;
	rXmlDocument doc;
	Emu.GetVFS().GetDevice("/dev_hdd0/home/00000001/trophy/" + ctxt->trp_name + "/TROPCONF.SFM", path);  // TODO: Get the path of the current user
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
			data->unlocked = ctxt->tropusr->GetTrophyUnlockState(trophyId) != 0; // ???
			data->timestamp = ctxt->tropusr->GetTrophyTimestamp(trophyId);
		}		
	}

	memcpy(details->name, name.c_str(), std::min((size_t) SCE_NP_TROPHY_NAME_MAX_SIZE, name.length() + 1));
	memcpy(details->description, detail.c_str(), std::min((size_t) SCE_NP_TROPHY_DESCR_MAX_SIZE, detail.length() + 1));
	return CELL_OK;
}

s32 sceNpTrophyGetGameProgress(u32 context, u32 handle, vm::ptr<s32> percentage)
{
	sceNpTrophy.Todo("sceNpTrophyGetGameProgress(context=0x%x, handle=0x%x, percentage=*0x%x)", context, handle, percentage);

	return CELL_OK;
}

s32 sceNpTrophyGetGameIcon(u32 context, u32 handle, vm::ptr<void> buffer, vm::ptr<u32> size)
{
	sceNpTrophy.Todo("sceNpTrophyGetGameIcon(context=0x%x, handle=0x%x, buffer=*0x%x, size=*0x%x)", context, handle, buffer, size);

	return CELL_OK;
}

s32 sceNpTrophyGetTrophyIcon(u32 context, u32 handle, s32 trophyId, vm::ptr<void> buffer, vm::ptr<u32> size)
{
	sceNpTrophy.Todo("sceNpTrophyGetTrophyIcon(context=0x%x, handle=0x%x, trophyId=%d, buffer=*0x%x, size=*0x%x)", context, handle, trophyId, buffer, size);

	return CELL_OK;
}


Module sceNpTrophy("sceNpTrophy", []()
{
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
