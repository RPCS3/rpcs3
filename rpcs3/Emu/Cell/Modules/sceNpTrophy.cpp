#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"

#include "restore_new.h"
#include "Utilities/rXml.h"
#include "define_new_memleakdetect.h"
#include "Loader/TRP.h"
#include "Loader/TROPUSR.h"

#include "sceNp.h"
#include "sceNpTrophy.h"

#include "Utilities/StrUtil.h"

logs::channel sceNpTrophy("sceNpTrophy");

struct trophy_context_t
{
	static const u32 id_base = 1;
	static const u32 id_step = 1;
	static const u32 id_count = 1023;

	std::string trp_name;
	fs::file trp_stream;
	std::unique_ptr<TROPUSRLoader> tropusr;
};

struct trophy_handle_t
{
	static const u32 id_base = 1;
	static const u32 id_step = 1;
	static const u32 id_count = 1023;
};

template<>
void fmt_class_string<SceNpTrophyError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
		STR_CASE(SCE_NP_TROPHY_ERROR_ALREADY_INITIALIZED);
		STR_CASE(SCE_NP_TROPHY_ERROR_NOT_INITIALIZED);
		STR_CASE(SCE_NP_TROPHY_ERROR_NOT_SUPPORTED);
		STR_CASE(SCE_NP_TROPHY_ERROR_CONTEXT_NOT_REGISTERED);
		STR_CASE(SCE_NP_TROPHY_ERROR_OUT_OF_MEMORY);
		STR_CASE(SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT);
		STR_CASE(SCE_NP_TROPHY_ERROR_EXCEEDS_MAX);
		STR_CASE(SCE_NP_TROPHY_ERROR_INSUFFICIENT);
		STR_CASE(SCE_NP_TROPHY_ERROR_UNKNOWN_CONTEXT);
		STR_CASE(SCE_NP_TROPHY_ERROR_INVALID_FORMAT);
		STR_CASE(SCE_NP_TROPHY_ERROR_BAD_RESPONSE);
		STR_CASE(SCE_NP_TROPHY_ERROR_INVALID_GRADE);
		STR_CASE(SCE_NP_TROPHY_ERROR_INVALID_CONTEXT);
		STR_CASE(SCE_NP_TROPHY_ERROR_PROCESSING_ABORTED);
		STR_CASE(SCE_NP_TROPHY_ERROR_ABORT);
		STR_CASE(SCE_NP_TROPHY_ERROR_UNKNOWN_HANDLE);
		STR_CASE(SCE_NP_TROPHY_ERROR_LOCKED);
		STR_CASE(SCE_NP_TROPHY_ERROR_HIDDEN);
		STR_CASE(SCE_NP_TROPHY_ERROR_CANNOT_UNLOCK_PLATINUM);
		STR_CASE(SCE_NP_TROPHY_ERROR_ALREADY_UNLOCKED);
		STR_CASE(SCE_NP_TROPHY_ERROR_INVALID_TYPE);
		STR_CASE(SCE_NP_TROPHY_ERROR_INVALID_HANDLE);
		STR_CASE(SCE_NP_TROPHY_ERROR_INVALID_NP_COMM_ID);
		STR_CASE(SCE_NP_TROPHY_ERROR_UNKNOWN_NP_COMM_ID);
		STR_CASE(SCE_NP_TROPHY_ERROR_DISC_IO);
		STR_CASE(SCE_NP_TROPHY_ERROR_CONF_DOES_NOT_EXIST);
		STR_CASE(SCE_NP_TROPHY_ERROR_UNSUPPORTED_FORMAT);
		STR_CASE(SCE_NP_TROPHY_ERROR_ALREADY_INSTALLED);
		STR_CASE(SCE_NP_TROPHY_ERROR_BROKEN_DATA);
		STR_CASE(SCE_NP_TROPHY_ERROR_VERIFICATION_FAILURE);
		STR_CASE(SCE_NP_TROPHY_ERROR_INVALID_TROPHY_ID);
		STR_CASE(SCE_NP_TROPHY_ERROR_UNKNOWN_TROPHY_ID);
		STR_CASE(SCE_NP_TROPHY_ERROR_UNKNOWN_TITLE);
		STR_CASE(SCE_NP_TROPHY_ERROR_UNKNOWN_FILE);
		STR_CASE(SCE_NP_TROPHY_ERROR_DISC_NOT_MOUNTED);
		STR_CASE(SCE_NP_TROPHY_ERROR_SHUTDOWN);
		STR_CASE(SCE_NP_TROPHY_ERROR_TITLE_ICON_NOT_FOUND);
		STR_CASE(SCE_NP_TROPHY_ERROR_TROPHY_ICON_NOT_FOUND);
		STR_CASE(SCE_NP_TROPHY_ERROR_INSUFFICIENT_DISK_SPACE);
		STR_CASE(SCE_NP_TROPHY_ERROR_ILLEGAL_UPDATE);
		STR_CASE(SCE_NP_TROPHY_ERROR_SAVEDATA_USER_DOES_NOT_MATCH);
		STR_CASE(SCE_NP_TROPHY_ERROR_TROPHY_ID_DOES_NOT_EXIST);
		STR_CASE(SCE_NP_TROPHY_ERROR_SERVICE_UNAVAILABLE);
		STR_CASE(SCE_NP_TROPHY_ERROR_UNKNOWN);
		}

		return unknown;
	});
}

