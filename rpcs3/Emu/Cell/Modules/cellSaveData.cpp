﻿#include "stdafx.h"
#include "Emu/VFS.h"
#include "Emu/Cell/lv2/sys_fs.h"
#include "Emu/Cell/lv2/sys_sync.h"
#include "Emu/Cell/lv2/sys_process.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/Modules/cellSysutil.h"
#include "Emu/Cell/Modules/cellUserInfo.h"

#include "cellSaveData.h"
#include "cellMsgDialog.h"

#include "Loader/PSF.h"
#include "Utilities/StrUtil.h"
#include "Utilities/span.h"

#include <thread>
#include <mutex>
#include <algorithm>

LOG_CHANNEL(cellSaveData);

template<>
void fmt_class_string<CellSaveDataError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_SAVEDATA_ERROR_CBRESULT);
			STR_CASE(CELL_SAVEDATA_ERROR_ACCESS_ERROR);
			STR_CASE(CELL_SAVEDATA_ERROR_INTERNAL);
			STR_CASE(CELL_SAVEDATA_ERROR_PARAM);
			STR_CASE(CELL_SAVEDATA_ERROR_NOSPACE);
			STR_CASE(CELL_SAVEDATA_ERROR_BROKEN);
			STR_CASE(CELL_SAVEDATA_ERROR_FAILURE);
			STR_CASE(CELL_SAVEDATA_ERROR_BUSY);
			STR_CASE(CELL_SAVEDATA_ERROR_NOUSER);
			STR_CASE(CELL_SAVEDATA_ERROR_SIZEOVER);
			STR_CASE(CELL_SAVEDATA_ERROR_NODATA);
			STR_CASE(CELL_SAVEDATA_ERROR_NOTSUPPORTED);
		}

		return unknown;
	});
}

SaveDialogBase::~SaveDialogBase()
{
}

// cellSaveData aliases (only for cellSaveData.cpp)
using PSetList = vm::ptr<CellSaveDataSetList>;
using PSetBuf = vm::ptr<CellSaveDataSetBuf>;
using PFuncFixed = vm::ptr<CellSaveDataFixedCallback>;
using PFuncList = vm::ptr<CellSaveDataListCallback>;
using PFuncStat = vm::ptr<CellSaveDataStatCallback>;
using PFuncFile = vm::ptr<CellSaveDataFileCallback>;
using PFuncDone = vm::ptr<CellSaveDataDoneCallback>;

enum : u32
{
	SAVEDATA_OP_AUTO_SAVE      = 0,
	SAVEDATA_OP_AUTO_LOAD      = 1,
	SAVEDATA_OP_LIST_AUTO_SAVE = 2,
	SAVEDATA_OP_LIST_AUTO_LOAD = 3,
	SAVEDATA_OP_LIST_SAVE      = 4,
	SAVEDATA_OP_LIST_LOAD      = 5,
	SAVEDATA_OP_FIXED_SAVE     = 6,
	SAVEDATA_OP_FIXED_LOAD     = 7,

	SAVEDATA_OP_LIST_DELETE    = 13,
	SAVEDATA_OP_FIXED_DELETE   = 14,
};

namespace
{
	struct savedata_context
	{
		alignas(16) CellSaveDataCBResult result;
		alignas(16) CellSaveDataListGet  listGet;
		alignas(16) CellSaveDataListSet  listSet;
		alignas(16) CellSaveDataFixedSet fixedSet;
		alignas(16) CellSaveDataStatGet  statGet;
		alignas(16) CellSaveDataStatSet  statSet;
		alignas(16) CellSaveDataFileGet  fileGet;
		alignas(16) CellSaveDataFileSet  fileSet;
		alignas(16) CellSaveDataDoneGet  doneGet;
	};
}

vm::gvar<savedata_context> g_savedata_context;

struct savedata_mutex
{
	semaphore<> mutex;
};

static std::vector<SaveDataEntry> get_save_entries(const std::string& base_dir, const std::string& prefix)
{
	std::vector<SaveDataEntry> save_entries;

	if (base_dir.empty() || prefix.empty())
	{
		return save_entries;
	}

	// get the saves matching the supplied prefix
	for (auto&& entry : fs::dir(base_dir))
	{
		if (!entry.is_directory || sysutil_check_name_string(entry.name.c_str(), 1, CELL_SAVEDATA_DIRNAME_SIZE) != 0)
		{
			continue;
		}

		if (!entry.name.starts_with(prefix))
		{
			continue;
		}

		// PSF parameters
		const psf::registry psf = psf::load_object(fs::file(base_dir + entry.name + "/PARAM.SFO"));

		if (psf.empty())
		{
			continue;
		}

		SaveDataEntry save_entry;
		save_entry.dirName   = psf.at("SAVEDATA_DIRECTORY").as_string();
		save_entry.listParam = psf.at("SAVEDATA_LIST_PARAM").as_string();
		save_entry.title     = psf.at("TITLE").as_string();
		save_entry.subtitle  = psf.at("SUB_TITLE").as_string();
		save_entry.details   = psf.at("DETAIL").as_string();

		for (const auto& entry2 : fs::dir(base_dir + entry.name))
		{
			save_entry.size += entry2.size;
		}

		save_entry.atime = entry.atime;
		save_entry.mtime = entry.mtime;
		save_entry.ctime = entry.ctime;
		if (fs::file icon{base_dir + entry.name + "/ICON0.PNG"})
			save_entry.iconBuf = icon.to_vector<uchar>();
		save_entry.isNew = false;

		save_entry.escaped = std::move(entry.name);
		save_entries.emplace_back(save_entry);
	}

	return save_entries;
}

static error_code select_and_delete(ppu_thread& ppu)
{
	std::unique_lock lock(g_fxo->get<savedata_mutex>()->mutex, std::try_to_lock);

	if (!lock)
	{
		return CELL_SAVEDATA_ERROR_BUSY;
	}

	const std::string base_dir = vfs::get(fmt::format("/dev_hdd0/home/%08u/savedata/", Emu.GetUsrId()));

	auto save_entries = get_save_entries(base_dir, Emu.GetTitleID());

	s32 selected = -1;
	s32 focused  = -1;

	while (true)
	{
		// Yield before a blocking dialog is being spawned
		lv2_obj::sleep(ppu);

		// Display a blocking Save Data List asynchronously in the GUI thread.
		if (auto save_dialog = Emu.GetCallbacks().get_save_dialog())
		{
			selected = save_dialog->ShowSaveDataList(save_entries, focused, SAVEDATA_OP_LIST_DELETE, vm::null);
		}

		// Reschedule after a blocking dialog returns
		if (ppu.check_state())
		{
			return 0;
		}

		// Abort if dialog was canceled or selection is invalid in this context
		if (selected < 0)
		{
			return CELL_CANCEL;
		}

		// Set focused entry for the next iteration
		focused = save_entries.empty() ? -1 : selected;

		// Get information from the selected entry
		SaveDataEntry entry    = save_entries[selected];
		const std::string info = entry.title + "\n" + entry.subtitle + "\n" + entry.details;

		// Reusable display message string
		std::string msg = "Do you really want to delete this entry?\n\n" + info;

		// Yield before a blocking dialog is being spawned
		lv2_obj::sleep(ppu);

		// Get user confirmation by opening a blocking dialog
		error_code res  = open_msg_dialog(true, CELL_MSGDIALOG_TYPE_SE_TYPE_NORMAL | CELL_MSGDIALOG_TYPE_BUTTON_TYPE_YESNO, vm::make_str(msg));

		// Reschedule after a blocking dialog returns
		if (ppu.check_state())
		{
			return 0;
		}

		if (res != CELL_OK)
		{
			return CELL_SAVEDATA_ERROR_INTERNAL;
		}

		if (g_last_user_response.load() == CELL_MSGDIALOG_BUTTON_YES)
		{
			// Remove directory
			const std::string path = base_dir + save_entries[selected].escaped;
			fs::remove_all(path);

			// Remove entry from the list and reset the selection
			save_entries.erase(save_entries.cbegin() + selected);
			selected = -1;

			// Reset the focused index if the new list is empty
			if (save_entries.empty())
			{
				focused = -1;
			}

			// Update display message
			msg = "Successfully removed entry!\n\n" + info;
			cellSaveData.success("%s", msg);

			// Yield before blocking dialog is being spawned
			lv2_obj::sleep(ppu);

			// Display success message by opening a blocking dialog (return value should be irrelevant here)
			res = open_msg_dialog(true, CELL_MSGDIALOG_TYPE_SE_TYPE_NORMAL | CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK, vm::make_str(msg));

			// Reschedule after blocking dialog returns
			if (ppu.check_state())
			{
				return 0;
			}
		}
	}

	return CELL_CANCEL;
}

// Displays a CellSaveDataCBResult error message.
static error_code display_callback_result_error_message(ppu_thread& ppu, const CellSaveDataCBResult& result, u32 errDialog)
{
	std::string msg;
	bool use_invalid_message = false;

	switch (result.result)
	{
	case CELL_SAVEDATA_CBRESULT_ERR_NOSPACE:
		msg = fmt::format("Error - Insufficient free space\n\nSpace needed: %d KB", result.errNeedSizeKB);
		break;
	case CELL_SAVEDATA_CBRESULT_ERR_FAILURE:
		msg = "Error - Failed to save or load";
		break;
	case CELL_SAVEDATA_CBRESULT_ERR_BROKEN:
		msg = "Error - Save data corrupted";
		break;
	case CELL_SAVEDATA_CBRESULT_ERR_NODATA:
		msg = "Error - Save data cannot be found";
		break;
	case CELL_SAVEDATA_CBRESULT_ERR_INVALID:
		if (result.invalidMsg)
			use_invalid_message = true;
		break;
	default:
		// ****** sysutil savedata parameter error : 22 ******
		return {CELL_SAVEDATA_ERROR_PARAM, "22"};
	}

	// TODO: errDialog == CELL_SAVEDATA_ERRDIALOG_NOREPEAT
	if (errDialog != CELL_SAVEDATA_ERRDIALOG_ALWAYS)
		return CELL_SAVEDATA_ERROR_CBRESULT;

	// Yield before a blocking dialog is being spawned
	lv2_obj::sleep(ppu);

	// Get user confirmation by opening a blocking dialog (return value should be irrelevant here)
	error_code res = open_msg_dialog(true, CELL_MSGDIALOG_TYPE_SE_TYPE_NORMAL | CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK, use_invalid_message ? result.invalidMsg : vm::make_str(msg));

	// Reschedule after a blocking dialog returns
	if (ppu.check_state())
	{
		return 0;
	}

	return CELL_SAVEDATA_ERROR_CBRESULT;
}

static std::string get_confirmation_message(u32 operation)
{
	if (operation == SAVEDATA_OP_LIST_DELETE || operation == SAVEDATA_OP_FIXED_DELETE)
	{
		return "Delete this entry?";
	}
	else if (operation == SAVEDATA_OP_LIST_LOAD || operation == SAVEDATA_OP_FIXED_LOAD)
	{
		return "Load this entry?";
	}
	else if (operation == SAVEDATA_OP_LIST_SAVE || operation == SAVEDATA_OP_FIXED_SAVE)
	{
		return "Overwrite this entry?";
	}

	return "";
}

