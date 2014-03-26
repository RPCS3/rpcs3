#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "wx/xml/xml.h"

#include "sceNp.h"
#include "sceNpTrophy.h"

#include "Loader/TRP.h"
#include "Loader/TROPUSR.h"
#include "Emu/SysCalls/lv2/SC_Time.h"

void sceNpTrophy_unload();
void sceNpTrophy_init();
Module sceNpTrophy(0xf035, sceNpTrophy_init, nullptr, sceNpTrophy_unload);

// Internal Structs
struct sceNpTrophyInternalContext
{
	// TODO
	std::string trp_name;
	vfsStream* trp_stream;

	TROPUSRLoader* tropusr;
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
		return SCE_NP_TROPHY_ERROR_NOT_SUPPORTED;
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

	TRPLoader trp(*(ctxt.trp_stream));
	if (!trp.LoadHeader())
		return SCE_NP_TROPHY_ERROR_ILLEGAL_UPDATE;

	// Rename or discard certain entries based on the files found
	char target [32];
	sprintf(target, "TROP_%02d.SFM", Ini.SysLanguage.GetValue());

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
		sprintf(target, "TROP_%02d.SFM", i);
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
	ctxt.tropusr = tropusr;

	// TODO: Callbacks
	
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

	wxString path;
	wxXmlDocument doc;
	sceNpTrophyInternalContext& ctxt = s_npTrophyInstance.contexts[context];
	Emu.GetVFS().GetDevice("/dev_hdd0/home/00000001/trophy/" + ctxt.trp_name + "/TROPCONF.SFM", path);  // TODO: Get the path of the current user
	doc.Load(path);

	std::string titleName;
	std::string titleDetail;
	for (wxXmlNode *n = doc.GetRoot()->GetChildren(); n; n = n->GetNext()) {
		if (n->GetName() == "title-name")
			titleName = n->GetNodeContent().mb_str();
		if (n->GetName() == "title-detail")
			titleDetail = n->GetNodeContent().mb_str();
		if (n->GetName() == "trophy")
		{
			u32 trophy_id = atoi(n->GetAttribute("id").mb_str());
			
			details->numTrophies++;
			switch (((const char *)n->GetAttribute("ttype").mb_str())[0]) {
			case 'B': details->numBronze++;   break;
			case 'S': details->numSilver++;   break;
			case 'G': details->numGold++;     break;
			case 'P': details->numPlatinum++; break;
			}
			
			if (ctxt.tropusr->GetTrophyUnlockState(trophy_id))
			{
				data->unlockedTrophies++;
				switch (((const char *)n->GetAttribute("ttype").mb_str())[0]) {
				case 'B': data->unlockedBronze++;   break;
				case 'S': data->unlockedSilver++;   break;
				case 'G': data->unlockedGold++;     break;
				case 'P': data->unlockedPlatinum++; break;
				}
			}
		}
	}

	memcpy(details->title, titleName.c_str(), SCE_NP_TROPHY_NAME_MAX_SIZE);
	memcpy(details->description, titleDetail.c_str(), SCE_NP_TROPHY_DESCR_MAX_SIZE);
	return CELL_OK;
}

int sceNpTrophyDestroyHandle()
{
	UNIMPLEMENTED_FUNC(sceNpTrophy);
	return CELL_OK;
}

int sceNpTrophyUnlockTrophy(u32 context, u32 handle, s32 trophyId, mem32_t platinumId)
{
	sceNpTrophy.Warning("sceNpTrophyUnlockTrophy(context=%d, handle=%d, trophyId=%d, platinumId_addr=0x%x)",
		context, handle, trophyId, platinumId.GetAddr());
	
	if (!s_npTrophyInstance.m_bInitialized)
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	if (!platinumId.IsGood())
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	// TODO: There are other possible errors

	sceNpTrophyInternalContext& ctxt = s_npTrophyInstance.contexts[context];
	if (trophyId >= ctxt.tropusr->GetTrophiesCount())
		return SCE_NP_TROPHY_ERROR_INVALID_TROPHY_ID;
	if (ctxt.tropusr->GetTrophyUnlockState(trophyId))
		return SCE_NP_TROPHY_ERROR_ALREADY_UNLOCKED;

	u64 timestamp1 = get_system_time(); // TODO: Either timestamp1 or timestamp2 is wrong
	u64 timestamp2 = get_system_time(); // TODO: Either timestamp1 or timestamp2 is wrong
	ctxt.tropusr->UnlockTrophy(trophyId, timestamp1, timestamp2);
	std::string trophyPath = "/dev_hdd0/home/00000001/trophy/" + ctxt.trp_name + "/TROPUSR.DAT";
	ctxt.tropusr->Save(trophyPath);

	platinumId = SCE_NP_TROPHY_INVALID_TROPHY_ID; // TODO
	return CELL_OK;
}