// Functions
error_code sceNpTrophyInit(vm::ptr<void> pool, u32 poolSize, u32 containerId, u64 options)
{
	sceNpTrophy.warning("sceNpTrophyInit(pool=*0x%x, poolSize=0x%x, containerId=0x%x, options=0x%llx)", pool, poolSize, containerId, options);

	return CELL_OK;
}

error_code sceNpTrophyTerm()
{
	sceNpTrophy.warning("sceNpTrophyTerm()");

	return CELL_OK;
}

error_code sceNpTrophyCreateHandle(vm::ptr<u32> handle)
{
	sceNpTrophy.warning("sceNpTrophyCreateHandle(handle=*0x%x)", handle);

	if (!handle)
	{
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	}

	*handle = idm::make<trophy_handle_t>();

	return CELL_OK;
}

error_code sceNpTrophyDestroyHandle(u32 handle)
{
	sceNpTrophy.warning("sceNpTrophyDestroyHandle(handle=0x%x)", handle);

	const auto hndl = idm::get<trophy_handle_t>(handle);

	if (!hndl)
	{
		return SCE_NP_TROPHY_ERROR_UNKNOWN_HANDLE;
	}

	idm::remove<trophy_handle_t>(handle);

	return CELL_OK;
}

error_code sceNpTrophyAbortHandle(u32 handle)
{
	sceNpTrophy.todo("sceNpTrophyAbortHandle(handle=0x%x)", handle);

	const auto hndl = idm::get<trophy_handle_t>(handle);

	if (!hndl)
	{
		return SCE_NP_TROPHY_ERROR_UNKNOWN_HANDLE;
	}

	return CELL_OK;
}
void deleteTerminateChar(char* myStr, char _char) {

	char *del = &myStr[strlen(myStr)];

	while (del > myStr && *del != _char)
		del--;

	if (*del == _char)
		*del = '\0';

	return;
}
error_code sceNpTrophyCreateContext(vm::ptr<u32> context, vm::cptr<SceNpCommunicationId> commId, vm::cptr<SceNpCommunicationSignature> commSign, u64 options)
{
	sceNpTrophy.warning("sceNpTrophyCreateContext(context=*0x%x, commId=*0x%x, commSign=*0x%x, options=0x%llx)", context, commId, commSign, options);

	if (!context)
	{
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	}

	// rough checks for further fmt::format call
	if (commId->num > 99)
	{
		sceNpTrophy.error("sceNpTrophyCreateContext Invalid NP_COMM_ID");
		return SCE_NP_TROPHY_ERROR_INVALID_NP_COMM_ID;
	}
	// generate trophy context name
	std::string name;
	sceNpTrophy.warning("sceNpTrophyCreateContext term=%s data=%s num=%d", commId->term, commId->data, commId->num);
	if (commId->term)
	{
		char trimchar[9];
		memcpy(trimchar, commId->data, sizeof(trimchar));
		trimchar[8] = 0;
		deleteTerminateChar(trimchar, commId->term);
		name = fmt::format("%s_%02d", trimchar, commId->num);
	}
	else
	{
		name = fmt::format("%s_%02d", commId->data, commId->num);
	}

	// open trophy pack file
	fs::file stream(vfs::get("/app_home/../TROPDIR/" + name + "/TROPHY.TRP"));

	// check if exists and opened
	if (!stream)
	{
		sceNpTrophy.error("sceNpTrophyCreateContext CONF does not exist");
		return SCE_NP_TROPHY_ERROR_CONF_DOES_NOT_EXIST;
	}

	// create trophy context
	const auto ctxt = idm::make_ptr<trophy_context_t>();

	// set trophy context parameters (could be passed to constructor through make_ptr call)
	ctxt->trp_name = std::move(name);
	ctxt->trp_stream = std::move(stream);
	*context = idm::last_id();

	return CELL_OK;
}

