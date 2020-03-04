#include "stdafx.h"
#include "Emu/VFS.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"

#include "restore_new.h"
#include "Utilities/rXml.h"
#include "define_new_memleakdetect.h"
#include "Loader/TRP.h"
#include "Loader/TROPUSR.h"

#include "sceNp.h"
#include "sceNpTrophy.h"
#include "cellSysutil.h"

#include "Utilities/StrUtil.h"

#include "Emu/Cell/lv2/sys_event.h"
#include "Emu/Cell/lv2/sys_process.h"

#include <cmath>

LOG_CHANNEL(sceNpTrophy);

TrophyNotificationBase::~TrophyNotificationBase()
{
}

struct trophy_context_t
{
	static const u32 id_base = 1;
	static const u32 id_step = 1;
	static const u32 id_count = 4;

	std::string trp_name;
	fs::file trp_stream;
	std::unique_ptr<TROPUSRLoader> tropusr;
};

struct trophy_handle_t
{
	static const u32 id_base = 1;
	static const u32 id_step = 1;
	static const u32 id_count = 4;

	bool is_aborted = false;
};

struct sce_np_trophy_manager
{
	shared_mutex mtx;
	std::atomic<bool> is_initialized = false;

	// Get context + check handle given
	static std::pair<trophy_context_t*, SceNpTrophyError> get_context_ex(u32 context, u32 handle)
	{
		decltype(get_context_ex(0, 0)) res{};
		auto& [ctxt, error] = res;

		if (context < trophy_context_t::id_base ||
			context >= trophy_context_t::id_base + trophy_context_t::id_count)
		{
			// Id was not in range of valid ids
			error = SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
			return res;
		}

		ctxt = idm::check<trophy_context_t>(context);

		if (!ctxt)
		{
			error = SCE_NP_TROPHY_ERROR_UNKNOWN_CONTEXT;
			return res;
		}

		if (handle < trophy_handle_t::id_base ||
			handle >= trophy_handle_t::id_base + trophy_handle_t::id_count)
		{
			error = SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
			return res;
		}

		const auto hndl = idm::check<trophy_handle_t>(handle);

		if (!hndl)
		{
			error = SCE_NP_TROPHY_ERROR_UNKNOWN_HANDLE;
			return res;
		}
		else if (hndl->is_aborted)
		{
			error = SCE_NP_TROPHY_ERROR_ABORT;
			return res;
		}

		return res;
	}
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

// Helpers

static error_code NpTrophyGetTrophyInfo(const trophy_context_t* ctxt, s32 trophyId, SceNpTrophyDetails* details, SceNpTrophyData* data);

static void show_trophy_notification(const trophy_context_t* ctxt, s32 trophyId)
{
	// Get icon for the notification.
	const std::string padded_trophy_id = fmt::format("%03u", trophyId);
	const std::string trophy_icon_path = "/dev_hdd0/home/" + Emu.GetUsr() + "/trophy/" + ctxt->trp_name + "/TROP" + padded_trophy_id + ".PNG";
	fs::file trophy_icon_file = fs::file(vfs::get(trophy_icon_path));
	std::vector<uchar> trophy_icon_data;
	trophy_icon_file.read(trophy_icon_data, trophy_icon_file.size());

	SceNpTrophyDetails details{};

	if (const auto ret = NpTrophyGetTrophyInfo(ctxt, trophyId, &details, nullptr))
	{
		sceNpTrophy.error("Failed to get info for trophy dialog. Error code 0x%x", +ret);
	}

	if (auto trophy_notification_dialog = Emu.GetCallbacks().get_trophy_notification_dialog())
	{
		trophy_notification_dialog->ShowTrophyNotification(details, trophy_icon_data);
	}
}

// Functions

error_code sceNpTrophyInit(vm::ptr<void> pool, u32 poolSize, u32 containerId, u64 options)
{
	sceNpTrophy.warning("sceNpTrophyInit(pool=*0x%x, poolSize=0x%x, containerId=0x%x, options=0x%llx)", pool, poolSize, containerId, options);

	const auto trophy_manager = g_fxo->get<sce_np_trophy_manager>();

	std::scoped_lock lock(trophy_manager->mtx);

	if (trophy_manager->is_initialized)
	{
		return SCE_NP_TROPHY_ERROR_ALREADY_INITIALIZED;
	}

	if (options > 0)
	{
		return SCE_NP_TROPHY_ERROR_NOT_SUPPORTED;
	}

	trophy_manager->is_initialized = true;

	return CELL_OK;
}

error_code sceNpTrophyTerm()
{
	sceNpTrophy.warning("sceNpTrophyTerm()");

	const auto trophy_manager = g_fxo->get<sce_np_trophy_manager>();

	std::scoped_lock lock(trophy_manager->mtx);

	if (!trophy_manager->is_initialized)
	{
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	}

	u32 it = 0;
	u32 ids[std::max(trophy_context_t::id_count, trophy_handle_t::id_count)];

	const auto get_handles = [&](u32 id, trophy_handle_t&)
	{
		ids[it++] = id;
	};

	const auto get_contexts = [&](u32 id, trophy_context_t&)
	{
		ids[it++] = id;
	};

	// This functionality could be implemented in idm instead
	idm::select<trophy_handle_t>(get_handles);

	while (it)
	{
		idm::remove<trophy_handle_t>(ids[--it]);
	}

	it = 0;
	idm::select<trophy_context_t>(get_contexts);

	while (it)
	{
		idm::remove<trophy_context_t>(ids[--it]);
	}

	trophy_manager->is_initialized = false;

	return CELL_OK;
}

error_code sceNpTrophyCreateHandle(vm::ptr<u32> handle)
{
	sceNpTrophy.warning("sceNpTrophyCreateHandle(handle=*0x%x)", handle);

	const auto trophy_manager = g_fxo->get<sce_np_trophy_manager>();

	std::scoped_lock lock(trophy_manager->mtx);

	if (!trophy_manager->is_initialized)
	{
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	}

	if (!handle)
	{
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	}

	const u32 id = idm::make<trophy_handle_t>();

	if (!id)
	{
		return SCE_NP_TROPHY_ERROR_EXCEEDS_MAX;
	}

	*handle = id;
	return CELL_OK;
}

error_code sceNpTrophyDestroyHandle(u32 handle)
{
	sceNpTrophy.warning("sceNpTrophyDestroyHandle(handle=0x%x)", handle);

	const auto trophy_manager = g_fxo->get<sce_np_trophy_manager>();

	std::scoped_lock lock(trophy_manager->mtx);

	// TODO: find out if this is checked
	//if (!trophy_manager->is_initialized)
	//{
	//	return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	//}

	if (handle < trophy_handle_t::id_base ||
		handle >= trophy_handle_t::id_base + trophy_handle_t::id_count)
	{
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	}

	if (!idm::remove<trophy_handle_t>(handle))
	{
		return SCE_NP_TROPHY_ERROR_UNKNOWN_HANDLE;
	}

	return CELL_OK;
}

error_code sceNpTrophyGetGameDetails()
{
	UNIMPLEMENTED_FUNC(sceNpTrophy);
	return CELL_OK;
}

error_code sceNpTrophyAbortHandle(u32 handle)
{
	sceNpTrophy.todo("sceNpTrophyAbortHandle(handle=0x%x)", handle);

	const auto trophy_manager = g_fxo->get<sce_np_trophy_manager>();

	std::scoped_lock lock(trophy_manager->mtx);

	// TODO: find out if this is checked
	//if (!trophy_manager->is_initialized)
	//{
	//	return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	//}

	if (handle < trophy_handle_t::id_base ||
		handle >= trophy_handle_t::id_base + trophy_handle_t::id_count)
	{
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	}

	const auto hndl = idm::check<trophy_handle_t>(handle);

	if (!hndl)
	{
		return SCE_NP_TROPHY_ERROR_UNKNOWN_HANDLE;
	}

	// Once it is aborted it cannot be used anymore
	// TODO: Implement function abortion process maybe? (depends if its actually make sense for some functions)
	hndl->is_aborted = true;
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

	if (!commSign)
	{
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	}

	const auto trophy_manager = g_fxo->get<sce_np_trophy_manager>();

	std::scoped_lock lock(trophy_manager->mtx);

	if (!trophy_manager->is_initialized)
	{
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	}

	if (!context || !commId || !commSign)
	{
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	}

	if (options > SCE_NP_TROPHY_OPTIONS_CREATE_CONTEXT_READ_ONLY)
	{
		return SCE_NP_TROPHY_ERROR_NOT_SUPPORTED;
	}

	// rough checks for further fmt::format call
	if (commId->num > 99)
	{
		return SCE_NP_TROPHY_ERROR_INVALID_NP_COMM_ID;
	}

	// generate trophy context name
	std::string name;
	sceNpTrophy.warning("sceNpTrophyCreateContext term=%s data=%s num=%d", commId->term, commId->data, commId->num);
	if (commId->term)
	{
		char trimchar[10];
		strcpy_trunc(trimchar, commId->data);
		deleteTerminateChar(trimchar, commId->term);
		name = fmt::format("%s_%02d", trimchar, commId->num);
	}
	else
	{
		name = fmt::format("%s_%02d", commId->data, commId->num);
	}

	// open trophy pack file
	fs::file stream(vfs::get(Emu.GetDir() + "TROPDIR/" + name + "/TROPHY.TRP"));

	if (!stream && Emu.GetCat() == "GD")
	{
		stream.open(vfs::get("/dev_bdvd/PS3_GAME/TROPDIR/" + name + "/TROPHY.TRP"));
	}

	// check if exists and opened
	if (!stream)
	{
		return SCE_NP_TROPHY_ERROR_CONF_DOES_NOT_EXIST;
	}

	// create trophy context
	const auto ctxt = idm::make_ptr<trophy_context_t>();

	if (!ctxt)
	{
		return SCE_NP_TROPHY_ERROR_EXCEEDS_MAX;
	}

	// set trophy context parameters (could be passed to constructor through make_ptr call)
	ctxt->trp_name = std::move(name);
	ctxt->trp_stream = std::move(stream);
	*context = idm::last_id();

	return CELL_OK;
}

error_code sceNpTrophyDestroyContext(u32 context)
{
	sceNpTrophy.warning("sceNpTrophyDestroyContext(context=0x%x)", context);

	const auto trophy_manager = g_fxo->get<sce_np_trophy_manager>();

	std::scoped_lock lock(trophy_manager->mtx);

	if (!trophy_manager->is_initialized)
	{
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	}

	if (context < trophy_context_t::id_base	||
		context >= trophy_context_t::id_base + trophy_context_t::id_count)
	{
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	}

	if (!idm::remove<trophy_context_t>(context))
	{
		return SCE_NP_TROPHY_ERROR_UNKNOWN_CONTEXT;
	}

	return CELL_OK;
}

error_code sceNpTrophyRegisterContext(ppu_thread& ppu, u32 context, u32 handle, vm::ptr<SceNpTrophyStatusCallback> statusCb, vm::ptr<void> arg, u64 options)
{
	sceNpTrophy.error("sceNpTrophyRegisterContext(context=0x%x, handle=0x%x, statusCb=*0x%x, arg=*0x%x, options=0x%llx)", context, handle, statusCb, arg, options);

	const auto trophy_manager = g_fxo->get<sce_np_trophy_manager>();

	std::shared_lock lock(trophy_manager->mtx);

	if (!trophy_manager->is_initialized)
	{
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	}

	const auto [ctxt, error] = trophy_manager->get_context_ex(context, handle);

	if (error)
	{
		return error;
	}

	if (!statusCb)
	{
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	}

	TRPLoader trp(ctxt->trp_stream);
	if (!trp.LoadHeader())
	{
		sceNpTrophy.error("sceNpTrophyRegisterContext(): Failed to load trophy config header");
		return SCE_NP_TROPHY_ERROR_ILLEGAL_UPDATE;
	}

	// Rename or discard certain entries based on the files found
	const size_t kTargetBufferLength = 31;
	char target[kTargetBufferLength + 1];
	target[kTargetBufferLength] = 0;
	strcpy_trunc(target, fmt::format("TROP_%02d.SFM", static_cast<s32>(g_cfg.sys.language)));

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
		sceNpTrophy.error("sceNpTrophyRegisterContext(): Invalid/Incomplete trophy config");
		return SCE_NP_TROPHY_ERROR_ILLEGAL_UPDATE;
	}

