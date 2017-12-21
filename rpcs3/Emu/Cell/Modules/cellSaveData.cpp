#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/Modules/cellSysutil.h"

#include "cellSaveData.h"

#include "Loader/PSF.h"
#include "Utilities/StrUtil.h"

#include <mutex>
#include <algorithm>

logs::channel cellSaveData("cellSaveData");

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
		CellSaveDataCBResult result;
		CellSaveDataListGet  listGet;
		CellSaveDataListSet  listSet;
		CellSaveDataFixedSet fixedSet;
		CellSaveDataStatGet  statGet;
		CellSaveDataStatSet  statSet;
		CellSaveDataFileGet  fileGet;
		CellSaveDataFileSet  fileSet;
		CellSaveDataDoneGet  doneGet;
	};
}

vm::gvar<savedata_context> g_savedata_context;

std::mutex g_savedata_mutex;

static NEVER_INLINE s32 savedata_op(ppu_thread& ppu, u32 operation, u32 version, vm::cptr<char> dirName,
	u32 errDialog, PSetList setList, PSetBuf setBuf, PFuncList funcList, PFuncFixed funcFixed, PFuncStat funcStat,
	PFuncFile funcFile, u32 container, u32 unknown, vm::ptr<void> userdata, u32 userId, PFuncDone funcDone)
{
	// TODO: check arguments
	std::unique_lock<std::mutex> lock(g_savedata_mutex, std::try_to_lock);

	if (!lock)
	{
		return CELL_SAVEDATA_ERROR_BUSY;
	}

	*g_savedata_context = {};

	vm::ptr<CellSaveDataCBResult> result   = g_savedata_context.ptr(&savedata_context::result);
	vm::ptr<CellSaveDataListGet>  listGet  = g_savedata_context.ptr(&savedata_context::listGet);
	vm::ptr<CellSaveDataListSet>  listSet  = g_savedata_context.ptr(&savedata_context::listSet);
	vm::ptr<CellSaveDataFixedSet> fixedSet = g_savedata_context.ptr(&savedata_context::fixedSet);
	vm::ptr<CellSaveDataStatGet>  statGet  = g_savedata_context.ptr(&savedata_context::statGet);
	vm::ptr<CellSaveDataStatSet>  statSet  = g_savedata_context.ptr(&savedata_context::statSet);
	vm::ptr<CellSaveDataFileGet>  fileGet  = g_savedata_context.ptr(&savedata_context::fileGet);
	vm::ptr<CellSaveDataFileSet>  fileSet  = g_savedata_context.ptr(&savedata_context::fileSet);
	vm::ptr<CellSaveDataDoneGet>  doneGet  = g_savedata_context.ptr(&savedata_context::doneGet);

	//TODO: get current user ID
	// userId(0) = CELL_SYSUTIL_USERID_CURRENT
	// path of the specified user (00000001 by default)
	const std::string base_dir = vfs::get(fmt::format("/dev_hdd0/home/%08u/savedata/", userId ? userId : 1u));

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
			if (!entry.is_directory)
			{
				continue;
			}

			entry.name = vfs::unescape(entry.name);

			for (const auto& prefix : prefix_list)
			{
				if (entry.name.substr(0, prefix.size()) == prefix)
				{
					// Count the amount of matches and the amount of listed directories
					if (listGet->dirListNum++ < setBuf->dirListMax)
					{
						listGet->dirNum++;

						// PSF parameters
						const auto& psf = psf::load_object(fs::file(base_dir + entry.name + "/PARAM.SFO"));

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

						save_entry2.size = 0;

						for (const auto entry2 : fs::dir(base_dir + entry.name))
						{
							save_entry2.size += entry2.size;
						}

						save_entry2.atime = entry.atime;
						save_entry2.mtime = entry.mtime;
						save_entry2.ctime = entry.ctime;
						if (fs::is_file(base_dir + entry.name + "/ICON0.PNG"))
						{
							fs::file icon = fs::file(base_dir + entry.name + "/ICON0.PNG");
							u32 iconSize = icon.size();
							std::vector<uchar> iconData;
							icon.read(iconData, iconSize);
							save_entry2.iconBuf = iconData;
						}
						save_entry2.isNew = false;
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

			if (order > CELL_SAVEDATA_SORTORDER_ASCENT || type > CELL_SAVEDATA_SORTTYPE_SUBTITLE)
			{
				// error
			}

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
			std::memset(dir.reserved, 0, sizeof(dir.reserved));
		}

		s32 selected = -1;
		s32 focused = -1;

		if (funcList)
		{
			// List Callback
			funcList(ppu, result, listGet, listSet);

			if (result->result < 0)
			{
				//TODO: display dialog
				cellSaveData.warning("savedata_op(): funcList returned result=%d.", result->result);
				return CELL_SAVEDATA_ERROR_CBRESULT;
			}

			// if the callback has returned ok, lets return OK.
			// typically used at game launch when no list is actually required.
			// CELL_SAVEDATA_CBRESULT_OK_LAST_NOCONFIRM is only valid for funcFile and funcDone
			if (result->result == CELL_SAVEDATA_CBRESULT_OK_LAST || result->result == CELL_SAVEDATA_CBRESULT_OK_LAST_NOCONFIRM)
			{
				return CELL_OK;
			}

			// Clean save data list
			save_entries.erase(std::remove_if(save_entries.begin(), save_entries.end(), [&listSet](const SaveDataEntry& entry) -> bool
			{
				for (u32 i = 0; i < listSet->fixedListNum; i++)
				{
					if (entry.dirName == listSet->fixedList[i].dirName)
					{
						return false;
					}
				}

				return true;
			}), save_entries.end());

			switch (const u32 pos_type = listSet->focusPosition)
			{
			case CELL_SAVEDATA_FOCUSPOS_DIRNAME:
			{
				for (s32 i = 0; i < save_entries.size(); i++)
				{
					if (save_entries[i].dirName == listSet->focusDirName.get_ptr())
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
				focused = save_entries.size() - 1;
				break;
			}
			case CELL_SAVEDATA_FOCUSPOS_LATEST:
			{
				s64 max = INT64_MIN;

				for (s32 i = 0; i < save_entries.size(); i++)
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

				for (s32 i = 0; i < save_entries.size(); i++)
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
				//TODO: If adding the new data to the save_entries vector
				// to be displayed in the save mangaer UI, it should be focused here
				break;
			}
			default:
			{
				cellSaveData.error("savedata_op(): unknown listSet->focusPosition (0x%x)", pos_type);
				return CELL_SAVEDATA_ERROR_PARAM;
			}
			}
		}

		auto delete_save = [&](const std::string& del_path)
		{
			strcpy_trunc(doneGet->dirName, save_entries[selected].dirName);
			doneGet->hddFreeSizeKB = 40 * 1024 * 1024; // 40 GB
			doneGet->sizeKB        = 0;
			doneGet->excResult     = CELL_OK;
			std::memset(doneGet->reserved, 0, sizeof(doneGet->reserved));

			const fs::dir _dir{del_path};

			for (auto&& file : _dir)
			{
				if (!file.is_directory)
				{
					doneGet->sizeKB += ::align(file.size, 4096);

					if (!fs::remove_file(del_path + file.name))
					{
						doneGet->excResult = CELL_SAVEDATA_ERROR_FAILURE;
					}
				}
			}

			if (!_dir)
			{
				doneGet->excResult = CELL_SAVEDATA_ERROR_NODATA;
			}

			if (!doneGet->excResult && !fs::remove_dir(del_path))
			{
				doneGet->excResult = CELL_SAVEDATA_ERROR_FAILURE;
			}

			funcDone(ppu, result, doneGet);
		};

		while (funcList)
		{
			// Display Save Data List asynchronously in the GUI thread.
			atomic_t<bool> dlg_result(false);

			Emu.CallAfter([&]()
			{
				selected = Emu.GetCallbacks().get_save_dialog()->ShowSaveDataList(save_entries, focused, operation, listSet);
				dlg_result = true;
			});

			while (!dlg_result)
			{
				thread_ctrl::wait_for(1000);
			}

			// UI returns -1 for new save games
			if (selected == -1)
			{
				save_entry.dirName = listSet->newData->dirName.get_ptr();
			}

			// Cancel selected in UI
			if (selected == -2)
			{
				return CELL_CANCEL;
			}

			if (operation == SAVEDATA_OP_LIST_DELETE)
			{
				delete_save(base_dir + save_entries[selected].dirName + '/');

				if (result->result < 0)
				{
					cellSaveData.warning("savedata_op(): funcDone returned result=%d.", result->result);
					return CELL_SAVEDATA_ERROR_CBRESULT;
				}

				if (result->result == CELL_SAVEDATA_CBRESULT_OK_LAST || result->result == CELL_SAVEDATA_CBRESULT_OK_LAST_NOCONFIRM)
				{
					return CELL_OK;
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
			// Fixed Callback
			funcFixed(ppu, result, listGet, fixedSet);

			// skip all following steps if OK_LAST
			if (result->result == CELL_SAVEDATA_CBRESULT_OK_LAST || result->result == CELL_SAVEDATA_CBRESULT_OK_LAST_NOCONFIRM)
			{
				return CELL_OK;
			}

			if (result->result < 0)
			{
				//TODO: Show msgDialog if required
				// depends on fixedSet->option
				// 0 = none
				// 1 = skip confirmation dialog

				cellSaveData.warning("savedata_op(): funcFixed returned result=%d.", result->result);
				return CELL_SAVEDATA_ERROR_CBRESULT;
			}

			if (!fixedSet->dirName)
			{
				return CELL_SAVEDATA_ERROR_PARAM;
			}

			for (s32 i = 0; i < save_entries.size(); i++)
			{
				if (save_entries[i].dirName == fixedSet->dirName.get_ptr())
				{
					selected = i;
					break;
				}
			}

			if (selected == -1)
			{
				save_entry.dirName = fixedSet->dirName.get_ptr();
			}

			if (operation == SAVEDATA_OP_FIXED_DELETE)
			{
				delete_save(base_dir + save_entries[selected].dirName + '/');

				if (result->result < 0)
				{
					cellSaveData.warning("savedata_op(): funcDone_ returned result=%d.", result->result);
					return CELL_SAVEDATA_ERROR_CBRESULT;
				}

				return CELL_OK;
			}
		}

		if (selected >= 0)
		{
			if (selected < save_entries.size())
			{
				save_entry.dirName = std::move(save_entries[selected].dirName);
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
	}

	std::string dir_path = base_dir + save_entry.dirName + "/";
	std::string sfo_path = dir_path + "PARAM.SFO";

	auto&& psf = psf::load_object(fs::file(sfo_path));

	// Get save stats
	{
		fs::stat_t dir_info{};
		if (!fs::stat(dir_path, dir_info))
		{
			// error
		}

		statGet->hddFreeSizeKB = 40 * 1024 * 1024; // 40 GB
		statGet->isNewData = save_entry.isNew = psf.empty();

		statGet->dir.atime = save_entry.atime = dir_info.atime;
		statGet->dir.mtime = save_entry.mtime = dir_info.mtime;
		statGet->dir.ctime = save_entry.ctime = dir_info.ctime;
		strcpy_trunc(statGet->dir.dirName, save_entry.dirName);

		if (!psf.empty())
		{
			statGet->getParam.attribute = psf.at("ATTRIBUTE").as_integer(); // ???
			strcpy_trunc(statGet->getParam.title, save_entry.title = psf.at("TITLE").as_string());
			strcpy_trunc(statGet->getParam.subTitle, save_entry.subtitle = psf.at("SUB_TITLE").as_string());
			strcpy_trunc(statGet->getParam.detail, save_entry.details = psf.at("DETAIL").as_string());
			strcpy_trunc(statGet->getParam.listParam, save_entry.listParam = psf.at("SAVEDATA_LIST_PARAM").as_string());
		}

		statGet->bind = 0;
		statGet->sizeKB = save_entry.size / 1024;
		statGet->sysSizeKB = 0; // This is the size of system files, but PARAM.SFO is very small and PARAM.PDF is not used

		statGet->fileNum = 0;
		statGet->fileList.set(setBuf->buf.addr());
		statGet->fileListNum = 0;
		memset(statGet->reserved, 0, sizeof(statGet->reserved));

		auto file_list = statGet->fileList.get_ptr();

		for (auto&& entry : fs::dir(dir_path))
		{
			entry.name = vfs::unescape(entry.name);

			// only files, system files ignored, fileNum is limited by setBuf->fileListMax
			if (!entry.is_directory && entry.name != "PARAM.SFO" && statGet->fileListNum++ < setBuf->fileListMax)
			{
				statGet->fileNum++;

				auto& file = *file_list++;

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

				file.size = entry.size;
				file.atime = entry.atime;
				file.mtime = entry.mtime;
				file.ctime = entry.ctime;
				strcpy_trunc(file.fileName, entry.name);
			}
		}

		// Stat Callback
		funcStat(ppu, result, statGet, statSet);

		if (result->result < 0)
		{
			cellSaveData.warning("savedata_op(): funcStat returned result=%d.", result->result);
			return CELL_SAVEDATA_ERROR_CBRESULT;
		}

		if (result->result == CELL_SAVEDATA_CBRESULT_OK_LAST || result->result == CELL_SAVEDATA_CBRESULT_OK_LAST_NOCONFIRM)
		{
			return CELL_OK;
		}

		if (statSet->setParam)
		{
			// Update PARAM.SFO
			psf.clear();
			psf.insert(
			{
				{ "ACCOUNT_ID", psf::array(16, "0000000000000000") }, // ???
				{ "ATTRIBUTE", statSet->setParam->attribute.value() },
				{ "CATEGORY",  psf::string(4, "SD") }, // ???
				{ "PARAMS", psf::string(16, {}) }, // ???
				{ "PARAMS2", psf::string(16, {}) }, // ???
				{ "PARENTAL_LEVEL", 0 }, // ???
				{ "DETAIL", psf::string(1024, statSet->setParam->detail) },
				{ "SAVEDATA_DIRECTORY", psf::string(256, save_entry.dirName) },
				{ "SAVEDATA_LIST_PARAM", psf::string(8, statSet->setParam->listParam) },
				{ "SUB_TITLE", psf::string(128, statSet->setParam->subTitle) },
				{ "TITLE", psf::string(128, statSet->setParam->title) },
			});
		}
		//else if (psf.empty())
		//{
		//	// setParam is specified if something required updating.
		//	// Do not exit. Recreate mode will handle the rest
		//	//return CELL_OK;
		//}

		switch (const u32 mode = statSet->reCreateMode & 0xffff)
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

			// TODO: Only delete data, not owner info
			for (const auto& entry : fs::dir(dir_path))
			{
				if (!entry.is_directory)
				{
					fs::remove_file(dir_path + entry.name);
				}
			}

			//TODO: probably not deleting owner info
			if (!statSet->setParam)
			{
				// Savedata deleted and setParam is NULL: delete directory and abort operation
				if (fs::remove_dir(dir_path)) cellSaveData.error("savedata_op(): savedata directory %s deleted", save_entry.dirName);

				//return CELL_OK;
			}

			break;
		}

		default:
		{
			cellSaveData.error("savedata_op(): unknown statSet->reCreateMode (0x%x)", statSet->reCreateMode);
			return CELL_SAVEDATA_ERROR_PARAM;
		}
		}
	}



	// Create save directory if necessary
	if (psf.size() && save_entry.isNew && !fs::create_dir(dir_path))
	{
		cellSaveData.warning("savedata_op(): failed to create %s", dir_path);
		return CELL_SAVEDATA_ERROR_ACCESS_ERROR;
	}

	// Enter the loop where the save files are read/created/deleted

	fileGet->excSize = 0;
	memset(fileGet->reserved, 0, sizeof(fileGet->reserved));

	while (funcFile)
	{
		funcFile(ppu, result, fileGet, fileSet);

		if (result->result < 0)
		{
			cellSaveData.warning("savedata_op(): funcFile returned result=%d.", result->result);
			return CELL_SAVEDATA_ERROR_CBRESULT;
		}

		if (result->result == CELL_SAVEDATA_CBRESULT_OK_LAST || result->result == CELL_SAVEDATA_CBRESULT_OK_LAST_NOCONFIRM)
		{
			//todo: display user prompt
			break;
		}

		//TODO: Show progress
		// if it's not an auto load/save

		std::string file_path;

		switch (const u32 type = fileSet->fileType)
		{
		case CELL_SAVEDATA_FILETYPE_SECUREFILE:
		case CELL_SAVEDATA_FILETYPE_NORMALFILE:
		{
			file_path = fileSet->fileName.get_ptr();
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
			cellSaveData.error("savedata_op(): unknown fileSet->fileType (0x%x)", type);
			return CELL_SAVEDATA_ERROR_PARAM;
		}
		}

		psf.emplace("*" + file_path, fileSet->fileType == CELL_SAVEDATA_FILETYPE_SECUREFILE);

		switch (const u32 op = fileSet->fileOperation)
		{
		case CELL_SAVEDATA_FILEOP_READ:
		{
			fs::file file(dir_path + file_path, fs::read);
			if (!file)
			{
				cellSaveData.error("Failed to open file %s%s", dir_path, file_path);
				return CELL_SAVEDATA_ERROR_FAILURE;
			}

			file.seek(fileSet->fileOffset);
			std::vector<uchar> buf;
			buf.resize(std::min<u32>(fileSet->fileSize, fileSet->fileBufSize));
			buf.resize(file.read(buf.data(), buf.size()));
			std::memcpy(fileSet->fileBuf.get_ptr(), buf.data(), buf.size());
			fileGet->excSize = ::size32(buf);
			break;
		}

		case CELL_SAVEDATA_FILEOP_WRITE:
		{
			fs::file file(dir_path + file_path, fs::write + fs::create);
			file.seek(fileSet->fileOffset);
			const auto start = static_cast<uchar*>(fileSet->fileBuf.get_ptr());
			std::vector<uchar> buf(start, start + std::min<u32>(fileSet->fileSize, fileSet->fileBufSize));
			fileGet->excSize = ::narrow<u32>(file.write(buf.data(), buf.size()));
			file.trunc(file.pos()); // truncate
			break;
		}

		case CELL_SAVEDATA_FILEOP_DELETE:
		{
			fs::remove_file(dir_path + file_path);
			fileGet->excSize = 0;
			break;
		}

		case CELL_SAVEDATA_FILEOP_WRITE_NOTRUNC:
		{
			fs::file file(dir_path + file_path, fs::write + fs::create);
			file.seek(fileSet->fileOffset);
			const auto start = static_cast<uchar*>(fileSet->fileBuf.get_ptr());
			std::vector<uchar> buf(start, start + std::min<u32>(fileSet->fileSize, fileSet->fileBufSize));
			fileGet->excSize = ::narrow<u32>(file.write(buf.data(), buf.size()));
			break;
		}

		default:
		{
			cellSaveData.error("savedata_op(): unknown fileSet->fileOperation (0x%x)", op);
			return CELL_SAVEDATA_ERROR_PARAM;
		}
		}
	}

	// Write PARAM.SFO
	if (psf.size())
	{
		psf::save_object(fs::file(sfo_path, fs::rewrite), psf);
	}

	return CELL_OK;
}

static NEVER_INLINE s32 savedata_get_list_item(vm::cptr<char> dirName, vm::ptr<CellSaveDataDirStat> dir, vm::ptr<CellSaveDataSystemFileParam> sysFileParam, vm::ptr<u32> bind, vm::ptr<u32> sizeKB, u32 userId)
{
	//TODO: accurately get the current user
	if (userId == 0)
	{
		userId = 1u;
	}
	std::string save_path = vfs::get(fmt::format("/dev_hdd0/home/%08u/savedata/%s/", userId, dirName.get_ptr()));
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
		strcpy_trunc(dir->dirName, dirName.get_ptr());
		dir->atime = dir_info.atime;
		dir->ctime = dir_info.ctime;
		dir->mtime = dir_info.mtime;
	}

	if (bind)
	{
		//TODO: Set bind in accordance to any problems
		*bind = 0;
	}

	return CELL_OK;
}

// Functions
s32 cellSaveDataListSave2(ppu_thread& ppu, u32 version, PSetList setList, PSetBuf setBuf, PFuncList funcList,
	PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.warning("cellSaveDataListSave2(version=%d, setList=*0x%x, setBuf=*0x%x, funcList=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, setList, setBuf, funcList, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_LIST_SAVE, version, vm::null, 1, setList, setBuf, funcList, vm::null, funcStat, funcFile, container, 2, userdata, 0, vm::null);
}

s32 cellSaveDataListLoad2(ppu_thread& ppu, u32 version, PSetList setList, PSetBuf setBuf, PFuncList funcList,
	PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.warning("cellSaveDataListLoad2(version=%d, setList=*0x%x, setBuf=*0x%x, funcList=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, setList, setBuf, funcList, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_LIST_LOAD, version, vm::null, 1, setList, setBuf, funcList, vm::null, funcStat, funcFile, container, 2, userdata, 0, vm::null);
}

s32 cellSaveDataListSave(ppu_thread& ppu, u32 version, PSetList setList, PSetBuf setBuf, PFuncList funcList,
	PFuncStat funcStat, PFuncFile funcFile, u32 container)
{
	cellSaveData.warning("cellSaveDataListSave(version=%d, setList=*0x%x, setBuf=*0x%x, funcList=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x)",
		version, setList, setBuf, funcList, funcStat, funcFile, container);

	return savedata_op(ppu, SAVEDATA_OP_LIST_SAVE, version, vm::null, 1, setList, setBuf, funcList, vm::null, funcStat, funcFile, container, 2, vm::null, 0, vm::null);
}

s32 cellSaveDataListLoad(ppu_thread& ppu, u32 version, PSetList setList, PSetBuf setBuf, PFuncList funcList,
	PFuncStat funcStat, PFuncFile funcFile, u32 container)
{
	cellSaveData.warning("cellSaveDataListLoad(version=%d, setList=*0x%x, setBuf=*0x%x, funcList=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x)",
		version, setList, setBuf, funcList, funcStat, funcFile, container);

	return savedata_op(ppu, SAVEDATA_OP_LIST_LOAD, version, vm::null, 1, setList, setBuf, funcList, vm::null, funcStat, funcFile, container, 2, vm::null, 0, vm::null);
}

s32 cellSaveDataFixedSave2(ppu_thread& ppu, u32 version, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed,
	PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.warning("cellSaveDataFixedSave2(version=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_FIXED_SAVE, version, vm::null, 1, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 2, userdata, 0, vm::null);
}

s32 cellSaveDataFixedLoad2(ppu_thread& ppu, u32 version, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed,
	PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.warning("cellSaveDataFixedLoad2(version=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_FIXED_LOAD, version, vm::null, 1, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 2, userdata, 0, vm::null);
}

s32 cellSaveDataFixedSave(ppu_thread& ppu, u32 version, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed,
	PFuncStat funcStat, PFuncFile funcFile, u32 container)
{
	cellSaveData.warning("cellSaveDataFixedSave(version=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x)",
		version, setList, setBuf, funcFixed, funcStat, funcFile, container);

	return savedata_op(ppu, SAVEDATA_OP_FIXED_SAVE, version, vm::null, 1, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 2, vm::null, 0, vm::null);
}

s32 cellSaveDataFixedLoad(ppu_thread& ppu, u32 version, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed,
	PFuncStat funcStat, PFuncFile funcFile, u32 container)
{
	cellSaveData.warning("cellSaveDataFixedLoad(version=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x)",
		version, setList, setBuf, funcFixed, funcStat, funcFile, container);

	return savedata_op(ppu, SAVEDATA_OP_FIXED_LOAD, version, vm::null, 1, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 2, vm::null, 0, vm::null);
}

s32 cellSaveDataAutoSave2(ppu_thread& ppu, u32 version, vm::cptr<char> dirName, u32 errDialog, PSetBuf setBuf,
	PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.warning("cellSaveDataAutoSave2(version=%d, dirName=%s, errDialog=%d, setBuf=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, dirName, errDialog, setBuf, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_AUTO_SAVE, version, dirName, errDialog, vm::null, setBuf, vm::null, vm::null, funcStat, funcFile, container, 2, userdata, 0, vm::null);
}

s32 cellSaveDataAutoLoad2(ppu_thread& ppu, u32 version, vm::cptr<char> dirName, u32 errDialog, PSetBuf setBuf,
	PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.warning("cellSaveDataAutoLoad2(version=%d, dirName=%s, errDialog=%d, setBuf=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, dirName, errDialog, setBuf, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_AUTO_LOAD, version, dirName, errDialog, vm::null, setBuf, vm::null, vm::null, funcStat, funcFile, container, 2, userdata, 0, vm::null);
}

s32 cellSaveDataAutoSave(ppu_thread& ppu, u32 version, vm::cptr<char> dirName, u32 errDialog, PSetBuf setBuf,
	PFuncStat funcStat, PFuncFile funcFile, u32 container)
{
	cellSaveData.warning("cellSaveDataAutoSave(version=%d, dirName=%s, errDialog=%d, setBuf=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x)",
		version, dirName, errDialog, setBuf, funcStat, funcFile, container);

	return savedata_op(ppu, SAVEDATA_OP_AUTO_SAVE, version, dirName, errDialog, vm::null, setBuf, vm::null, vm::null, funcStat, funcFile, container, 2, vm::null, 0, vm::null);
}

s32 cellSaveDataAutoLoad(ppu_thread& ppu, u32 version, vm::cptr<char> dirName, u32 errDialog, PSetBuf setBuf,
	PFuncStat funcStat, PFuncFile funcFile, u32 container)
{
	cellSaveData.warning("cellSaveDataAutoLoad(version=%d, dirName=%s, errDialog=%d, setBuf=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x)",
		version, dirName, errDialog, setBuf, funcStat, funcFile, container);

	return savedata_op(ppu, SAVEDATA_OP_AUTO_LOAD, version, dirName, errDialog, vm::null, setBuf, vm::null, vm::null, funcStat, funcFile, container, 2, vm::null, 0, vm::null);
}

s32 cellSaveDataListAutoSave(ppu_thread& ppu, u32 version, u32 errDialog, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.warning("cellSaveDataListAutoSave(version=%d, errDialog=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, errDialog, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_LIST_AUTO_SAVE, version, vm::null, errDialog, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 0, userdata, 0, vm::null);
}

s32 cellSaveDataListAutoLoad(ppu_thread& ppu, u32 version, u32 errDialog, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.warning("cellSaveDataListAutoLoad(version=%d, errDialog=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, errDialog, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_LIST_AUTO_LOAD, version, vm::null, errDialog, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 0, userdata, 0, vm::null);
}

s32 cellSaveDataDelete2(u32 container)
{
	cellSaveData.todo("cellSaveDataDelete2(container=0x%x)", container);

	return CELL_CANCEL;
}

s32 cellSaveDataDelete(u32 container)
{
	cellSaveData.todo("cellSaveDataDelete(container=0x%x)", container);

	return CELL_CANCEL;
}

s32 cellSaveDataFixedDelete(ppu_thread& ppu, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.warning("cellSaveDataFixedDelete(setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcDone=*0x%x, container=0x%x, userdata=*0x%x)",
		setList, setBuf, funcFixed, funcDone, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_FIXED_DELETE, 0, vm::null, 1, setList, setBuf, vm::null, funcFixed, vm::null, vm::null, container, 2, userdata, 0, funcDone);
}

s32 cellSaveDataUserListSave(ppu_thread& ppu, u32 version, u32 userId, PSetList setList, PSetBuf setBuf, PFuncList funcList, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.error("cellSaveDataUserListSave(version=%d, userId=%d, setList=*0x%x, setBuf=*0x%x, funcList=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, setList, setBuf, funcList, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_LIST_SAVE, version, vm::null, 0, setList, setBuf, funcList, vm::null, funcStat, funcFile, container, 6, userdata, userId, vm::null);
}

s32 cellSaveDataUserListLoad(ppu_thread& ppu, u32 version, u32 userId, PSetList setList, PSetBuf setBuf, PFuncList funcList, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.error("cellSaveDataUserListLoad(version=%d, userId=%d, setList=*0x%x, setBuf=*0x%x, funcList=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, setList, setBuf, funcList, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_LIST_LOAD, version, vm::null, 0, setList, setBuf, funcList, vm::null, funcStat, funcFile, container, 6, userdata, userId, vm::null);
}

s32 cellSaveDataUserFixedSave(ppu_thread& ppu, u32 version, u32 userId, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.error("cellSaveDataUserFixedSave(version=%d, userId=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_FIXED_SAVE, version, vm::null, 0, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 6, userdata, userId, vm::null);
}

s32 cellSaveDataUserFixedLoad(ppu_thread& ppu, u32 version, u32 userId, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.error("cellSaveDataUserFixedLoad(version=%d, userId=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_FIXED_LOAD, version, vm::null, 0, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 6, userdata, userId, vm::null);
}

s32 cellSaveDataUserAutoSave(ppu_thread& ppu, u32 version, u32 userId, vm::cptr<char> dirName, u32 errDialog, PSetBuf setBuf, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.error("cellSaveDataUserAutoSave(version=%d, userId=%d, dirName=%s, errDialog=%d, setBuf=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, dirName, errDialog, setBuf, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_AUTO_SAVE, version, dirName, errDialog, vm::null, setBuf, vm::null, vm::null, funcStat, funcFile, container, 6, userdata, userId, vm::null);
}

s32 cellSaveDataUserAutoLoad(ppu_thread& ppu, u32 version, u32 userId, vm::cptr<char> dirName, u32 errDialog, PSetBuf setBuf, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.error("cellSaveDataUserAutoLoad(version=%d, userId=%d, dirName=%s, errDialog=%d, setBuf=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, dirName, errDialog, setBuf, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_AUTO_LOAD, version, dirName, errDialog, vm::null, setBuf, vm::null, vm::null, funcStat, funcFile, container, 6, userdata, userId, vm::null);
}

s32 cellSaveDataUserListAutoSave(ppu_thread& ppu, u32 version, u32 userId, u32 errDialog, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.error("cellSaveDataUserListAutoSave(version=%d, userId=%d, errDialog=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, errDialog, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_LIST_AUTO_SAVE, version, vm::null, errDialog, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 6, userdata, userId, vm::null);
}

s32 cellSaveDataUserListAutoLoad(ppu_thread& ppu, u32 version, u32 userId, u32 errDialog, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.error("cellSaveDataUserListAutoLoad(version=%d, userId=%d, errDialog=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, errDialog, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_LIST_AUTO_LOAD, version, vm::null, errDialog, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 6, userdata, userId, vm::null);
}

s32 cellSaveDataUserFixedDelete(ppu_thread& ppu, u32 userId, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
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
s32 cellSaveDataListDelete(ppu_thread& ppu, PSetList setList, PSetBuf setBuf, PFuncList funcList, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.warning("cellSaveDataListDelete(setList=*0x%x, setBuf=*0x%x, funcList=*0x%x, funcDone=*0x%x, container=0x%x, userdata=*0x%x)", setList, setBuf, funcList, funcDone, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_LIST_DELETE, 0, vm::null, 0, setList, setBuf, funcList, vm::null, vm::null, vm::null, container, 0x40, userdata, 0, funcDone);
}

s32 cellSaveDataListImport(ppu_thread& ppu, PSetList setList, u32 maxSizeKB, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSaveData);

	return CELL_OK;
}

s32 cellSaveDataListExport(ppu_thread& ppu, PSetList setList, u32 maxSizeKB, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSaveData);

	return CELL_OK;
}

s32 cellSaveDataFixedImport(ppu_thread& ppu, vm::cptr<char> dirName, u32 maxSizeKB, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSaveData);

	return CELL_OK;
}

s32 cellSaveDataFixedExport(ppu_thread& ppu, vm::cptr<char> dirName, u32 maxSizeKB, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSaveData);

	return CELL_OK;
}

s32 cellSaveDataGetListItem(vm::cptr<char> dirName, vm::ptr<CellSaveDataDirStat> dir, vm::ptr<CellSaveDataSystemFileParam> sysFileParam, vm::ptr<u32> bind, vm::ptr<u32> sizeKB)
{
	cellSaveData.warning("cellSavaDataGetListItem(dirName=%s, dir=*0x%x, sysFileParam=*0x%x, bind=*0x%x, sizeKB=*0x%x)", dirName, dir, sysFileParam, bind, sizeKB);

	return savedata_get_list_item(dirName, dir, sysFileParam, bind, sizeKB, 0);
}

s32 cellSaveDataUserListDelete(ppu_thread& ppu, u32 userId, PSetList setList, PSetBuf setBuf, PFuncList funcList, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	cellSaveData.error("cellSaveDataUserListDelete(userId=%d, setList=*0x%x, setBuf=*0x%x, funcList=*0x%x, funcDone=*0x%x, container=0x%x, userdata=*0x%x)", userId, setList, setBuf, funcList, funcDone, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_LIST_DELETE, 0, vm::null, 0, setList, setBuf, funcList, vm::null, vm::null, vm::null, container, 0x40, userdata, userId, funcDone);
}

s32 cellSaveDataUserListImport(ppu_thread& ppu, u32 userId, PSetList setList, u32 maxSizeKB, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSaveData);

	return CELL_OK;
}

s32 cellSaveDataUserListExport(ppu_thread& ppu, u32 userId, PSetList setList, u32 maxSizeKB, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSaveData);

	return CELL_OK;
}

s32 cellSaveDataUserFixedImport(ppu_thread& ppu, u32 userId, vm::cptr<char> dirName, u32 maxSizeKB, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSaveData);

	return CELL_OK;
}

s32 cellSaveDataUserFixedExport(ppu_thread& ppu, u32 userId, vm::cptr<char> dirName, u32 maxSizeKB, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSaveData);

	return CELL_OK;
}

s32 cellSaveDataUserGetListItem(u32 userId, vm::cptr<char> dirName, vm::ptr<CellSaveDataDirStat> dir, vm::ptr<CellSaveDataSystemFileParam> sysFileParam, vm::ptr<u32> bind, vm::ptr<u32> sizeKB)
{
	cellSaveData.warning("cellSavaDataGetListItem(dirName=%s, dir=*0x%x, sysFileParam=*0x%x, bind=*0x%x, sizeKB=*0x%x, userID=*0x%x)", dirName, dir, sysFileParam, bind, sizeKB, userId);

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