error_code sceNpTrophyDestroyContext(u32 context)
{
	sceNpTrophy.warning("sceNpTrophyDestroyContext(context=0x%x)", context);

	const auto ctxt = idm::get<trophy_context_t>(context);

	if (!ctxt)
	{
		return SCE_NP_TROPHY_ERROR_UNKNOWN_CONTEXT;
	}

	idm::remove<trophy_context_t>(context);

	return CELL_OK;
}

error_code sceNpTrophyRegisterContext(ppu_thread& ppu, u32 context, u32 handle, vm::ptr<SceNpTrophyStatusCallback> statusCb, vm::ptr<void> arg, u64 options)
{
	sceNpTrophy.error("sceNpTrophyRegisterContext(context=0x%x, handle=0x%x, statusCb=*0x%x, arg=*0x%x, options=0x%llx)", context, handle, statusCb, arg, options);

	if (!statusCb)
	{
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	}

	const auto ctxt = idm::get<trophy_context_t>(context);

	if (!ctxt)
	{
		sceNpTrophy.error("sceNpTrophyRegisterContext(): SCE_NP_TROPHY_ERROR_UNKNOWN_CONTEXT");
		return SCE_NP_TROPHY_ERROR_UNKNOWN_CONTEXT;
	}

	const auto hndl = idm::get<trophy_handle_t>(handle);

	if (!hndl)
	{
		sceNpTrophy.error("sceNpTrophyRegisterContext(): SCE_NP_TROPHY_ERROR_UNKNOWN_HANDLE");
		return SCE_NP_TROPHY_ERROR_UNKNOWN_HANDLE;
	}

	TRPLoader trp(ctxt->trp_stream);
	if (!trp.LoadHeader())
	{
		sceNpTrophy.error("sceNpTrophyRegisterContext(): SCE_NP_TROPHY_ERROR_ILLEGAL_UPDATE");
		return SCE_NP_TROPHY_ERROR_ILLEGAL_UPDATE;
	}

	// Rename or discard certain entries based on the files found
	const size_t kTargetBufferLength = 31;
	char target[kTargetBufferLength + 1];
	target[kTargetBufferLength] = 0;
	strcpy_trunc(target, fmt::format("TROP_%02d.SFM", /*rpcs3::config.system.language.value()*/0));

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
		strcpy_trunc(target, fmt::format("TROP_%02d.SFM", i));
		if (i != /*rpcs3::config.system.language.value()*/0)
		{
			trp.RemoveEntry(target);
		}
	}

	// TODO: Get the path of the current user
	std::string trophyPath = "/dev_hdd0/home/00000001/trophy/" + ctxt->trp_name;
	if (!trp.Install(trophyPath))
	{
		sceNpTrophy.error("sceNpTrophyRegisterContext(): SCE_NP_TROPHY_ERROR_ILLEGAL_UPDATE");
		return SCE_NP_TROPHY_ERROR_ILLEGAL_UPDATE;
	}

	TROPUSRLoader* tropusr = new TROPUSRLoader();
	std::string trophyUsrPath = trophyPath + "/TROPUSR.DAT";
	std::string trophyConfPath = trophyPath + "/TROPCONF.SFM";
	tropusr->Load(trophyUsrPath, trophyConfPath);
	ctxt->tropusr.reset(tropusr);

	// TODO: Callbacks
	if (statusCb(ppu, context, SCE_NP_TROPHY_STATUS_INSTALLED, 100, 100, arg) < 0)
	{
		return SCE_NP_TROPHY_ERROR_PROCESSING_ABORTED;
	}

	if (statusCb(ppu, context, SCE_NP_TROPHY_STATUS_PROCESSING_COMPLETE, 100, 100, arg) < 0)
	{
		return SCE_NP_TROPHY_ERROR_PROCESSING_ABORTED;
	}

	return CELL_OK;
}