	// Discard unnecessary TROP_XX.SFM files
	for (s32 i = 0; i <= 18; i++)
	{
		strcpy_trunc(target, fmt::format("TROP_%02d.SFM", i));
		if (i != g_cfg.sys.language)
		{
			trp.RemoveEntry(target);
		}
	}

	const std::string trophyPath = "/dev_hdd0/home/" + Emu.GetUsr() + "/trophy/" + ctxt->trp_name;
	if (!trp.Install(trophyPath))
	{
		sceNpTrophy.error("sceNpTrophyRegisterContext(): Failed to install trophy context '%s' (%s)", trophyPath, fs::g_tls_error);
		return SCE_NP_TROPHY_ERROR_ILLEGAL_UPDATE;
	}

	TROPUSRLoader* tropusr = new TROPUSRLoader();
	const std::string trophyUsrPath = trophyPath + "/TROPUSR.DAT";
	const std::string trophyConfPath = trophyPath + "/TROPCONF.SFM";
	tropusr->Load(trophyUsrPath, trophyConfPath);
	ctxt->tropusr.reset(tropusr);

	// TODO: Callbacks
	// From RE-ing a game's state machine, it seems the possible order is one of the following:
	// * Install (Not installed)  - Setup - Progress * ? - Finalize - Complete - Installed
	// * Reinstall (Corrupted)    - Setup - Progress * ? - Finalize - Complete - Installed
	// * Update (Required update) - Setup - Progress * ? - Finalize - Complete - Installed
	// * Installed
	// We will go with the easy path of Installed, and that's it.