static s32 savedata_check_args(u32 operation, u32 version, vm::cptr<char> dirName,
	u32 errDialog, PSetList setList, PSetBuf setBuf, PFuncList funcList, PFuncFixed funcFixed, PFuncStat funcStat,
	PFuncFile funcFile, u32 container, u32 unk_op_flags, vm::ptr<void> userdata, u32 userId, PFuncDone funcDone)
{
	if (version > CELL_SAVEDATA_VERSION_420)
	{
		// ****** sysutil savedata parameter error : 1 ******
		return 1;
	}

	if (errDialog > CELL_SAVEDATA_ERRDIALOG_NOREPEAT)
	{
		// ****** sysutil savedata parameter error : 5 ******
		return 5;
	}

	if (operation <= SAVEDATA_OP_AUTO_LOAD)
	{
		if (!dirName)
		{
			// ****** sysutil savedata parameter error : 2 ******
			return 2;
		}

		switch (sysutil_check_name_string(dirName.get_ptr(), 1, CELL_SAVEDATA_DIRNAME_SIZE))
		{
		case -1:
		{
			// ****** sysutil savedata parameter error : 3 ******
			return 3;
		}
		case -2:
		{
			// ****** sysutil savedata parameter error : 4 ******
			return 4;
		}
		case 0: break;
		default: ASSUME(0);
		}
	}

	if ((operation >= SAVEDATA_OP_LIST_AUTO_SAVE && operation <= SAVEDATA_OP_FIXED_LOAD) || operation == SAVEDATA_OP_FIXED_DELETE)
	{
		if (!setList)
		{
			// ****** sysutil savedata parameter error : 11 ******
			return 11;
		}

		if (setList->sortType > CELL_SAVEDATA_SORTTYPE_SUBTITLE)
		{
			// ****** sysutil savedata parameter error : 12 ******
			return 12;
		}

		if (setList->sortOrder > CELL_SAVEDATA_SORTORDER_ASCENT)
		{
			// ****** sysutil savedata parameter error : 13 ******
			return 13;
		}

		if (!setList->dirNamePrefix)
		{
			// ****** sysutil savedata parameter error : 15 ******
			return 15;
		}

		if (!memchr(setList->dirNamePrefix.get_ptr(), '\0', CELL_SAVEDATA_PREFIX_SIZE)
			|| (g_ps3_process_info.sdk_ver > 0x3FFFFF && !setList->dirNamePrefix[0]))
		{
			// ****** sysutil savedata parameter error : 17 ******
			return 17;
		}

		char cur, buf[CELL_SAVEDATA_DIRNAME_SIZE + 1]{};

		for (s32 pos = 0, posprefix = 0; cur = setList->dirNamePrefix[pos++], true;)
		{
			if (cur == '\0' || cur == '|')
			{
				// Check prefix if not empty
				if (posprefix)
				{
					switch (sysutil_check_name_string(buf, 1, CELL_SAVEDATA_DIRNAME_SIZE))
					{
					case -1:
					{
						// ****** sysutil savedata parameter error : 16 ******
						return 16;
					}
					case -2:
					{
						// ****** sysutil savedata parameter error : 17 ******
						return 17;
					}
					case 0: break;
					default: ASSUME(0);
					}
				}

				if (cur == '\0')
				{
					break;
				}

				// Note: no need to reset buffer, only position
				posprefix = 0;
				continue;
			}

			if (posprefix == CELL_SAVEDATA_DIRNAME_SIZE)
			{
				// ****** sysutil savedata parameter error : 17 ******
				return 17;
			}

			buf[posprefix++] = cur;
		}

		if (setList->reserved)
		{
			// ****** sysutil savedata parameter error : 14 ******
			return 14;
		}
	}

	if (!setBuf)
	{
		// ****** sysutil savedata parameter error : 74 ******
		return 74;
	}

	if ((operation >= SAVEDATA_OP_LIST_AUTO_SAVE && operation <= SAVEDATA_OP_FIXED_LOAD) || operation == SAVEDATA_OP_FIXED_DELETE)
	{
		if (setBuf->dirListMax > CELL_SAVEDATA_DIRLIST_MAX)
		{
			// ****** sysutil savedata parameter error : 8 ******
			return 8;
		}

		CHECK_SIZE(CellSaveDataDirList, 48);

		if (setBuf->dirListMax * sizeof(CellSaveDataDirList) > setBuf->bufSize)
		{
			// ****** sysutil savedata parameter error : 7 ******
			return 7;
		}
	}

	CHECK_SIZE(CellSaveDataFileStat, 56);

	if (operation == SAVEDATA_OP_FIXED_DELETE)
	{
		if (setBuf->fileListMax != 0u)
		{
			// ****** sysutil savedata parameter error : 9 ******
			return 9;
		}
	}
	else if (setBuf->fileListMax * sizeof(CellSaveDataFileStat) > setBuf->bufSize)
	{
		// ****** sysutil savedata parameter error : 7 ******
		return 7;
	}

	if (setBuf->bufSize && !setBuf->buf)
	{
		// ****** sysutil savedata parameter error : 6 ******
		return 6;
	}

	for (auto resv : setBuf->reserved)
	{
		if (resv)
		{
			// ****** sysutil savedata parameter error : 10 ******
			return 10;
		}
	}

	if ((operation == SAVEDATA_OP_LIST_SAVE || operation == SAVEDATA_OP_LIST_LOAD) && !funcList)
	{
		// ****** sysutil savedata parameter error : 18 ******
		return 18;
	}
	else if ((operation == SAVEDATA_OP_FIXED_SAVE || operation == SAVEDATA_OP_FIXED_LOAD ||
		operation == SAVEDATA_OP_LIST_AUTO_LOAD || operation == SAVEDATA_OP_LIST_AUTO_SAVE || operation == SAVEDATA_OP_FIXED_DELETE) && !funcFixed)
	{
		// ****** sysutil savedata parameter error : 19 ******
		return 19;
	}

	if (!(unk_op_flags & 0x2) || operation == SAVEDATA_OP_AUTO_SAVE || operation == SAVEDATA_OP_AUTO_LOAD)
	{
		if (!funcStat)
		{
			// ****** sysutil savedata parameter error : 20 ******
			return 20;
		}

		if (!(unk_op_flags & 0x2) && !funcFile)
		{
			// ****** sysutil savedata parameter error : 18 ******
			return 18;
		}
	}

	if (userId > CELL_SYSUTIL_USERID_MAX)
	{
		// ****** sysutil savedata parameter error : 91 ******
		return 91;
	}

	return CELL_OK;
}