error_code sceNpTrophyGetRequiredDiskSpace(u32 context, u32 handle, vm::ptr<u64> reqspace, u64 options)
{
	sceNpTrophy.todo("sceNpTrophyGetRequiredDiskSpace(context=0x%x, handle=0x%x, reqspace=*0x%x, options=0x%llx)", context, handle, reqspace, options);

	if (!reqspace)
	{
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	}

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
	//       The real value can be obtained in TRP.cpp:
	//		 m_headers.trp_file_size - sizeof(m_headers) - (m_headers.trp_files_count * m_headers.trp_element_size);
	// TODO: eventually this should be set to 0 when trophys are detected as already installed, setting to 0 now causes some games to not call registerContext, which leads to trophys never getting installed
	*reqspace = ctxt->trp_stream.size();

	return CELL_OK;
}

error_code sceNpTrophySetSoundLevel(u32 context, u32 handle, u32 level, u64 options)
{
	sceNpTrophy.todo("sceNpTrophySetSoundLevel(context=0x%x, handle=0x%x, level=%d, options=0x%llx)", context, handle, level, options);

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

	return CELL_OK;
}

error_code sceNpTrophyGetGameInfo(u32 context, u32 handle, vm::ptr<SceNpTrophyGameDetails> details, vm::ptr<SceNpTrophyGameData> data)
{
	sceNpTrophy.error("sceNpTrophyGetGameInfo(context=0x%x, handle=0x%x, details=*0x%x, data=*0x%x)", context, handle, details, data);

	if (!details && !data)
	{
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	}

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

	// TODO: Get the path of the current user
	const std::string& path = vfs::get("/dev_hdd0/home/00000001/trophy/" + ctxt->trp_name + "/TROPCONF.SFM");

	// TODO: rXmlDocument can open only real file
	verify(HERE), !fs::get_virtual_device(path);
	rXmlDocument doc;
	doc.Load(path);

	for (std::shared_ptr<rXmlNode> n = doc.GetRoot()->GetChildren(); n; n = n->GetNext())
	{
		if (details)
		{
			if (n->GetName() == "title-name")
			{
				std::string titleName = n->GetNodeContent();
				memcpy(details->title, titleName.c_str(), titleName.size());
			}

			if (n->GetName() == "title-detail")
			{
				std::string titleDetail = n->GetNodeContent();
				memcpy(details->description, titleDetail.c_str(), titleDetail.size());
			}
		}

		if (n->GetName() == "trophy")
		{
			u32 trophy_id = atoi(n->GetAttribute("id").c_str());

			if (details)
			{
				details->numTrophies++;
				switch (n->GetAttribute("ttype")[0]) {
				case 'B': details->numBronze++;   break;
				case 'S': details->numSilver++;   break;
				case 'G': details->numGold++;     break;
				case 'P': details->numPlatinum++; break;
				}
			}

			if (data)
			{
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
	}

	return CELL_OK;
}

error_code sceNpTrophyUnlockTrophy(u32 context, u32 handle, s32 trophyId, vm::ptr<u32> platinumId)
{
	sceNpTrophy.error("sceNpTrophyUnlockTrophy(context=0x%x, handle=0x%x, trophyId=%d, platinumId=*0x%x)", context, handle, trophyId, platinumId);

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

error_code sceNpTrophyGetTrophyUnlockState(u32 context, u32 handle, vm::ptr<SceNpTrophyFlagArray> flags, vm::ptr<u32> count)
{
	sceNpTrophy.error("sceNpTrophyGetTrophyUnlockState(context=0x%x, handle=0x%x, flags=*0x%x, count=*0x%x)", context, handle, flags, count);

	if (!flags || !count)
	{
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	}

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
		sceNpTrophy.error("sceNpTrophyGetTrophyUnlockState: More than 128 trophies detected!");

	// Pack up to 128 bools in u32 flag_bits[4]
	for (u32 id = 0; id < count_; id++)
	{
		if (ctxt->tropusr->GetTrophyUnlockState(id))
			flags->flag_bits[id / 32] |= 1 << (id % 32);
		else
			flags->flag_bits[id / 32] &= ~(1 << (id % 32));
	}

	return CELL_OK;
}

error_code sceNpTrophyGetTrophyInfo(u32 context, u32 handle, s32 trophyId, vm::ptr<SceNpTrophyDetails> details, vm::ptr<SceNpTrophyData> data)
{
	sceNpTrophy.warning("sceNpTrophyGetTrophyInfo(context=0x%x, handle=0x%x, trophyId=%d, details=*0x%x, data=*0x%x)", context, handle, trophyId, details, data);

	if (!details && !data)
	{
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	}

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

	// TODO: Get the path of the current user
	const std::string& path = vfs::get("/dev_hdd0/home/00000001/trophy/" + ctxt->trp_name + "/TROPCONF.SFM");

	// TODO: rXmlDocument can open only real file
	verify(HERE), !fs::get_virtual_device(path);
	rXmlDocument doc;
	doc.Load(path);

	bool found = false;
	for (std::shared_ptr<rXmlNode> n = doc.GetRoot()->GetChildren(); n; n = n->GetNext()) {
		if (n->GetName() == "trophy" && (trophyId == atoi(n->GetAttribute("id").c_str())))
		{
			found = true;

			if (n->GetAttribute("hidden")[0] == 'y' && !ctxt->tropusr->GetTrophyUnlockState(trophyId)) // Trophy is hidden
			{
				return SCE_NP_TROPHY_ERROR_HIDDEN;
			}

			if (details)
			{
				details->trophyId = trophyId;
				switch (n->GetAttribute("ttype")[0]) {
				case 'B': details->trophyGrade = SCE_NP_TROPHY_GRADE_BRONZE;   break;
				case 'S': details->trophyGrade = SCE_NP_TROPHY_GRADE_SILVER;   break;
				case 'G': details->trophyGrade = SCE_NP_TROPHY_GRADE_GOLD;     break;
				case 'P': details->trophyGrade = SCE_NP_TROPHY_GRADE_PLATINUM; break;
				}

				switch (n->GetAttribute("hidden")[0]) {
				case 'y': details->hidden = true;  break;
				case 'n': details->hidden = false; break;
				}

				for (std::shared_ptr<rXmlNode> n2 = n->GetChildren(); n2; n2 = n2->GetNext()) {
					if (n2->GetName() == "name")
					{
						std::string name = n2->GetNodeContent();
						memcpy(details->name, name.c_str(), std::min((size_t)SCE_NP_TROPHY_NAME_MAX_SIZE, name.length() + 1));
					}
					if (n2->GetName() == "detail")
					{
						std::string detail = n2->GetNodeContent();
						memcpy(details->description, detail.c_str(), std::min((size_t)SCE_NP_TROPHY_DESCR_MAX_SIZE, detail.length() + 1));
					}
				}
			}

			if (data)
			{
				data->trophyId = trophyId;
				data->unlocked = ctxt->tropusr->GetTrophyUnlockState(trophyId) != 0; // ???
				data->timestamp = ctxt->tropusr->GetTrophyTimestamp(trophyId);
			}
			break;
		}
	}

	if (!found)
	{
		return not_an_error(SCE_NP_TROPHY_INVALID_TROPHY_ID);
	}

	return CELL_OK;
}

error_code sceNpTrophyGetGameProgress(u32 context, u32 handle, vm::ptr<s32> percentage)
{
	sceNpTrophy.warning("sceNpTrophyGetGameProgress(context=0x%x, handle=0x%x, percentage=*0x%x)", context, handle, percentage);

	if (!percentage)
	{
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	}

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

	double accuratePercentage = 0;
	for (int i = ctxt->tropusr->GetTrophiesCount() - 1; i >= 0; i--)
	{
		if (ctxt->tropusr->GetTrophyUnlockState(i))
		{
			accuratePercentage++;
		}
	}

	*percentage = (s32)(accuratePercentage / ctxt->tropusr->GetTrophiesCount());

	return CELL_OK;
}

error_code sceNpTrophyGetGameIcon(u32 context, u32 handle, vm::ptr<void> buffer, vm::ptr<u32> size)
{
	sceNpTrophy.warning("sceNpTrophyGetGameIcon(context=0x%x, handle=0x%x, buffer=*0x%x, size=*0x%x)", context, handle, buffer, size);

	if (!size)
	{
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	}

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

	const std::string& path = vfs::get("/dev_hdd0/home/00000001/trophy/" + ctxt->trp_name + "/ICON0.PNG");

	if (!fs::exists(path))
	{
		return SCE_NP_TROPHY_ERROR_UNKNOWN_FILE;
	}

	fs::file gameIconFile(path);

	if (buffer && *size >= gameIconFile.size())
	{
		gameIconFile.read(buffer.get_ptr(), gameIconFile.size());
	}

	*size = gameIconFile.size();

	return CELL_OK;
}

error_code sceNpTrophyGetTrophyIcon(u32 context, u32 handle, s32 trophyId, vm::ptr<void> buffer, vm::ptr<u32> size)
{
	sceNpTrophy.warning("sceNpTrophyGetTrophyIcon(context=0x%x, handle=0x%x, trophyId=%d, buffer=*0x%x, size=*0x%x)", context, handle, trophyId, buffer, size);

	if (!size)
	{
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	}

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

	if (ctxt->tropusr->GetTrophiesCount() <= (u32)trophyId)
	{
		return SCE_NP_TROPHY_ERROR_INVALID_TROPHY_ID;
	}

	if (!ctxt->tropusr->GetTrophyUnlockState(trophyId))
	{
		bool hidden = false; // TODO obtain this value
		return hidden ? SCE_NP_TROPHY_ERROR_HIDDEN : SCE_NP_TROPHY_ERROR_LOCKED;
	}

	const std::string& path = vfs::get("/dev_hdd0/home/00000001/trophy/" + ctxt->trp_name + fmt::format("/TROP%03d.PNG", trophyId));

	if (!fs::exists(path))
	{
		return SCE_NP_TROPHY_ERROR_UNKNOWN_FILE;
	}

	fs::file trophyIconFile(path);

	if (buffer && *size >= trophyIconFile.size())
	{
		trophyIconFile.read(buffer.get_ptr(), trophyIconFile.size());
	}

	*size = trophyIconFile.size();

	return CELL_OK;
}


DECLARE(ppu_module_manager::sceNpTrophy)("sceNpTrophy", []()
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