	// The callback is called once and then if it returns >= 0 the cb is called through events(coming from vsh) that are passed to the CB through cellSysutilCheckCallback
	if (statusCb(ppu, context, SCE_NP_TROPHY_STATUS_INSTALLED, 0, 0, arg) < 0)
	{
		return SCE_NP_TROPHY_ERROR_PROCESSING_ABORTED;
	}

	// This emulates vsh sending the events and ensures that not 2 events are processed at once
	const std::pair<u32, s32> statuses[] =
	{
		{ SCE_NP_TROPHY_STATUS_PROCESSING_SETUP, 3 },
		{ SCE_NP_TROPHY_STATUS_PROCESSING_PROGRESS, ::narrow<s32>(tropusr->GetTrophiesCount()) - 1 },
		{ SCE_NP_TROPHY_STATUS_PROCESSING_FINALIZE, 4 },
		{ SCE_NP_TROPHY_STATUS_PROCESSING_COMPLETE, 0 }
	};

	lv2_obj::sleep(ppu);

	// Create a counter which is destroyed after the function ends
	const auto queued = std::make_shared<atomic_t<u32>>(0);
	std::weak_ptr<atomic_t<u32>> wkptr = queued;

	for (auto status : statuses)
	{
		// One status max per cellSysutilCheckCallback call
		*queued += status.second;
		for (s32 completed = 0; completed <= status.second; completed++)
		{
			sysutil_register_cb([statusCb, status, context, completed, arg, wkptr](ppu_thread& cb_ppu) -> s32
			{
				statusCb(cb_ppu, context, status.first, completed, status.second, arg);

				const auto queued = wkptr.lock();
				if (queued && (*queued)-- == 1)
				{
					queued->notify_one();
				}

				return 0;
			});
		}

		u64 current = get_system_time();
		const u64 until = current + 300'000;

		// If too much time passes just send the rest of the events anyway
		for (u32 old_value; current < until && (old_value = *queued);
			current = get_system_time())
		{
			queued->wait(old_value, atomic_wait_timeout{(until - current) * 1000});

			if (ppu.is_stopped())
			{
				return 0;
			}
		}
	}