static NEVER_INLINE error_code savedata_op(ppu_thread& ppu, u32 operation, u32 version, vm::cptr<char> dirName,
	u32 errDialog, PSetList setList, PSetBuf setBuf, PFuncList funcList, PFuncFixed funcFixed, PFuncStat funcStat,
	PFuncFile funcFile, u32 container, u32 unk_op_flags /*TODO*/, vm::ptr<void> userdata, u32 userId, PFuncDone funcDone)
{
	if (const auto ecode = savedata_check_args(operation, version, dirName, errDialog, setList, setBuf, funcList, funcFixed, funcStat,
		funcFile, container, unk_op_flags, userdata, userId, funcDone))
	{
		return {CELL_SAVEDATA_ERROR_PARAM, std::to_string(ecode)};
	}

	std::unique_lock lock(g_fxo->get<savedata_mutex>()->mutex, std::try_to_lock);

	if (!lock)
	{
		return CELL_SAVEDATA_ERROR_BUSY;
	}

	// Simulate idle time while data is being sent to VSH
	const auto lv2_sleep = [](ppu_thread& ppu, size_t sleep_time)
	{
		lv2_obj::sleep(ppu);
		std::this_thread::sleep_for(std::chrono::microseconds(sleep_time));
		ppu.check_state();
	};

	lv2_sleep(ppu, 500);

	std::memset(g_savedata_context.get_ptr(), 0, g_savedata_context.size());

	vm::ptr<CellSaveDataCBResult> result   = g_savedata_context.ptr(&savedata_context::result);
	vm::ptr<CellSaveDataListGet>  listGet  = g_savedata_context.ptr(&savedata_context::listGet);
	vm::ptr<CellSaveDataListSet>  listSet  = g_savedata_context.ptr(&savedata_context::listSet);
	vm::ptr<CellSaveDataFixedSet> fixedSet = g_savedata_context.ptr(&savedata_context::fixedSet);
	vm::ptr<CellSaveDataStatGet>  statGet  = g_savedata_context.ptr(&savedata_context::statGet);
	vm::ptr<CellSaveDataStatSet>  statSet  = g_savedata_context.ptr(&savedata_context::statSet);
	vm::ptr<CellSaveDataFileGet>  fileGet  = g_savedata_context.ptr(&savedata_context::fileGet);
	vm::ptr<CellSaveDataFileSet>  fileSet  = g_savedata_context.ptr(&savedata_context::fileSet);
	vm::ptr<CellSaveDataDoneGet>  doneGet  = g_savedata_context.ptr(&savedata_context::doneGet);

	// userId(0) = CELL_SYSUTIL_USERID_CURRENT;
	// path of the specified user (00000001 by default)
	const std::string base_dir = vfs::get(fmt::format("/dev_hdd0/home/%08u/savedata/", userId ? userId : Emu.GetUsrId()));

	if (userId && !fs::is_dir(base_dir))
	{
		return CELL_SAVEDATA_ERROR_NOUSER;
	}

	result->userdata = userdata; // probably should be assigned only once (allows the callback to change it)

	SaveDataEntry save_entry;

	if (setList)
	{
		std::vector<SaveDataEntry> save_entries;

		listGet->dirNum = 0;
		listGet->dirListNum = 0;
		listGet->dirList.set(setBuf->buf.addr());
		std::memset(listGet->reserved, 0, sizeof(listGet->reserved));

		const auto prefix_list = fmt::split(setList->dirNamePrefix.get_ptr(), {"|"});

		// get the saves matching the supplied prefix
		for (auto&& entry : fs::dir(base_dir))
		{
			if (!entry.is_directory || sysutil_check_name_string(entry.name.c_str(), 1, CELL_SAVEDATA_DIRNAME_SIZE) != 0)
			{
				continue;
			}

			for (const auto& prefix : prefix_list)
			{
				if (entry.name.starts_with(prefix))
				{
					// Count the amount of matches and the amount of listed directories
					if (!listGet->dirNum++) // total number of directories
					{
						// Clear buf exactly to bufSize only if dirNum becomes non-zero (regardless of dirListNum)
						std::memset(setBuf->buf.get_ptr(), 0, setBuf->bufSize);
					}

					if (listGet->dirListNum < setBuf->dirListMax)
					{
						listGet->dirListNum++; // number of directories in list

						// PSF parameters
						const psf::registry psf = psf::load_object(fs::file(base_dir + entry.name + "/PARAM.SFO"));

						if (psf.empty())
						{
							break;
						}

						SaveDataEntry save_entry2;
						save_entry2.dirName = psf.at("SAVEDATA_DIRECTORY").as_string();
						save_entry2.listParam = psf.at("SAVEDATA_LIST_PARAM").as_string();
						save_entry2.title = psf.at("TITLE").as_string();
						save_entry2.subtitle = psf.at("SUB_TITLE").as_string();
						save_entry2.details = psf.at("DETAIL").as_string();

						for (const auto& entry2 : fs::dir(base_dir + entry.name))
						{
							save_entry2.size += entry2.size;
						}

						save_entry2.atime = entry.atime;
						save_entry2.mtime = entry.mtime;
						save_entry2.ctime = entry.ctime;
						if (fs::file icon{base_dir + entry.name + "/ICON0.PNG"})
							save_entry2.iconBuf = icon.to_vector<uchar>();
						save_entry2.isNew = false;

						save_entry2.escaped = std::move(entry.name);
						save_entries.emplace_back(save_entry2);
					}

					break;
				}
			}
		}

		// Sort the entries
		{
			const u32 order = setList->sortOrder;
			const u32 type = setList->sortType;

			std::sort(save_entries.begin(), save_entries.end(), [=](const SaveDataEntry& entry1, const SaveDataEntry& entry2)
			{
				if (order == CELL_SAVEDATA_SORTORDER_DESCENT && type == CELL_SAVEDATA_SORTTYPE_MODIFIEDTIME)
				{
					return entry1.mtime >= entry2.mtime;
				}
				if (order == CELL_SAVEDATA_SORTORDER_DESCENT && type == CELL_SAVEDATA_SORTTYPE_SUBTITLE)
				{
					return entry1.subtitle >= entry2.subtitle;
				}
				if (order == CELL_SAVEDATA_SORTORDER_ASCENT && type == CELL_SAVEDATA_SORTTYPE_MODIFIEDTIME)
				{
					return entry1.mtime < entry2.mtime;
				}
				if (order == CELL_SAVEDATA_SORTORDER_ASCENT && type == CELL_SAVEDATA_SORTTYPE_SUBTITLE)
				{
					return entry1.subtitle < entry2.subtitle;
				}

				return true;
			});
		}

		// Fill the listGet->dirList array
		auto dir_list = listGet->dirList.get_ptr();

		for (const auto& entry : save_entries)
		{
			auto& dir = *dir_list++;
			strcpy_trunc(dir.dirName, entry.dirName);
			strcpy_trunc(dir.listParam, entry.listParam);
		}

		s32 selected = -1;
		s32 focused = -1;

		if (funcList)
		{
			listSet->focusPosition = CELL_SAVEDATA_FOCUSPOS_LISTHEAD;

			std::memset(result.get_ptr(), 0, ::offset32(&CellSaveDataCBResult::userdata));

			// List Callback
			funcList(ppu, result, listGet, listSet);

			if (const s32 res = result->result; res != CELL_SAVEDATA_CBRESULT_OK_NEXT)
			{
				cellSaveData.warning("savedata_op(): funcList returned result=%d.", result->result);

				// if the callback has returned ok, lets return OK.
				// typically used at game launch when no list is actually required.
				// CELL_SAVEDATA_CBRESULT_OK_LAST_NOCONFIRM is only valid for funcFile and funcDone
				if (result->result == CELL_SAVEDATA_CBRESULT_OK_LAST || result->result == CELL_SAVEDATA_CBRESULT_OK_LAST_NOCONFIRM)
				{
					return CELL_OK;
				}

				return display_callback_result_error_message(ppu, *result, errDialog);
			}

			if (listSet->fixedListNum > CELL_SAVEDATA_LISTITEM_MAX)
			{
				// ****** sysutil savedata parameter error : 38 ******
				return {CELL_SAVEDATA_ERROR_PARAM, "38"};
			}

			if (listSet->fixedListNum && !listSet->fixedList)
			{
				// ****** sysutil savedata parameter error : 39 ******
				return {CELL_SAVEDATA_ERROR_PARAM, "39"};
			}
			else
			{
				// TODO: What happens if fixedListNum is zero?
			}

			std::set<std::string_view> selected_list;

			for (u32 i = 0; i < listSet->fixedListNum; i++)
			{
				switch (sysutil_check_name_string(listSet->fixedList[i].dirName, 1, CELL_SAVEDATA_DIRNAME_SIZE))
				{
				case -1:
				{
					// ****** sysutil savedata parameter error : 40 ******
					return {CELL_SAVEDATA_ERROR_PARAM, "40"};
				}
				case -2:
				{
					if (listSet->fixedList[i].dirName[0]) // ???
					{
						// ****** sysutil savedata parameter error : 41 ******
						return {CELL_SAVEDATA_ERROR_PARAM, "41"};
					}

					break;
				}
				case 0: break;
				default: ASSUME(0);
				}

				selected_list.emplace(listSet->fixedList[i].dirName);
			}

			// Clean save data list
			save_entries.erase(std::remove_if(save_entries.begin(), save_entries.end(), [&selected_list](const SaveDataEntry& entry) -> bool
			{
				return selected_list.count(entry.dirName) == 0;
			}), save_entries.end());

			if (listSet->newData)
			{
				switch (listSet->newData->iconPosition)
				{
				case CELL_SAVEDATA_ICONPOS_HEAD:
				case CELL_SAVEDATA_ICONPOS_TAIL:
					break;
				default:
				{
					// ****** sysutil savedata parameter error : 43 ******
					return {CELL_SAVEDATA_ERROR_PARAM, "43"};
				}
				}

				if (!listSet->newData->dirName)
				{
					// ****** sysutil savedata parameter error : 44 ******
					return {CELL_SAVEDATA_ERROR_PARAM, "44"};
				}

				switch (sysutil_check_name_string(listSet->newData->dirName.get_ptr(), 1, CELL_SAVEDATA_DIRNAME_SIZE))
				{
				case -1:
				{
					// ****** sysutil savedata parameter error : 45 ******
					return {CELL_SAVEDATA_ERROR_PARAM, "45"};
				}
				case -2:
				{
					if (listSet->newData->dirName[0]) // ???
					{
						// ****** sysutil savedata parameter error : 4 ******
						return {CELL_SAVEDATA_ERROR_PARAM, "46"};
					}

					break;
				}
				case 0: break;
				default: ASSUME(0);
				}
			}

			switch (const u32 pos_type = listSet->focusPosition)
			{
			case CELL_SAVEDATA_FOCUSPOS_DIRNAME:
			{
				if (!listSet->focusDirName)
				{
					// ****** sysutil savedata parameter error : 35 ******
					return {CELL_SAVEDATA_ERROR_PARAM, "35"};
				}

				switch (sysutil_check_name_string(listSet->focusDirName.get_ptr(), 1, CELL_SAVEDATA_DIRNAME_SIZE))
				{
				case -1:
				{
					// ****** sysutil savedata parameter error : 36 ******
					return {CELL_SAVEDATA_ERROR_PARAM, "36"};
				}
				case -2:
				{
					if (listSet->focusDirName[0]) // ???
					{
						// ****** sysutil savedata parameter error : 37 ******
						return {CELL_SAVEDATA_ERROR_PARAM, "37"};
					}

					break;
				}
				case 0: break;
				default: ASSUME(0);
				}

				const std::string dirStr = listSet->focusDirName.get_ptr();

				for (u32 i = 0; i < save_entries.size(); i++)
				{
					if (save_entries[i].dirName == dirStr)
					{
						focused = i;
						break;
					}
				}

				break;
			}
			case CELL_SAVEDATA_FOCUSPOS_LISTHEAD:
			{
				focused = save_entries.empty() ? -1 : 0;
				break;
			}
			case CELL_SAVEDATA_FOCUSPOS_LISTTAIL:
			{
				focused = ::size32(save_entries) - 1;
				break;
			}
			case CELL_SAVEDATA_FOCUSPOS_LATEST:
			{
				s64 max = INT64_MIN;

				for (u32 i = 0; i < save_entries.size(); i++)
				{
					if (save_entries[i].mtime > max)
					{
						focused = i;
						max = save_entries[i].mtime;
					}
				}

				break;
			}
			case CELL_SAVEDATA_FOCUSPOS_OLDEST:
			{
				s64 min = INT64_MAX;

				for (u32 i = 0; i < save_entries.size(); i++)
				{
					if (save_entries[i].mtime < min)
					{
						focused = i;
						min = save_entries[i].mtime;
					}
				}

				break;
			}
			case CELL_SAVEDATA_FOCUSPOS_NEWDATA:
			{
				if (!listSet->newData)
				{
					// ****** sysutil savedata parameter error : 34 ******
					cellSaveData.error("savedata_op(): listSet->newData is null while listSet->focusPosition is NEWDATA");
					return {CELL_SAVEDATA_ERROR_PARAM, "34"};
				}

				//TODO: If adding the new data to the save_entries vector
				// to be displayed in the save mangaer UI, it should be focused here
				break;
			}
			default:
			{
				// ****** sysutil savedata parameter error : 34 ******
				cellSaveData.error("savedata_op(): unknown listSet->focusPosition (0x%x)", pos_type);
				return {CELL_SAVEDATA_ERROR_PARAM, "34"};
			}
			}
		}

		auto delete_save = [&]()
		{
			strcpy_trunc(doneGet->dirName, save_entries[selected].dirName);
			doneGet->hddFreeSizeKB = 40 * 1024 * 1024 - 1; // Read explanation in cellHddGameCheck
			doneGet->excResult     = CELL_OK;
			std::memset(doneGet->reserved, 0, sizeof(doneGet->reserved));

			const std::string old_path = base_dir + ".backup_" + save_entries[selected].escaped + "/";
			const std::string del_path = base_dir + save_entries[selected].escaped + "/";

			const fs::dir _dir(del_path);
			u64 size_bytes = 0;

			for (auto&& file : _dir)
			{
				if (!file.is_directory)
				{
					size_bytes += ::align(file.size, 1024);
				}
			}

			doneGet->sizeKB = ::narrow<s32>(size_bytes / 1024);

			if (_dir)
			{
				// Remove old backup
				fs::remove_all(old_path);

				// Remove savedata by renaming
				if (!vfs::host::rename(del_path, old_path, false))
				{
					fmt::throw_exception("Failed to move directory %s (%s)", del_path, fs::g_tls_error);
				}

				// Cleanup
				fs::remove_all(old_path);
			}
			else
			{
				doneGet->excResult = CELL_SAVEDATA_ERROR_NODATA;
			}

			std::memset(result.get_ptr(), 0, ::offset32(&CellSaveDataCBResult::userdata));
			funcDone(ppu, result, doneGet);
		};

		while (funcList)
		{
			// Yield before a blocking dialog is being spawned
			lv2_obj::sleep(ppu);

			// Display a blocking Save Data List asynchronously in the GUI thread.
			if (auto save_dialog = Emu.GetCallbacks().get_save_dialog())
			{
				selected = save_dialog->ShowSaveDataList(save_entries, focused, operation, listSet);
			}
			else
			{
				selected = -2;
			}

			// Reschedule after a blocking dialog returns
			if (ppu.check_state())
			{
				return 0;
			}

			// Cancel selected in UI
			if (selected == -2)
			{
				return CELL_CANCEL;
			}

			std::string message;

			// UI returns -1 for new save games
			if (selected == -1)
			{
				message = "Create new Save Data?";
				save_entry.dirName = listSet->newData->dirName.get_ptr();
				save_entry.escaped = vfs::escape(save_entry.dirName);
			}
			else
			{
				// Get information from the selected entry
				SaveDataEntry entry = save_entries[selected];
				message = get_confirmation_message(operation) + "\n\n" + entry.title + "\n" + entry.subtitle + "\n" + entry.details;
			}

			// Yield before a blocking dialog is being spawned
			lv2_obj::sleep(ppu);

			// Get user confirmation by opening a blocking dialog
			error_code res = open_msg_dialog(true, CELL_MSGDIALOG_TYPE_SE_TYPE_NORMAL | CELL_MSGDIALOG_TYPE_BUTTON_TYPE_YESNO, vm::make_str(message));

			// Reschedule after a blocking dialog returns
			if (ppu.check_state())
			{
				return 0;
			}

			if (res != CELL_OK)
			{
				return CELL_SAVEDATA_ERROR_INTERNAL;
			}

			if (g_last_user_response != CELL_MSGDIALOG_BUTTON_YES)
			{
				continue;
			}

			if (operation == SAVEDATA_OP_LIST_DELETE)
			{
				delete_save();

				if (const s32 res = result->result; res != CELL_SAVEDATA_CBRESULT_OK_NEXT)
				{
					cellSaveData.warning("savedata_op(): funcDone returned result=%d.", res);

					if (res == CELL_SAVEDATA_CBRESULT_OK_LAST || res == CELL_SAVEDATA_CBRESULT_OK_LAST_NOCONFIRM)
					{
						return CELL_OK;
					}

					return display_callback_result_error_message(ppu, *result, errDialog);
				}

				// CELL_SAVEDATA_CBRESULT_OK_NEXT expected
				save_entries.erase(save_entries.cbegin() + selected);
				focused = save_entries.empty() ? -1 : selected;
				selected = -1;
				continue;
			}

			break;
		}

		if (funcFixed)
		{
			lv2_sleep(ppu, 250);

			std::memset(result.get_ptr(), 0, ::offset32(&CellSaveDataCBResult::userdata));

			// Fixed Callback
			funcFixed(ppu, result, listGet, fixedSet);

			if (const s32 res = result->result; res != CELL_SAVEDATA_CBRESULT_OK_NEXT)
			{
				cellSaveData.warning("savedata_op(): funcFixed returned result=%d.", res);

				// skip all following steps if OK_LAST (NOCONFIRM is not allowed)
				if (res == CELL_SAVEDATA_CBRESULT_OK_LAST)
				{
					return CELL_OK;
				}

				return display_callback_result_error_message(ppu, *result, errDialog);
			}

			if (!fixedSet->dirName)
			{
				// ****** sysutil savedata parameter error : 26 ******
				return {CELL_SAVEDATA_ERROR_PARAM, "26"};
			}

			switch (sysutil_check_name_string(fixedSet->dirName.get_ptr(), 1, CELL_SAVEDATA_DIRNAME_SIZE))
			{
			case -1:
			{
				// ****** sysutil savedata parameter error : 27 ******
				return {CELL_SAVEDATA_ERROR_PARAM, "27"};
			}
			case -2:
			{
				// ****** sysutil savedata parameter error : 28 ******
				return {CELL_SAVEDATA_ERROR_PARAM, "28"};
			}
			case 0: break;
			default: ASSUME(0);
			}

			const std::string dirStr = fixedSet->dirName.get_ptr();

			for (u32 i = 0; i < save_entries.size(); i++)
			{
				if (save_entries[i].dirName == dirStr)
				{
					selected = i;
					break;
				}
			}

			switch (fixedSet->option)
			{
			case CELL_SAVEDATA_OPTION_NONE:
			{
				if (operation != SAVEDATA_OP_FIXED_SAVE && operation != SAVEDATA_OP_FIXED_LOAD && operation != SAVEDATA_OP_FIXED_DELETE)
				{
					break;
				}

				std::string message;

				if (selected == -1)
				{
					message = "Create new Save Data?";
				}
				else
				{
					// Get information from the selected entry
					SaveDataEntry entry = save_entries[selected];
					message = get_confirmation_message(operation) + "\n\n" + entry.title + "\n" + entry.subtitle + "\n" + entry.details;
				}

				// Yield before a blocking dialog is being spawned
				lv2_obj::sleep(ppu);

				// Get user confirmation by opening a blocking dialog
				error_code res = open_msg_dialog(true, CELL_MSGDIALOG_TYPE_SE_TYPE_NORMAL | CELL_MSGDIALOG_TYPE_BUTTON_TYPE_YESNO, vm::make_str(message));

				// Reschedule after a blocking dialog returns
				if (ppu.check_state())
				{
					return 0;
				}

				if (res != CELL_OK)
				{
					return CELL_SAVEDATA_ERROR_INTERNAL;
				}

				if (g_last_user_response != CELL_MSGDIALOG_BUTTON_YES)
				{
					return CELL_CANCEL;
				}

				break;
			}
			case CELL_SAVEDATA_OPTION_NOCONFIRM:
				break;

			default :
				// ****** sysutil savedata parameter error : 81 ******
				return {CELL_SAVEDATA_ERROR_PARAM, "81"};
			}

			if (selected == -1)
			{
				save_entry.dirName = dirStr;
				save_entry.escaped = vfs::escape(save_entry.dirName);
			}

			if (operation == SAVEDATA_OP_FIXED_DELETE)
			{
				delete_save();

				if (const s32 res = result->result; res != CELL_SAVEDATA_CBRESULT_OK_NEXT)
				{
					cellSaveData.warning("savedata_op(): funcDone returned result=%d.", res);

					return display_callback_result_error_message(ppu, *result, errDialog);
				}

				return CELL_OK;
			}
		}

		if (listGet->dirNum)
		{
			// Clear buf exactly to bufSize again after funcFixed/List (for funcStat)
			std::memset(setBuf->buf.get_ptr(), 0, setBuf->bufSize);
		}

		if (selected >= 0)
		{
			if (selected + 0u < save_entries.size())
			{
				save_entry.dirName = std::move(save_entries[selected].dirName);
				save_entry.escaped = vfs::escape(save_entry.dirName);
			}
			else
			{
				fmt::throw_exception("Invalid savedata selected" HERE);
			}
		}
	}

	if (dirName)
	{
		save_entry.dirName = dirName.get_ptr();
		save_entry.escaped = vfs::escape(save_entry.dirName);
	}

	const std::string dir_path = base_dir + save_entry.escaped + "/";
	const std::string old_path = base_dir + ".backup_" + save_entry.escaped + "/";
	const std::string new_path = base_dir + ".working_" + save_entry.escaped + "/";

	psf::registry psf = psf::load_object(fs::file(dir_path + "PARAM.SFO"));
	bool has_modified = false;
	bool recreated = false;

	lv2_sleep(ppu, 250);

	// Check if RPCS3_BLIST section exist in PARAM.SFO
	// This section contains the list of files in the save ordered as they would be in BSD filesystem
	std::vector<std::string> blist;

	if (psf.count("RPCS3_BLIST"))
		blist = fmt::split(psf.at("RPCS3_BLIST").as_string(), {"/"}, false);

	// Get save stats
	{
		if (!funcStat)
		{
			// ****** sysutil savedata parameter error : 20 ******
			return {CELL_SAVEDATA_ERROR_PARAM, "20"};
		}

		fs::stat_t dir_info{};
		if (!fs::stat(dir_path, dir_info))
		{
			// funcStat is called even if the directory doesn't exist.
		}

		statGet->hddFreeSizeKB = 40 * 1024 * 1024 - 1; // Read explanation in cellHddGameCheck
		statGet->isNewData = save_entry.isNew = psf.empty();

		statGet->dir.atime = save_entry.atime = dir_info.atime;
		statGet->dir.mtime = save_entry.mtime = dir_info.mtime;
		statGet->dir.ctime = save_entry.ctime = dir_info.ctime;
		strcpy_trunc(statGet->dir.dirName, save_entry.dirName);

		if (!psf.empty())
		{
			statGet->getParam.parental_level = psf.at("PARENTAL_LEVEL").as_integer();
			statGet->getParam.attribute = psf.at("ATTRIBUTE").as_integer(); // ???
			strcpy_trunc(statGet->getParam.title, save_entry.title = psf.at("TITLE").as_string());
			strcpy_trunc(statGet->getParam.subTitle, save_entry.subtitle = psf.at("SUB_TITLE").as_string());
			strcpy_trunc(statGet->getParam.detail, save_entry.details = psf.at("DETAIL").as_string());
			strcpy_trunc(statGet->getParam.listParam, save_entry.listParam = psf.at("SAVEDATA_LIST_PARAM").as_string());
		}

		statGet->bind = 0;
		statGet->fileNum = 0;
		statGet->fileList.set(setBuf->buf.addr());
		statGet->fileListNum = 0;
		std::memset(statGet->reserved, 0, sizeof(statGet->reserved));

		if (!save_entry.isNew)
		{
			// Clear to bufSize if !isNew regardless of fileNum
			std::memset(setBuf->buf.get_ptr(), 0, setBuf->bufSize);
		}

		auto file_list = statGet->fileList.get_ptr();

		u64 size_bytes = 0;

		std::vector<fs::dir_entry> files_sorted;

		for (auto&& entry : fs::dir(dir_path))
		{
			entry.name = vfs::unescape(entry.name);

			if (!entry.is_directory)
			{
				if (entry.name == "PARAM.SFO" || entry.name == "PARAM.PFD")
				{
					continue; // system files are not included in the file list
				}

				files_sorted.push_back(entry);
			}
		}

		// clang-format off
		std::sort(files_sorted.begin(), files_sorted.end(), [&](const fs::dir_entry& a, const fs::dir_entry& b) -> bool
		{
			const auto a_it = std::find(blist.begin(), blist.end(), a.name);
			const auto b_it = std::find(blist.begin(), blist.end(), b.name);

			if (a_it == blist.end() && b_it == blist.end())
			{
				// Order alphabetically for old saves
				return a.name.compare(b.name);
			}

			return a_it < b_it;
		});
		// clang-format on

		for (auto&& entry : files_sorted)
		{
			{
				statGet->fileNum++;

				size_bytes += ::align(entry.size, 1024); // firmware rounds this value up

				if (statGet->fileListNum >= setBuf->fileListMax)
					continue;

				statGet->fileListNum++;

				auto& file = *file_list++;

				file.size = entry.size;
				file.atime = entry.atime;
				file.mtime = entry.mtime;
				file.ctime = entry.ctime;
				strcpy_trunc(file.fileName, entry.name);

				if (entry.name == "ICON0.PNG")
				{
					file.fileType = CELL_SAVEDATA_FILETYPE_CONTENT_ICON0;
				}
				else if (entry.name == "ICON1.PAM")
				{
					file.fileType = CELL_SAVEDATA_FILETYPE_CONTENT_ICON1;
				}
				else if (entry.name == "PIC1.PNG")
				{
					file.fileType = CELL_SAVEDATA_FILETYPE_CONTENT_PIC1;
				}
				else if (entry.name == "SND0.AT3")
				{
					file.fileType = CELL_SAVEDATA_FILETYPE_CONTENT_SND0;
				}
				else if (psf::get_integer(psf, "*" + entry.name)) // let's put the list of protected files in PARAM.SFO (int param = 1 if protected)
				{
					file.fileType = CELL_SAVEDATA_FILETYPE_SECUREFILE;
				}
				else
				{
					file.fileType = CELL_SAVEDATA_FILETYPE_NORMALFILE;
				}
			}
		}

		statGet->sysSizeKB = 35; // always reported as 35 regardless of actual file sizes
		statGet->sizeKB = !save_entry.isNew ? ::narrow<s32>((size_bytes / 1024) + statGet->sysSizeKB) : 0;

		std::memset(result.get_ptr(), 0, ::offset32(&CellSaveDataCBResult::userdata));

		// Stat Callback
		funcStat(ppu, result, statGet, statSet);

		if (const s32 res = result->result; res != CELL_SAVEDATA_CBRESULT_OK_NEXT)
		{
			cellSaveData.warning("savedata_op(): funcStat returned result=%d.", res);

			// Skip and return without error on OK_LAST (NOCONFIRM is not allowed)
			if (res == CELL_SAVEDATA_CBRESULT_OK_LAST)
			{
				return CELL_OK;
			}

			return display_callback_result_error_message(ppu, *result, errDialog);
		}

		if (statSet->setParam)
		{
			if (statSet->setParam->attribute > CELL_SAVEDATA_ATTR_NODUPLICATE)
			{
				// ****** sysutil savedata parameter error : 57 ******
				return {CELL_SAVEDATA_ERROR_PARAM, "57"};
			}

			if (g_ps3_process_info.sdk_ver > 0x36FFFF)
			{
				// In firmware 3.70 or higher parental_level was changed to reserved2 and has to zeroes
				if (statSet->setParam->parental_level)
				{
					// ****** sysutil savedata parameter error : 58 ******
					return {CELL_SAVEDATA_ERROR_PARAM, "58"};
				}
			}
			else
			{
				if (statSet->setParam->parental_level > 11)
				{
					// ****** sysutil savedata parameter error : 58 ******
					return {CELL_SAVEDATA_ERROR_PARAM, "58"};
				}
			}

			for (u8 resv : statSet->setParam->reserved)
			{
				if (resv)
				{
					// ****** sysutil savedata parameter error : 59 ******
					return {CELL_SAVEDATA_ERROR_PARAM, "59"};
				}
			}

			// Update PARAM.SFO
			psf.clear();
			psf.insert(
			{
				{ "ACCOUNT_ID", psf::array(16, "0000000000000000") }, // ???
				{ "ATTRIBUTE", statSet->setParam->attribute.value() },
				{ "CATEGORY",  psf::string(4, "SD") }, // ???
				{ "PARAMS", psf::string(1024, {}) }, // ???
				{ "PARAMS2", psf::string(12, {}) }, // ???
				{ "PARENTAL_LEVEL", statSet->setParam->parental_level.value() },
				{ "DETAIL", psf::string(CELL_SAVEDATA_SYSP_DETAIL_SIZE, statSet->setParam->detail) },
				{ "SAVEDATA_DIRECTORY", psf::string(CELL_SAVEDATA_DIRNAME_SIZE, save_entry.dirName) },
				{ "SAVEDATA_LIST_PARAM", psf::string(CELL_SAVEDATA_SYSP_LPARAM_SIZE, statSet->setParam->listParam) },
				{ "SUB_TITLE", psf::string(CELL_SAVEDATA_SYSP_SUBTITLE_SIZE, statSet->setParam->subTitle) },
				{ "TITLE", psf::string(CELL_SAVEDATA_SYSP_TITLE_SIZE, statSet->setParam->title) }
			});

			has_modified = true;
		}
		else if (save_entry.isNew)
		{
			// ****** sysutil savedata parameter error : 50 ******
			return {CELL_SAVEDATA_ERROR_PARAM, "50"};
		}

		switch (const u32 mode = statSet->reCreateMode & CELL_SAVEDATA_RECREATE_MASK)
		{
		case CELL_SAVEDATA_RECREATE_NO:
		{
			//CELL_SAVEDATA_RECREATE_NO = overwrite and let the user know, not data is corrupt.
			//cellSaveData.error("Savedata %s considered broken", save_entry.dirName);
			//TODO: if this is a save, and it's not auto, then show a dialog
			// fallthrough
		}

		case CELL_SAVEDATA_RECREATE_NO_NOBROKEN:
		{
			break;
		}

		case CELL_SAVEDATA_RECREATE_YES:
		case CELL_SAVEDATA_RECREATE_YES_RESET_OWNER:
		{
			if (!statSet->setParam)
			{
				// ****** sysutil savedata parameter error : 50 ******
				return {CELL_SAVEDATA_ERROR_PARAM, "50"};
			}

			// Clear secure file info
			for (auto it = psf.cbegin(), end = psf.cend(); it != end;)
			{
				if (it->first[0] == '*')
					it = psf.erase(it);
				else
					it++;
			}

			// Clear order info
			blist.clear();

			// Set to not load files on next step
			recreated = true;
			has_modified = true;
			break;
		}

		default:
		{
			// ****** sysutil savedata parameter error : 48 ******
			cellSaveData.error("savedata_op(): unknown statSet->reCreateMode (0x%x)", statSet->reCreateMode);
			return {CELL_SAVEDATA_ERROR_PARAM, "48"};
		}
		}
	}

	// Create save directory if necessary
	if (!psf.empty() && save_entry.isNew && !fs::create_dir(dir_path) && fs::g_tls_error != fs::error::exist)
	{
		cellSaveData.warning("savedata_op(): failed to create %s (%s)", dir_path, fs::g_tls_error);
		return CELL_SAVEDATA_ERROR_ACCESS_ERROR;
	}

	// Enter the loop where the save files are read/created/deleted
	std::map<std::string, std::pair<s64, s64>> all_times;
	std::map<std::string, fs::file> all_files;

	// First, preload all files (TODO: beware of possible lag, although it should be insignificant)
	for (auto&& entry : fs::dir(dir_path))
	{
		if (!recreated && !entry.is_directory)
		{
			// Read file into a vector and make a memory file
			entry.name = vfs::unescape(entry.name);
			all_times.emplace(entry.name, std::make_pair(entry.atime, entry.mtime));
			all_files.emplace(std::move(entry.name), fs::make_stream(fs::file(dir_path + entry.name).to_vector<uchar>()));
		}
	}

	fileGet->excSize = 0;

	error_code savedata_result = CELL_OK;

	while (funcFile)
	{
		std::memset(fileSet.get_ptr(), 0, fileSet.size());
		std::memset(fileGet->reserved, 0, sizeof(fileGet->reserved));
		std::memset(result.get_ptr(), 0, ::offset32(&CellSaveDataCBResult::userdata));

		funcFile(ppu, result, fileGet, fileSet);

		if (const s32 res = result->result; res != CELL_SAVEDATA_CBRESULT_OK_NEXT)
		{
			if (res == CELL_SAVEDATA_CBRESULT_OK_LAST || res == CELL_SAVEDATA_CBRESULT_OK_LAST_NOCONFIRM)
			{
				// TODO: display user prompt
				break;
			}

			cellSaveData.warning("savedata_op(): funcFile returned result=%d.", res);

			if (res < CELL_SAVEDATA_CBRESULT_ERR_INVALID || res > CELL_SAVEDATA_CBRESULT_OK_LAST_NOCONFIRM)
			{
				// ****** sysutil savedata parameter error : 22 ******
				savedata_result = {CELL_SAVEDATA_ERROR_PARAM, "22"};
				break;
			}

			savedata_result = {CELL_SAVEDATA_ERROR_CBRESULT, res};
			break;
		}

		// TODO: Show progress if it's not an auto load/save

		std::string file_path;

		switch (const u32 type = fileSet->fileType)
		{
		case CELL_SAVEDATA_FILETYPE_SECUREFILE:
		case CELL_SAVEDATA_FILETYPE_NORMALFILE:
		{
			if (!fileSet->fileName)
			{
				// ****** sysutil savedata parameter error : 69 ******
				savedata_result = {CELL_SAVEDATA_ERROR_PARAM, "69"};
				break;
			}

			const char* fileName = fileSet->fileName.get_ptr();

			if (const auto termpos = std::memchr(fileName, '\0', CELL_SAVEDATA_FILENAME_SIZE))
			{
				file_path.assign(fileName, static_cast<const char*>(termpos));
			}
			else
			{
				// ****** sysutil savedata parameter error : 71 ******
				savedata_result = {CELL_SAVEDATA_ERROR_PARAM, "71"};
				break;
			}

			auto dotpos = file_path.find_last_of('.');

			if (dotpos == umax)
			{
				// Point to end of string instead
				dotpos = file_path.size();
			}

			if (file_path.empty() || dotpos > 8u || file_path.size() - dotpos > 4u)
			{
				cellSaveData.error("savedata_op(): fileSet->fileName is illegal ('%s')", file_path);

				// ****** sysutil savedata parameter error : 70 ******
				savedata_result = {CELL_SAVEDATA_ERROR_PARAM, "70"};
				break;
			}

			char name[10];

			if (dotpos)
			{
				// Copy file name
				gsl::span dst(name, dotpos + 1);
				strcpy_trunc(dst, file_path);

				// Allow multiple '.' even though sysutil_check_name_string does not
				std::replace(name, name + dotpos, '.', '-');

				// Allow '_' at start even though sysutil_check_name_string does not
				if (name[0] == '_')
				{
					name[0] = '-';
				}

				if ((dotpos >= 5u && std::memcmp(name, "PARAM", 5) == 0) ||
					(dotpos >= 4u && std::memcmp(name, "ICON", 4) == 0) ||
					(dotpos >= 3u && std::memcmp(name, "PIC", 3) == 0) ||
					(dotpos >= 3u && std::memcmp(name, "SND", 3) == 0))
				{
					// ****** sysutil savedata parameter error : 70 ******
					cellSaveData.error("savedata_op(): fileSet->fileName is set to a system file name (%s)", file_path);
					savedata_result = {CELL_SAVEDATA_ERROR_PARAM, "70"};
					break;
				}

				// Check filename
				if (sysutil_check_name_string(name, 1, 9) == -1)
				{
					cellSaveData.error("savedata_op(): fileSet->fileName is illegal due to file name ('%s')", file_path);

					// ****** sysutil savedata parameter error : 70 ******
					savedata_result = {CELL_SAVEDATA_ERROR_PARAM, "70"};
					break;
				}
			}

			if (file_path.size() > dotpos + 1)
			{
				// Copy file extension
				gsl::span dst(name, file_path.size() - dotpos);
				strcpy_trunc(dst, file_path.operator std::string_view().substr(dotpos + 1));

				// Allow '_' at start even though sysutil_check_name_string does not
				if (name[0] == '_')
				{
					name[0] = '-';
				}

				// Check file extension
				if (sysutil_check_name_string(name, 1, 4) == -1)
				{
					cellSaveData.error("savedata_op(): fileSet->fileName is illegal due to file extension ('%s')", file_path);

					// ****** sysutil savedata parameter error : 70 ******
					savedata_result = {CELL_SAVEDATA_ERROR_PARAM, "70"};
					break;
				}
			}

			if (type == CELL_SAVEDATA_FILETYPE_SECUREFILE)
			{
				cellSaveData.notice("SECUREFILE: %s -> %s", file_path, fileSet->secureFileId);
			}

			break;
		}
		case CELL_SAVEDATA_FILETYPE_CONTENT_ICON0:
		{
			file_path = "ICON0.PNG";
			break;
		}
		case CELL_SAVEDATA_FILETYPE_CONTENT_ICON1:
		{
			file_path = "ICON1.PAM";
			break;
		}
		case CELL_SAVEDATA_FILETYPE_CONTENT_PIC1:
		{
			file_path = "PIC1.PNG";
			break;
		}
		case CELL_SAVEDATA_FILETYPE_CONTENT_SND0:
		{
			file_path = "SND0.AT3";
			break;
		}
		default:
		{
			// ****** sysutil savedata parameter error : 61 ******
			cellSaveData.error("savedata_op(): unknown fileSet->fileType (0x%x)", type);
			savedata_result = {CELL_SAVEDATA_ERROR_PARAM, "61"};
			break;
		}
		}

		if (savedata_result)
		{
			break;
		}

		// clang-format off
		auto add_to_blist = [&](const std::string& to_add)
		{
			if (std::find(blist.begin(), blist.end(), to_add) == blist.end())
			{
				if(auto it = std::find(blist.begin(), blist.end(), ""); it != blist.end())
					*it = to_add;
				else
					blist.push_back(to_add);
			}
		};

		auto del_from_blist = [&](const std::string& to_del)
		{
			if (auto it = std::find(blist.begin(), blist.end(), to_del); it != blist.end())
				*it = "";
		};
		// clang-format on

		if ((file_path == "." || file_path == "..") && fileSet->fileOperation <= CELL_SAVEDATA_FILEOP_WRITE_NOTRUNC)
		{
			savedata_result = CELL_SAVEDATA_ERROR_BROKEN;
			break;
		}

		switch (const u32 op = fileSet->fileOperation)
		{
		case CELL_SAVEDATA_FILEOP_READ:
		{
			if (fileSet->fileBufSize < fileSet->fileSize)
			{
				// ****** sysutil savedata parameter error : 72 ******
				savedata_result = {CELL_SAVEDATA_ERROR_PARAM, "72"};
				break;
			}

			if (!fileSet->fileBuf && fileSet->fileBufSize)
			{
				// ****** sysutil savedata parameter error : 73 ******
				savedata_result = {CELL_SAVEDATA_ERROR_PARAM, "73"};
				break;
			}

			const auto file = std::as_const(all_files).find(file_path);

			if (file == all_files.cend() || file->second.size() <= fileSet->fileOffset)
			{
				cellSaveData.error("Failed to open file %s%s", dir_path, file_path);
				savedata_result = CELL_SAVEDATA_ERROR_FAILURE;
				break;
			}

			// Read from memory file to vm
			const u64 sr = file->second.seek(fileSet->fileOffset);
			const u64 rr = lv2_file::op_read(file->second, fileSet->fileBuf, fileSet->fileSize);
			fileGet->excSize = ::narrow<u32>(rr);
			break;
		}

		case CELL_SAVEDATA_FILEOP_WRITE:
		{
			if (fileSet->fileBufSize < fileSet->fileSize)
			{
				// ****** sysutil savedata parameter error : 72 ******
				savedata_result = {CELL_SAVEDATA_ERROR_PARAM, "72"};
				break;
			}

			if (!fileSet->fileBuf && fileSet->fileBufSize)
			{
				// ****** sysutil savedata parameter error : 73 ******
				savedata_result = {CELL_SAVEDATA_ERROR_PARAM, "73"};
				break;
			}

			fs::file& file = all_files[file_path];

			if (!file)
			{
				file = fs::make_stream<std::vector<uchar>>();
			}

			// Write to memory file and truncate
			const u64 sr = file.seek(fileSet->fileOffset);
			const u64 wr = lv2_file::op_write(file, fileSet->fileBuf, fileSet->fileSize);
			file.trunc(sr + wr);
			fileGet->excSize = ::narrow<u32>(wr);
			all_times.erase(file_path);
			add_to_blist(file_path);
			has_modified = true;
			break;
		}

		case CELL_SAVEDATA_FILEOP_DELETE:
		{
			// Delete memory file
			if (all_files.erase(file_path) == 0)
			{
				cellSaveData.error("Failed to delete file %s%s", dir_path, file_path);
				savedata_result = CELL_SAVEDATA_ERROR_FAILURE;
				break;
			}

			psf.erase("*" + file_path);
			fileGet->excSize = 0;
			all_times.erase(file_path);
			del_from_blist(file_path);
			has_modified = true;
			break;
		}

		case CELL_SAVEDATA_FILEOP_WRITE_NOTRUNC:
		{
			if (fileSet->fileBufSize < fileSet->fileSize)
			{
				// ****** sysutil savedata parameter error : 72 ******
				savedata_result = {CELL_SAVEDATA_ERROR_PARAM, "72"};
				break;
			}

			if (!fileSet->fileBuf && fileSet->fileBufSize)
			{
				// ****** sysutil savedata parameter error : 73 ******
				savedata_result = {CELL_SAVEDATA_ERROR_PARAM, "73"};
				break;
			}

			fs::file& file = all_files[file_path];

			if (!file)
			{
				file = fs::make_stream<std::vector<uchar>>();
			}

			// Write to memory file normally
			const u64 sr = file.seek(fileSet->fileOffset);
			const u64 wr = lv2_file::op_write(file, fileSet->fileBuf, fileSet->fileSize);
			fileGet->excSize = ::narrow<u32>(wr);
			all_times.erase(file_path);
			add_to_blist(file_path);
			has_modified = true;
			break;
		}

		default:
		{
			// ****** sysutil savedata parameter error : 60 ******
			cellSaveData.error("savedata_op(): unknown fileSet->fileOperation (0x%x)", op);
			savedata_result = {CELL_SAVEDATA_ERROR_PARAM, "60"};
			break;
		}
		}

		if (savedata_result)
		{
			break;
		}

		if (fileSet->fileOperation != CELL_SAVEDATA_FILEOP_DELETE)
		{
			psf.emplace("*" + file_path, fileSet->fileType == CELL_SAVEDATA_FILETYPE_SECUREFILE);
		}
	}

	// Write PARAM.SFO and savedata
	if (!psf.empty() && has_modified)
	{
		// First, create temporary directory
		if (fs::create_dir(new_path) || fs::g_tls_error == fs::error::exist)
		{
			fs::remove_all(new_path, false);
		}
		else
		{
			fmt::throw_exception("Failed to create directory %s (%s)", new_path, fs::g_tls_error);
		}

		// add file list per FS order to PARAM.SFO
		std::string final_blist;
		final_blist = fmt::merge(blist, "/");
		psf::assign(psf, "RPCS3_BLIST", psf::string(::align(::size32(final_blist) + 1, 4), final_blist));

		// Write all files in temporary directory
		auto& fsfo = all_files["PARAM.SFO"];
		fsfo = fs::make_stream<std::vector<uchar>>();
		psf::save_object(fsfo, psf);

		for (auto&& pair : all_files)
		{
			if (auto file = pair.second.release())
			{
				auto fvec = static_cast<fs::container_stream<std::vector<uchar>>&>(*file);
				fs::file(new_path + vfs::escape(pair.first), fs::rewrite).write(fvec.obj);
			}
		}

		for (auto&& pair : all_times)
		{
			// Restore atime/mtime for files which have not been modified
			fs::utime(new_path + vfs::escape(pair.first), pair.second.first, pair.second.second);
		}

		// Remove old backup
		fs::remove_all(old_path);

		// Backup old savedata
		if (!vfs::host::rename(dir_path, old_path, false))
		{
			fmt::throw_exception("Failed to move directory %s (%s)", dir_path, fs::g_tls_error);
		}

		// Commit new savedata
		if (!vfs::host::rename(new_path, dir_path, false))
		{
			// TODO: handle the case when only commit failed at the next save load
			fmt::throw_exception("Failed to move directory %s (%s)", new_path, fs::g_tls_error);
		}

		// Remove backup again (TODO: may be changed to persistent backup implementation)
		fs::remove_all(old_path);
	}

	if (savedata_result + 0u == CELL_SAVEDATA_ERROR_CBRESULT)
	{
		return display_callback_result_error_message(ppu, *result, errDialog);
	}

	return savedata_result;
}