int sceNpTrophyTerm()
{
	UNIMPLEMENTED_FUNC(sceNpTrophy);
	return CELL_OK;
}

int sceNpTrophyGetTrophyUnlockState(u32 context, u32 handle, mem_ptr_t<SceNpTrophyFlagArray> flags, mem32_t count)
{
	sceNpTrophy.Warning("sceNpTrophyGetTrophyUnlockState(context=%d, handle=%d, flags_addr=0x%x, count_addr=0x%x)",
		context, handle, flags.GetAddr(), count.GetAddr());

	if (!s_npTrophyInstance.m_bInitialized)
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	if (!flags.IsGood() || !count.IsGood())
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	// TODO: There are other possible errors

	sceNpTrophyInternalContext& ctxt = s_npTrophyInstance.contexts[context];
	count = ctxt.tropusr->GetTrophiesCount();
	if (count.GetValue() > 128)
		ConLog.Warning("sceNpTrophyGetTrophyUnlockState: More than 128 trophies detected!");

	// Pack up to 128 bools in u32 flag_bits[4]
	for (u32 id=0; id<count.GetValue(); id++)
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

int sceNpTrophyGetTrophyInfo(u32 context, u32 handle, s32 trophyId, mem_ptr_t<SceNpTrophyDetails> details, mem_ptr_t<SceNpTrophyData> data)
{
	sceNpTrophy.Warning("sceNpTrophyGetTrophyInfo(context=%u, handle=%u, trophyId=%d, details_addr=0x%x, data_addr=0x%x)",
		context, handle, trophyId, details.GetAddr(), data.GetAddr());

	if (!s_npTrophyInstance.m_bInitialized)
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	if (!details.IsGood() || !data.IsGood())
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	// TODO: There are other possible errors

	wxString path;
	wxXmlDocument doc;
	sceNpTrophyInternalContext& ctxt = s_npTrophyInstance.contexts[context];
	Emu.GetVFS().GetDevice("/dev_hdd0/home/00000001/trophy/" + ctxt.trp_name + "/TROPCONF.SFM", path);  // TODO: Get the path of the current user
	doc.Load(path);

	std::string name;
	std::string detail;
	for (wxXmlNode *n = doc.GetRoot()->GetChildren(); n; n = n->GetNext()) {
		if (n->GetName() == "trophy" && (trophyId == atoi(n->GetAttribute("id").mb_str())))
		{
			details->trophyId = trophyId;
			switch (((const char *)n->GetAttribute("ttype").mb_str())[0]) {
			case 'B': details->trophyGrade = SCE_NP_TROPHY_GRADE_BRONZE;   break;
			case 'S': details->trophyGrade = SCE_NP_TROPHY_GRADE_SILVER;   break;
			case 'G': details->trophyGrade = SCE_NP_TROPHY_GRADE_GOLD;     break;
			case 'P': details->trophyGrade = SCE_NP_TROPHY_GRADE_PLATINUM; break;
			}

			switch (((const char *)n->GetAttribute("ttype").mb_str())[0]) {
			case 'y': details->hidden = true;  break;
			case 'n': details->hidden = false; break;
			}

			for (wxXmlNode *n2 = n->GetChildren(); n2; n2 = n2->GetNext()) {
				if (n2->GetName() == "name")   name = n2->GetNodeContent().mb_str();
				if (n2->GetName() == "detail") detail = n2->GetNodeContent().mb_str();
			}

			data->trophyId = trophyId;
			data->unlocked = ctxt.tropusr->GetTrophyUnlockState(trophyId);
			data->timestamp.tick = ctxt.tropusr->GetTrophyTimestamp(trophyId);
		}		
	}

	memcpy(details->name, name.c_str(), SCE_NP_TROPHY_NAME_MAX_SIZE);
	memcpy(details->description, detail.c_str(), SCE_NP_TROPHY_DESCR_MAX_SIZE);
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