	return CELL_OK;
}

error_code sceNpTrophyGetRequiredDiskSpace(u32 context, u32 handle, vm::ptr<u64> reqspace, u64 options)
{
	sceNpTrophy.warning("sceNpTrophyGetRequiredDiskSpace(context=0x%x, handle=0x%x, reqspace=*0x%x, options=0x%llx)", context, handle, reqspace, options);

	if (!reqspace)
	{
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	}

	if (options > 0)
	{
		return SCE_NP_TROPHY_ERROR_NOT_SUPPORTED;
	}

	const auto trophy_manager = g_fxo->get<sce_np_trophy_manager>();

	std::shared_lock lock(trophy_manager->mtx);

	if (!trophy_manager->is_initialized)
	{
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	}

	const auto [ctxt, error] = trophy_manager->get_context_ex(context, handle);

	if (error)
	{
		return error;
	}

	u64 space = 0;

	if (!fs::is_dir(vfs::get("/dev_hdd0/home/" + Emu.GetUsr() + "/trophy/" + ctxt->trp_name)))
	{
		TRPLoader trp(ctxt->trp_stream);

		if (trp.LoadHeader())
		{
			space = trp.GetRequiredSpace();
		}
		else
		{
			sceNpTrophy.error("sceNpTrophyGetRequiredDiskSpace(): Failed to load trophy header! (trp_name=%s)", ctxt->trp_name);
		}
	}
	else
	{
		sceNpTrophy.warning("sceNpTrophyGetRequiredDiskSpace(): Trophy config is already installed (trp_name=%s)", ctxt->trp_name);
	}

	sceNpTrophy.warning("sceNpTrophyGetRequiredDiskSpace(): reqspace is 0x%llx", space);

	*reqspace = space;
	return CELL_OK;
}

error_code sceNpTrophySetSoundLevel(u32 context, u32 handle, u32 level, u64 options)
{
	sceNpTrophy.todo("sceNpTrophySetSoundLevel(context=0x%x, handle=0x%x, level=%d, options=0x%llx)", context, handle, level, options);

	if (level > 100 || level < 20)
	{
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	}

	if (options > 0)
	{
		return SCE_NP_TROPHY_ERROR_NOT_SUPPORTED;
	}

	const auto trophy_manager = g_fxo->get<sce_np_trophy_manager>();

	std::shared_lock lock(trophy_manager->mtx);

	if (!trophy_manager->is_initialized)
	{
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	}

	const auto [ctxt, error] = trophy_manager->get_context_ex(context, handle);

	if (error)
	{
		return error;
	}

	return CELL_OK;
}