static NEVER_INLINE error_code savedata_get_list_item(vm::cptr<char> dirName, vm::ptr<CellSaveDataDirStat> dir, vm::ptr<CellSaveDataSystemFileParam> sysFileParam, vm::ptr<u32> bind, vm::ptr<u32> sizeKB, u32 userId)
{
	if (userId == CELL_SYSUTIL_USERID_CURRENT)
	{
		userId = Emu.GetUsrId();
	}
	else if (userId > CELL_USERINFO_USER_MAX)
	{
		// ****** sysutil savedata parameter error : 137 ******
		return {CELL_SAVEDATA_ERROR_PARAM, "137"};
	}

	if (!dirName)
	{
		// ****** sysutil savedata parameter error : 107 ******
		return {CELL_SAVEDATA_ERROR_PARAM, "107"};
	}

	switch (sysutil_check_name_string(dirName.get_ptr(), 1, CELL_SAVEDATA_DIRLIST_MAX))
	{
	case -1:
	{
		// ****** sysutil savedata parameter error : 108 ******
		return {CELL_SAVEDATA_ERROR_PARAM, "108"};
	}
	case -2:
	{
		// ****** sysutil savedata parameter error : 109 ******
		return {CELL_SAVEDATA_ERROR_PARAM, "109"};
	}
	case 0: break;
	default: ASSUME(0);
	}

	const std::string base_dir = fmt::format("/dev_hdd0/home/%08u/savedata/", userId);

	if (!fs::is_dir(vfs::get(base_dir)))
	{
		return CELL_SAVEDATA_ERROR_NOUSER;
	}

	const std::string save_path = vfs::get(base_dir + dirName.get_ptr() + '/');
	std::string sfo = save_path + "PARAM.SFO";

	if (!fs::is_dir(save_path) && !fs::is_file(sfo))
	{
		cellSaveData.error("cellSaveDataGetListItem(): Savedata at %s does not exist", dirName);
		return CELL_SAVEDATA_ERROR_NODATA;
	}

	auto psf = psf::load_object(fs::file(sfo));

	if (sysFileParam)
	{
		strcpy_trunc(sysFileParam->listParam, psf.at("SAVEDATA_LIST_PARAM").as_string());
		strcpy_trunc(sysFileParam->title, psf.at("TITLE").as_string());
		strcpy_trunc(sysFileParam->subTitle, psf.at("SUB_TITLE").as_string());
		strcpy_trunc(sysFileParam->detail, psf.at("DETAIL").as_string());
	}

	if (dir)
	{
		fs::stat_t dir_info{};
		if (!fs::stat(save_path, dir_info))
		{
			return CELL_SAVEDATA_ERROR_INTERNAL;
		}

		// get file stats, namely directory
		strcpy_trunc(dir->dirName, std::string_view(dirName.get_ptr()));
		dir->atime = dir_info.atime;
		dir->ctime = dir_info.ctime;
		dir->mtime = dir_info.mtime;
	}

	if (sizeKB)
	{
		u32 size_kbytes = 0;

		for (const auto& entry : fs::dir(save_path))
		{
			if (entry.is_directory)
			{
				continue;
			}

			size_kbytes += ::narrow<u32>((entry.size + 1023) / 1024); // firmware rounds this value up
		}

		*sizeKB = size_kbytes;
	}

	if (bind)
	{
		//TODO: Set bind in accordance to any problems
		*bind = 0;
	}

	return CELL_OK;
}