error_code sceNpTrophyGetGameInfo(u32 context, u32 handle, vm::ptr<SceNpTrophyGameDetails> details, vm::ptr<SceNpTrophyGameData> data)
{
	sceNpTrophy.error("sceNpTrophyGetGameInfo(context=0x%x, handle=0x%x, details=*0x%x, data=*0x%x)", context, handle, details, data);

	const auto trophy_manager = g_fxo->get<sce_np_trophy_manager>();

	std::shared_lock lock(trophy_manager->mtx);

	if (!trophy_manager->is_initialized)
	{
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	}

	const auto [ctxt, error] = trophy_manager->get_context_ex(context, handle);

	if (error)
	{
		return error;
	}

	if (!ctxt->tropusr)
	{
		// TODO: May return SCE_NP_TROPHY_ERROR_UNKNOWN_TITLE for older sdk version
		return SCE_NP_TROPHY_ERROR_CONTEXT_NOT_REGISTERED;
	}

	if (!details && !data)
	{
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	}

	fs::file config(vfs::get("/dev_hdd0/home/" + Emu.GetUsr() + "/trophy/" + ctxt->trp_name + "/TROPCONF.SFM"));

	if (!config)
	{
		return SCE_NP_TROPHY_ERROR_CONF_DOES_NOT_EXIST;
	}

	rXmlDocument doc;
	doc.Read(config.to_string());

	auto trophy_base = doc.GetRoot();
	if (trophy_base->GetChildren()->GetName() == "trophyconf")
	{
		trophy_base = trophy_base->GetChildren();
	}

	if (details)
		*details = {};
	if (data)
		*data = {};

	for (std::shared_ptr<rXmlNode> n = trophy_base->GetChildren(); n; n = n->GetNext())
	{
		const std::string n_name = n->GetName();

		if (details)
		{
			if (n_name == "title-name")
			{
				strcpy_trunc(details->title, n->GetNodeContent());
				continue;
			}
			else if (n_name == "title-detail")
			{
				strcpy_trunc(details->description, n->GetNodeContent());
				continue;
			}
		}

		if (n_name == "trophy")
		{
			if (details)
			{
				details->numTrophies++;
				switch (n->GetAttribute("ttype")[0])
				{
				case 'B': details->numBronze++;   break;
				case 'S': details->numSilver++;   break;
				case 'G': details->numGold++;     break;
				case 'P': details->numPlatinum++; break;
				}
			}

			if (data)
			{
				const u32 trophy_id = atoi(n->GetAttribute("id").c_str());

				if (ctxt->tropusr->GetTrophyUnlockState(trophy_id))
				{
					data->unlockedTrophies++;
					switch (n->GetAttribute("ttype")[0])
					{
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

error_code sceNpTrophyGetLatestTrophies()
{
	UNIMPLEMENTED_FUNC(sceNpTrophy);
	return CELL_OK;
}

error_code sceNpTrophyUnlockTrophy(u32 context, u32 handle, s32 trophyId, vm::ptr<u32> platinumId)
{
	sceNpTrophy.error("sceNpTrophyUnlockTrophy(context=0x%x, handle=0x%x, trophyId=%d, platinumId=*0x%x)", context, handle, trophyId, platinumId);

	const auto trophy_manager = g_fxo->get<sce_np_trophy_manager>();

	std::shared_lock lock(trophy_manager->mtx);

	if (!trophy_manager->is_initialized)
	{
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	}

	const auto [ctxt, error] = trophy_manager->get_context_ex(context, handle);

	if (error)
	{
		return error;
	}

	if (!ctxt->tropusr)
	{
		// TODO: May return SCE_NP_TROPHY_ERROR_UNKNOWN_TITLE for older sdk version
		return SCE_NP_TROPHY_ERROR_CONTEXT_NOT_REGISTERED;
	}

	if (trophyId < 0 || trophyId >= static_cast<s32>(ctxt->tropusr->GetTrophiesCount()))
	{
		return SCE_NP_TROPHY_ERROR_INVALID_TROPHY_ID;
	}

	if (ctxt->tropusr->GetTrophyGrade(trophyId) == SCE_NP_TROPHY_GRADE_PLATINUM)
	{
		return SCE_NP_TROPHY_ERROR_CANNOT_UNLOCK_PLATINUM;
	}

	if (ctxt->tropusr->GetTrophyUnlockState(trophyId))
	{
		return SCE_NP_TROPHY_ERROR_ALREADY_UNLOCKED;
	}

	ctxt->tropusr->UnlockTrophy(trophyId, 0, 0); // TODO: add timestamps

	// TODO: Make sure that unlocking platinum trophies is properly implemented and improve upon it
	const std::string& config_path = vfs::get("/dev_hdd0/home/" + Emu.GetUsr() + "/trophy/" + ctxt->trp_name + "/TROPCONF.SFM");
	const u32 unlocked_platinum_id = ctxt->tropusr->GetUnlockedPlatinumID(trophyId, config_path);

	if (unlocked_platinum_id != SCE_NP_TROPHY_INVALID_TROPHY_ID)
	{
		sceNpTrophy.warning("sceNpTrophyUnlockTrophy: All requirements for unlocking the platinum trophy (ID = %d) were met.)", unlocked_platinum_id);

		if (ctxt->tropusr->UnlockTrophy(unlocked_platinum_id, 0, 0)) // TODO: add timestamps
		{
			sceNpTrophy.success("You unlocked a platinum trophy! Hooray!!!");
		}
	}

	if (platinumId)
	{
		*platinumId = unlocked_platinum_id;
		sceNpTrophy.warning("sceNpTrophyUnlockTrophy: platinumId was set to %d", unlocked_platinum_id);
	}

	const std::string trophyPath = "/dev_hdd0/home/" + Emu.GetUsr() + "/trophy/" + ctxt->trp_name + "/TROPUSR.DAT";
	ctxt->tropusr->Save(trophyPath);

	if (g_cfg.misc.show_trophy_popups)
	{
		// Enqueue popup for the regular trophy
		show_trophy_notification(ctxt, trophyId);

		if (unlocked_platinum_id != SCE_NP_TROPHY_INVALID_TROPHY_ID)
		{
			// Enqueue popup for the holy platinum trophy
			show_trophy_notification(ctxt, unlocked_platinum_id);
		}
	}

	return CELL_OK;
}

error_code sceNpTrophyGetTrophyUnlockState(u32 context, u32 handle, vm::ptr<SceNpTrophyFlagArray> flags, vm::ptr<u32> count)
{
	sceNpTrophy.error("sceNpTrophyGetTrophyUnlockState(context=0x%x, handle=0x%x, flags=*0x%x, count=*0x%x)", context, handle, flags, count);

	if (!flags || !count) // is count really checked here?
	{
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	}

	const auto trophy_manager = g_fxo->get<sce_np_trophy_manager>();

	std::shared_lock lock(trophy_manager->mtx);

	if (!trophy_manager->is_initialized)
	{
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	}

	const auto [ctxt, error] = trophy_manager->get_context_ex(context, handle);

	if (error)
	{
		return error;
	}

	if (!ctxt->tropusr)
	{
		// TODO: May return SCE_NP_TROPHY_ERROR_UNKNOWN_TITLE for older sdk version
		return SCE_NP_TROPHY_ERROR_CONTEXT_NOT_REGISTERED;
	}

	const u32 count_ = ctxt->tropusr->GetTrophiesCount();
	*count = count_;
	if (count_ > 128)
		sceNpTrophy.error("sceNpTrophyGetTrophyUnlockState: More than 128 trophies detected!");

	// Needs hw testing
	*flags = {};

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

error_code sceNpTrophyGetTrophyDetails()
{
	UNIMPLEMENTED_FUNC(sceNpTrophy);
	return CELL_OK;
}

static error_code NpTrophyGetTrophyInfo(const trophy_context_t* ctxt, s32 trophyId, SceNpTrophyDetails* details, SceNpTrophyData* data)
{
	if (!details && !data)
	{
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	}

	if (!ctxt->tropusr)
	{
		// TODO: May return SCE_NP_TROPHY_ERROR_UNKNOWN_TITLE for older sdk version
		return SCE_NP_TROPHY_ERROR_CONTEXT_NOT_REGISTERED;
	}

	fs::file config(vfs::get("/dev_hdd0/home/" + Emu.GetUsr() + "/trophy/" + ctxt->trp_name + "/TROPCONF.SFM"));

	if (!config)
	{
		return SCE_NP_TROPHY_ERROR_CONF_DOES_NOT_EXIST;
	}

	SceNpTrophyDetails tmp_details{};
	SceNpTrophyData tmp_data{};

	rXmlDocument doc;
	doc.Read(config.to_string());

	auto trophy_base = doc.GetRoot();
	if (trophy_base->GetChildren()->GetName() == "trophyconf")
	{
		trophy_base = trophy_base->GetChildren();
	}

	bool found = false;
	for (std::shared_ptr<rXmlNode> n = trophy_base->GetChildren(); n; n = n->GetNext())
	{
		if (n->GetName() == "trophy" && (trophyId == atoi(n->GetAttribute("id").c_str())))
		{
			found = true;

			const bool hidden = n->GetAttribute("hidden")[0] == 'y';
			const bool unlocked = !!ctxt->tropusr->GetTrophyUnlockState(trophyId);

			if (hidden && !unlocked) // Trophy is hidden
			{
				return SCE_NP_TROPHY_ERROR_HIDDEN;
			}

			if (details)
			{
				tmp_details.trophyId = trophyId;
				tmp_details.hidden = hidden;

				switch (n->GetAttribute("ttype")[0])
				{
				case 'B': tmp_details.trophyGrade = SCE_NP_TROPHY_GRADE_BRONZE;   break;
				case 'S': tmp_details.trophyGrade = SCE_NP_TROPHY_GRADE_SILVER;   break;
				case 'G': tmp_details.trophyGrade = SCE_NP_TROPHY_GRADE_GOLD;     break;
				case 'P': tmp_details.trophyGrade = SCE_NP_TROPHY_GRADE_PLATINUM; break;
				}

				for (std::shared_ptr<rXmlNode> n2 = n->GetChildren(); n2; n2 = n2->GetNext())
				{
					const std::string n2_name = n2->GetName();

					if (n2_name == "name")
					{
						strcpy_trunc(tmp_details.name, n2->GetNodeContent());
					}
					else if (n2_name == "detail")
					{
						strcpy_trunc(tmp_details.description, n2->GetNodeContent());
					}
				}
			}

			if (data)
			{
				tmp_data.trophyId = trophyId;
				tmp_data.unlocked = unlocked;
				tmp_data.timestamp = ctxt->tropusr->GetTrophyTimestamp(trophyId);
			}

			break;
		}
	}

	if (!found)
	{
		return SCE_NP_TROPHY_ERROR_INVALID_TROPHY_ID;
	}

	if (details)
	{
		*details = tmp_details;
	}

	if (data)
	{
		*data = tmp_data;
	}

	return CELL_OK;
}

error_code sceNpTrophyGetTrophyInfo(u32 context, u32 handle, s32 trophyId, vm::ptr<SceNpTrophyDetails> details, vm::ptr<SceNpTrophyData> data)
{
	sceNpTrophy.warning("sceNpTrophyGetTrophyInfo(context=0x%x, handle=0x%x, trophyId=%d, details=*0x%x, data=*0x%x)", context, handle, trophyId, details, data);

	if (trophyId < 0 || trophyId > 127) // max 128 trophies
	{
		return SCE_NP_TROPHY_ERROR_INVALID_TROPHY_ID;
	}

	const auto trophy_manager = g_fxo->get<sce_np_trophy_manager>();

	std::shared_lock lock(trophy_manager->mtx);

	if (!trophy_manager->is_initialized)
	{
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	}

	const auto [ctxt, error] = trophy_manager->get_context_ex(context, handle);

	if (error)
	{
		return error;
	}

	return NpTrophyGetTrophyInfo(ctxt, trophyId, details ? details.get_ptr() : nullptr, data ? data.get_ptr() : nullptr);
}

error_code sceNpTrophyGetGameProgress(u32 context, u32 handle, vm::ptr<s32> percentage)
{
	sceNpTrophy.warning("sceNpTrophyGetGameProgress(context=0x%x, handle=0x%x, percentage=*0x%x)", context, handle, percentage);

	if (!percentage)
	{
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	}

	const auto trophy_manager = g_fxo->get<sce_np_trophy_manager>();

	std::shared_lock lock(trophy_manager->mtx);

	if (!trophy_manager->is_initialized)
	{
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	}

	const auto [ctxt, error] = trophy_manager->get_context_ex(context, handle);

	if (error)
	{
		return error;
	}

	if (!ctxt->tropusr)
	{
		// TODO: May return SCE_NP_TROPHY_ERROR_UNKNOWN_TITLE for older sdk version
		return SCE_NP_TROPHY_ERROR_CONTEXT_NOT_REGISTERED;
	}

	const u32 unlocked = ctxt->tropusr->GetUnlockedTrophiesCount();
	const u32 trp_count = ctxt->tropusr->GetTrophiesCount();

	// Round result to nearest (TODO: Check 0 trophies)
	*percentage = trp_count ? ::rounded_div(unlocked * 100, trp_count) : 0;

	if (trp_count == 0 || trp_count > 128)
	{
		sceNpTrophy.warning("sceNpTrophyGetGameProgress(): Trophies count may be invalid or untested (%d)", trp_count);
	}

	return CELL_OK;
}

error_code sceNpTrophyGetGameIcon(u32 context, u32 handle, vm::ptr<void> buffer, vm::ptr<u32> size)
{
	sceNpTrophy.warning("sceNpTrophyGetGameIcon(context=0x%x, handle=0x%x, buffer=*0x%x, size=*0x%x)", context, handle, buffer, size);

	const auto trophy_manager = g_fxo->get<sce_np_trophy_manager>();

	std::shared_lock lock(trophy_manager->mtx);

	if (!trophy_manager->is_initialized)
	{
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	}

	const auto [ctxt, error] = trophy_manager->get_context_ex(context, handle);

	if (error)
	{
		return error;
	}

	if (!size)
	{
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	}

	fs::file icon_file(vfs::get("/dev_hdd0/home/" + Emu.GetUsr() + "/trophy/" + ctxt->trp_name + "/ICON0.PNG"));

	if (!icon_file)
	{
		return SCE_NP_TROPHY_ERROR_UNKNOWN_FILE;
	}

	const u32 icon_size = ::size32(icon_file);

	if (buffer && *size >= icon_size)
	{
		icon_file.read(buffer.get_ptr(), icon_size);
	}

	*size = icon_size;

	return CELL_OK;
}

error_code sceNpTrophyGetUserInfo()
{
	UNIMPLEMENTED_FUNC(sceNpTrophy);
	return CELL_OK;
}

error_code sceNpTrophyGetTrophyIcon(u32 context, u32 handle, s32 trophyId, vm::ptr<void> buffer, vm::ptr<u32> size)
{
	sceNpTrophy.warning("sceNpTrophyGetTrophyIcon(context=0x%x, handle=0x%x, trophyId=%d, buffer=*0x%x, size=*0x%x)", context, handle, trophyId, buffer, size);

	const auto trophy_manager = g_fxo->get<sce_np_trophy_manager>();

	std::shared_lock lock(trophy_manager->mtx);

	if (!trophy_manager->is_initialized)
	{
		return SCE_NP_TROPHY_ERROR_NOT_INITIALIZED;
	}

	const auto [ctxt, error] = trophy_manager->get_context_ex(context, handle);

	if (error)
	{
		return error;
	}

	if (!size)
	{
		return SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT;
	}

	if (!ctxt->tropusr)
	{
		// TODO: May return SCE_NP_TROPHY_ERROR_UNKNOWN_TITLE for older sdk version
		return SCE_NP_TROPHY_ERROR_CONTEXT_NOT_REGISTERED;
	}

	if (ctxt->tropusr->GetTrophiesCount() <= static_cast<u32>(trophyId))
	{
		return SCE_NP_TROPHY_ERROR_INVALID_TROPHY_ID;
	}

	if (!ctxt->tropusr->GetTrophyUnlockState(trophyId))
	{
		bool hidden = false; // TODO obtain this value
		return hidden ? SCE_NP_TROPHY_ERROR_HIDDEN : SCE_NP_TROPHY_ERROR_LOCKED;
	}

	fs::file icon_file(vfs::get("/dev_hdd0/home/" + Emu.GetUsr() + "/trophy/" + ctxt->trp_name + fmt::format("/TROP%03d.PNG", trophyId)));

	if (!icon_file)
	{
		return SCE_NP_TROPHY_ERROR_UNKNOWN_FILE;
	}

	const u32 icon_size = ::size32(icon_file);

	if (buffer && *size >= icon_size)
	{
		icon_file.read(buffer.get_ptr(), icon_size);
	}

	*size = icon_size;

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
	REG_FUNC(sceNpTrophy, sceNpTrophyGetGameDetails);
	REG_FUNC(sceNpTrophy, sceNpTrophyUnlockTrophy);
	REG_FUNC(sceNpTrophy, sceNpTrophyGetLatestTrophies);
	REG_FUNC(sceNpTrophy, sceNpTrophyTerm);
	REG_FUNC(sceNpTrophy, sceNpTrophyGetTrophyUnlockState);
	REG_FUNC(sceNpTrophy, sceNpTrophyGetUserInfo);
	REG_FUNC(sceNpTrophy, sceNpTrophyGetTrophyIcon);
	REG_FUNC(sceNpTrophy, sceNpTrophyCreateContext);
	REG_FUNC(sceNpTrophy, sceNpTrophyGetTrophyDetails);
	REG_FUNC(sceNpTrophy, sceNpTrophyGetTrophyInfo);
	REG_FUNC(sceNpTrophy, sceNpTrophyGetGameIcon);
});