// Functions
error_code cellSaveDataListSave2(ppu_thread& ppu, u32 version, PSetList setList, PSetBuf setBuf, PFuncList funcList,
	PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.warning("cellSaveDataListSave2(version=%d, setList=*0x%x, setBuf=*0x%x, funcList=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, setList, setBuf, funcList, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_LIST_SAVE, version, vm::null, CELL_SAVEDATA_ERRDIALOG_ALWAYS, setList, setBuf, funcList, vm::null, funcStat, funcFile, container, 2, userdata, 0, vm::null);
}

error_code cellSaveDataListLoad2(ppu_thread& ppu, u32 version, PSetList setList, PSetBuf setBuf, PFuncList funcList,
	PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.warning("cellSaveDataListLoad2(version=%d, setList=*0x%x, setBuf=*0x%x, funcList=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, setList, setBuf, funcList, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_LIST_LOAD, version, vm::null, CELL_SAVEDATA_ERRDIALOG_ALWAYS, setList, setBuf, funcList, vm::null, funcStat, funcFile, container, 2, userdata, 0, vm::null);
}

error_code cellSaveDataListSave(ppu_thread& ppu, u32 version, PSetList setList, PSetBuf setBuf, PFuncList funcList,
	PFuncStat funcStat, PFuncFile funcFile, u32 container)
{
	cellSaveData.warning("cellSaveDataListSave(version=%d, setList=*0x%x, setBuf=*0x%x, funcList=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x)",
		version, setList, setBuf, funcList, funcStat, funcFile, container);

	return savedata_op(ppu, SAVEDATA_OP_LIST_SAVE, version, vm::null, CELL_SAVEDATA_ERRDIALOG_ALWAYS, setList, setBuf, funcList, vm::null, funcStat, funcFile, container, 2, vm::null, 0, vm::null);
}

error_code cellSaveDataListLoad(ppu_thread& ppu, u32 version, PSetList setList, PSetBuf setBuf, PFuncList funcList,
	PFuncStat funcStat, PFuncFile funcFile, u32 container)
{
	cellSaveData.warning("cellSaveDataListLoad(version=%d, setList=*0x%x, setBuf=*0x%x, funcList=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x)",
		version, setList, setBuf, funcList, funcStat, funcFile, container);

	return savedata_op(ppu, SAVEDATA_OP_LIST_LOAD, version, vm::null, CELL_SAVEDATA_ERRDIALOG_ALWAYS, setList, setBuf, funcList, vm::null, funcStat, funcFile, container, 2, vm::null, 0, vm::null);
}

error_code cellSaveDataFixedSave2(ppu_thread& ppu, u32 version, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed,
	PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.warning("cellSaveDataFixedSave2(version=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_FIXED_SAVE, version, vm::null, CELL_SAVEDATA_ERRDIALOG_ALWAYS, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 2, userdata, 0, vm::null);
}

error_code cellSaveDataFixedLoad2(ppu_thread& ppu, u32 version, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed,
	PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.warning("cellSaveDataFixedLoad2(version=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_FIXED_LOAD, version, vm::null, CELL_SAVEDATA_ERRDIALOG_ALWAYS, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 2, userdata, 0, vm::null);
}

error_code cellSaveDataFixedSave(ppu_thread& ppu, u32 version, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed,
	PFuncStat funcStat, PFuncFile funcFile, u32 container)
{
	cellSaveData.warning("cellSaveDataFixedSave(version=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x)",
		version, setList, setBuf, funcFixed, funcStat, funcFile, container);

	return savedata_op(ppu, SAVEDATA_OP_FIXED_SAVE, version, vm::null, CELL_SAVEDATA_ERRDIALOG_ALWAYS, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 2, vm::null, 0, vm::null);
}

error_code cellSaveDataFixedLoad(ppu_thread& ppu, u32 version, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed,
	PFuncStat funcStat, PFuncFile funcFile, u32 container)
{
	cellSaveData.warning("cellSaveDataFixedLoad(version=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x)",
		version, setList, setBuf, funcFixed, funcStat, funcFile, container);

	return savedata_op(ppu, SAVEDATA_OP_FIXED_LOAD, version, vm::null, CELL_SAVEDATA_ERRDIALOG_ALWAYS, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 2, vm::null, 0, vm::null);
}

error_code cellSaveDataAutoSave2(ppu_thread& ppu, u32 version, vm::cptr<char> dirName, u32 errDialog, PSetBuf setBuf,
	PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.warning("cellSaveDataAutoSave2(version=%d, dirName=%s, errDialog=%d, setBuf=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, dirName, errDialog, setBuf, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_AUTO_SAVE, version, dirName, errDialog, vm::null, setBuf, vm::null, vm::null, funcStat, funcFile, container, 2, userdata, 0, vm::null);
}

error_code cellSaveDataAutoLoad2(ppu_thread& ppu, u32 version, vm::cptr<char> dirName, u32 errDialog, PSetBuf setBuf,
	PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.warning("cellSaveDataAutoLoad2(version=%d, dirName=%s, errDialog=%d, setBuf=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, dirName, errDialog, setBuf, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_AUTO_LOAD, version, dirName, errDialog, vm::null, setBuf, vm::null, vm::null, funcStat, funcFile, container, 2, userdata, 0, vm::null);
}

error_code cellSaveDataAutoSave(ppu_thread& ppu, u32 version, vm::cptr<char> dirName, u32 errDialog, PSetBuf setBuf,
	PFuncStat funcStat, PFuncFile funcFile, u32 container)
{
	cellSaveData.warning("cellSaveDataAutoSave(version=%d, dirName=%s, errDialog=%d, setBuf=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x)",
		version, dirName, errDialog, setBuf, funcStat, funcFile, container);

	return savedata_op(ppu, SAVEDATA_OP_AUTO_SAVE, version, dirName, errDialog, vm::null, setBuf, vm::null, vm::null, funcStat, funcFile, container, 2, vm::null, 0, vm::null);
}

error_code cellSaveDataAutoLoad(ppu_thread& ppu, u32 version, vm::cptr<char> dirName, u32 errDialog, PSetBuf setBuf,
	PFuncStat funcStat, PFuncFile funcFile, u32 container)
{
	cellSaveData.warning("cellSaveDataAutoLoad(version=%d, dirName=%s, errDialog=%d, setBuf=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x)",
		version, dirName, errDialog, setBuf, funcStat, funcFile, container);

	return savedata_op(ppu, SAVEDATA_OP_AUTO_LOAD, version, dirName, errDialog, vm::null, setBuf, vm::null, vm::null, funcStat, funcFile, container, 2, vm::null, 0, vm::null);
}

error_code cellSaveDataListAutoSave(ppu_thread& ppu, u32 version, u32 errDialog, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.warning("cellSaveDataListAutoSave(version=%d, errDialog=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, errDialog, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_LIST_AUTO_SAVE, version, vm::null, errDialog, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 2, userdata, 0, vm::null);
}

error_code cellSaveDataListAutoLoad(ppu_thread& ppu, u32 version, u32 errDialog, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.warning("cellSaveDataListAutoLoad(version=%d, errDialog=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, errDialog, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_LIST_AUTO_LOAD, version, vm::null, errDialog, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 2, userdata, 0, vm::null);
}

error_code cellSaveDataDelete(ppu_thread& ppu, u32 container)
{
	cellSaveData.warning("cellSaveDataDelete(container=0x%x)", container);

	return select_and_delete(ppu);
}

error_code cellSaveDataDelete2(ppu_thread& ppu, u32 container)
{
	cellSaveData.warning("cellSaveDataDelete2(container=0x%x)", container);

	return select_and_delete(ppu);
}

error_code cellSaveDataFixedDelete(ppu_thread& ppu, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.warning("cellSaveDataFixedDelete(setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcDone=*0x%x, container=0x%x, userdata=*0x%x)",
		setList, setBuf, funcFixed, funcDone, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_FIXED_DELETE, 0, vm::null, 1, setList, setBuf, vm::null, funcFixed, vm::null, vm::null, container, 2, userdata, 0, funcDone);
}

error_code cellSaveDataUserListSave(ppu_thread& ppu, u32 version, u32 userId, PSetList setList, PSetBuf setBuf, PFuncList funcList, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.error("cellSaveDataUserListSave(version=%d, userId=%d, setList=*0x%x, setBuf=*0x%x, funcList=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, setList, setBuf, funcList, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_LIST_SAVE, version, vm::null, 0, setList, setBuf, funcList, vm::null, funcStat, funcFile, container, 6, userdata, userId, vm::null);
}

error_code cellSaveDataUserListLoad(ppu_thread& ppu, u32 version, u32 userId, PSetList setList, PSetBuf setBuf, PFuncList funcList, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.error("cellSaveDataUserListLoad(version=%d, userId=%d, setList=*0x%x, setBuf=*0x%x, funcList=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, setList, setBuf, funcList, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_LIST_LOAD, version, vm::null, 0, setList, setBuf, funcList, vm::null, funcStat, funcFile, container, 6, userdata, userId, vm::null);
}

error_code cellSaveDataUserFixedSave(ppu_thread& ppu, u32 version, u32 userId, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.error("cellSaveDataUserFixedSave(version=%d, userId=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_FIXED_SAVE, version, vm::null, 0, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 6, userdata, userId, vm::null);
}

error_code cellSaveDataUserFixedLoad(ppu_thread& ppu, u32 version, u32 userId, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.error("cellSaveDataUserFixedLoad(version=%d, userId=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_FIXED_LOAD, version, vm::null, 0, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 6, userdata, userId, vm::null);
}

error_code cellSaveDataUserAutoSave(ppu_thread& ppu, u32 version, u32 userId, vm::cptr<char> dirName, u32 errDialog, PSetBuf setBuf, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.error("cellSaveDataUserAutoSave(version=%d, userId=%d, dirName=%s, errDialog=%d, setBuf=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, dirName, errDialog, setBuf, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_AUTO_SAVE, version, dirName, errDialog, vm::null, setBuf, vm::null, vm::null, funcStat, funcFile, container, 6, userdata, userId, vm::null);
}

error_code cellSaveDataUserAutoLoad(ppu_thread& ppu, u32 version, u32 userId, vm::cptr<char> dirName, u32 errDialog, PSetBuf setBuf, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.error("cellSaveDataUserAutoLoad(version=%d, userId=%d, dirName=%s, errDialog=%d, setBuf=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, dirName, errDialog, setBuf, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_AUTO_LOAD, version, dirName, errDialog, vm::null, setBuf, vm::null, vm::null, funcStat, funcFile, container, 6, userdata, userId, vm::null);
}

error_code cellSaveDataUserListAutoSave(ppu_thread& ppu, u32 version, u32 userId, u32 errDialog, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.error("cellSaveDataUserListAutoSave(version=%d, userId=%d, errDialog=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, errDialog, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_LIST_AUTO_SAVE, version, vm::null, errDialog, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 6, userdata, userId, vm::null);
}

error_code cellSaveDataUserListAutoLoad(ppu_thread& ppu, u32 version, u32 userId, u32 errDialog, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.error("cellSaveDataUserListAutoLoad(version=%d, userId=%d, errDialog=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, errDialog, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_LIST_AUTO_LOAD, version, vm::null, errDialog, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 6, userdata, userId, vm::null);
}

error_code cellSaveDataUserFixedDelete(ppu_thread& ppu, u32 userId, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.error("cellSaveDataUserFixedDelete(userId=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcDone=*0x%x, container=0x%x, userdata=*0x%x)",
		userId, setList, setBuf, funcFixed, funcDone, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_FIXED_DELETE, 0, vm::null, 1, setList, setBuf, vm::null, funcFixed, vm::null, vm::null, container, 6, userdata, userId, funcDone);
}

void cellSaveDataEnableOverlay(s32 enable)
{
	cellSaveData.error("cellSaveDataEnableOverlay(enable=%d)", enable);
}


// Functions (Extensions)
error_code cellSaveDataListDelete(ppu_thread& ppu, PSetList setList, PSetBuf setBuf, PFuncList funcList, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.warning("cellSaveDataListDelete(setList=*0x%x, setBuf=*0x%x, funcList=*0x%x, funcDone=*0x%x, container=0x%x, userdata=*0x%x)", setList, setBuf, funcList, funcDone, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_LIST_DELETE, 0, vm::null, 0, setList, setBuf, funcList, vm::null, vm::null, vm::null, container, 0x40, userdata, 0, funcDone);
}

error_code cellSaveDataListImport(ppu_thread& ppu, PSetList setList, u32 maxSizeKB, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSaveData);

	return CELL_OK;
}

error_code cellSaveDataListExport(ppu_thread& ppu, PSetList setList, u32 maxSizeKB, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSaveData);

	return CELL_OK;
}

error_code cellSaveDataFixedImport(ppu_thread& ppu, vm::cptr<char> dirName, u32 maxSizeKB, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSaveData);

	return CELL_OK;
}

error_code cellSaveDataFixedExport(ppu_thread& ppu, vm::cptr<char> dirName, u32 maxSizeKB, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSaveData);

	return CELL_OK;
}

error_code cellSaveDataGetListItem(vm::cptr<char> dirName, vm::ptr<CellSaveDataDirStat> dir, vm::ptr<CellSaveDataSystemFileParam> sysFileParam, vm::ptr<u32> bind, vm::ptr<u32> sizeKB)
{
	cellSaveData.warning("cellSaveDataGetListItem(dirName=%s, dir=*0x%x, sysFileParam=*0x%x, bind=*0x%x, sizeKB=*0x%x)", dirName, dir, sysFileParam, bind, sizeKB);

	return savedata_get_list_item(dirName, dir, sysFileParam, bind, sizeKB, 0);
}

error_code cellSaveDataUserListDelete(ppu_thread& ppu, u32 userId, PSetList setList, PSetBuf setBuf, PFuncList funcList, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.error("cellSaveDataUserListDelete(userId=%d, setList=*0x%x, setBuf=*0x%x, funcList=*0x%x, funcDone=*0x%x, container=0x%x, userdata=*0x%x)", userId, setList, setBuf, funcList, funcDone, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_LIST_DELETE, 0, vm::null, 0, setList, setBuf, funcList, vm::null, vm::null, vm::null, container, 0x40, userdata, userId, funcDone);
}

error_code cellSaveDataUserListImport(ppu_thread& ppu, u32 userId, PSetList setList, u32 maxSizeKB, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSaveData);

	return CELL_OK;
}

error_code cellSaveDataUserListExport(ppu_thread& ppu, u32 userId, PSetList setList, u32 maxSizeKB, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSaveData);

	return CELL_OK;
}

error_code cellSaveDataUserFixedImport(ppu_thread& ppu, u32 userId, vm::cptr<char> dirName, u32 maxSizeKB, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSaveData);

	return CELL_OK;
}

error_code cellSaveDataUserFixedExport(ppu_thread& ppu, u32 userId, vm::cptr<char> dirName, u32 maxSizeKB, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSaveData);

	return CELL_OK;
}

error_code cellSaveDataUserGetListItem(u32 userId, vm::cptr<char> dirName, vm::ptr<CellSaveDataDirStat> dir, vm::ptr<CellSaveDataSystemFileParam> sysFileParam, vm::ptr<u32> bind, vm::ptr<u32> sizeKB)
{
	cellSaveData.warning("cellSaveDataUserGetListItem(dirName=%s, dir=*0x%x, sysFileParam=*0x%x, bind=*0x%x, sizeKB=*0x%x, userID=*0x%x)", dirName, dir, sysFileParam, bind, sizeKB, userId);

	return savedata_get_list_item(dirName, dir, sysFileParam, bind, sizeKB, userId);
}

void cellSysutil_SaveData_init()
{
	REG_VAR(cellSysutil, g_savedata_context).flag(MFF_HIDDEN);

	// libsysutil functions:
	REG_FUNC(cellSysutil, cellSaveDataEnableOverlay);

	REG_FUNC(cellSysutil, cellSaveDataDelete2);
	REG_FUNC(cellSysutil, cellSaveDataDelete);
	REG_FUNC(cellSysutil, cellSaveDataUserFixedDelete);
	REG_FUNC(cellSysutil, cellSaveDataFixedDelete);

	REG_FUNC(cellSysutil, cellSaveDataUserFixedLoad);
	REG_FUNC(cellSysutil, cellSaveDataUserFixedSave);
	REG_FUNC(cellSysutil, cellSaveDataFixedLoad2);
	REG_FUNC(cellSysutil, cellSaveDataFixedSave2);
	REG_FUNC(cellSysutil, cellSaveDataFixedLoad);
	REG_FUNC(cellSysutil, cellSaveDataFixedSave);

	REG_FUNC(cellSysutil, cellSaveDataUserListLoad);
	REG_FUNC(cellSysutil, cellSaveDataUserListSave);
	REG_FUNC(cellSysutil, cellSaveDataListLoad2);
	REG_FUNC(cellSysutil, cellSaveDataListSave2);
	REG_FUNC(cellSysutil, cellSaveDataListLoad);
	REG_FUNC(cellSysutil, cellSaveDataListSave);

	REG_FUNC(cellSysutil, cellSaveDataUserListAutoLoad);
	REG_FUNC(cellSysutil, cellSaveDataUserListAutoSave);
	REG_FUNC(cellSysutil, cellSaveDataListAutoLoad);
	REG_FUNC(cellSysutil, cellSaveDataListAutoSave);

	REG_FUNC(cellSysutil, cellSaveDataUserAutoLoad);
	REG_FUNC(cellSysutil, cellSaveDataUserAutoSave);
	REG_FUNC(cellSysutil, cellSaveDataAutoLoad2);
	REG_FUNC(cellSysutil, cellSaveDataAutoSave2);
	REG_FUNC(cellSysutil, cellSaveDataAutoLoad);
	REG_FUNC(cellSysutil, cellSaveDataAutoSave);
}

DECLARE(ppu_module_manager::cellSaveData)("cellSaveData", []()
{
	// libsysutil_savedata functions:
	REG_FUNC(cellSaveData, cellSaveDataUserGetListItem);
	REG_FUNC(cellSaveData, cellSaveDataGetListItem);
	REG_FUNC(cellSaveData, cellSaveDataUserListDelete);
	REG_FUNC(cellSaveData, cellSaveDataListDelete);
	REG_FUNC(cellSaveData, cellSaveDataUserFixedExport);
	REG_FUNC(cellSaveData, cellSaveDataUserFixedImport);
	REG_FUNC(cellSaveData, cellSaveDataUserListExport);
	REG_FUNC(cellSaveData, cellSaveDataUserListImport);
	REG_FUNC(cellSaveData, cellSaveDataFixedExport);
	REG_FUNC(cellSaveData, cellSaveDataFixedImport);
	REG_FUNC(cellSaveData, cellSaveDataListExport);
	REG_FUNC(cellSaveData, cellSaveDataListImport);
});

DECLARE(ppu_module_manager::cellMinisSaveData)("cellMinisSaveData", []()
{
	// libsysutil_savedata_psp functions:
	//REG_FUNC(cellMinisSaveData, cellMinisSaveDataDelete); // 0x6eb168b3
	//REG_FUNC(cellMinisSaveData, cellMinisSaveDataListDelete); // 0xe63eb964

	//REG_FUNC(cellMinisSaveData, cellMinisSaveDataFixedLoad); // 0x66515c18
	//REG_FUNC(cellMinisSaveData, cellMinisSaveDataFixedSave); // 0xf3f974b8
	//REG_FUNC(cellMinisSaveData, cellMinisSaveDataListLoad); // 0xba161d45
	//REG_FUNC(cellMinisSaveData, cellMinisSaveDataListSave); // 0xa342a73f
	//REG_FUNC(cellMinisSaveData, cellMinisSaveDataListAutoLoad); // 0x22f2a553
	//REG_FUNC(cellMinisSaveData, cellMinisSaveDataListAutoSave); // 0xa931356e
	//REG_FUNC(cellMinisSaveData, cellMinisSaveDataAutoLoad); // 0xfc3045d9
});
