#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "Emu/FS/VFS.h"
#include "Emu/FS/vfsFile.h"
#include "Emu/FS/vfsDir.h"
#include "Utilities/File.h"
#include "Loader/PSF.h"
#include "cellSaveData.h"

extern Module cellSysutil;

std::unique_ptr<SaveDataDialogInstance> g_savedata_dialog;

SaveDataDialogInstance::SaveDataDialogInstance()
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
	SAVEDATA_OP_AUTO_SAVE = 0,
	SAVEDATA_OP_AUTO_LOAD = 1,
	SAVEDATA_OP_LIST_AUTO_SAVE = 2,
	SAVEDATA_OP_LIST_AUTO_LOAD = 3,
	SAVEDATA_OP_LIST_SAVE = 4,
	SAVEDATA_OP_LIST_LOAD = 5,
	SAVEDATA_OP_FIXED_SAVE = 6,
	SAVEDATA_OP_FIXED_LOAD = 7,

	SAVEDATA_OP_FIXED_DELETE = 14,
};

never_inline s32 savedata_op(PPUThread& ppu, u32 operation, u32 version, vm::cptr<char> dirName, u32 errDialog, PSetList setList, PSetBuf setBuf, PFuncList funcList, PFuncFixed funcFixed, PFuncStat funcStat, PFuncFile funcFile, u32 container, u32 unknown, vm::ptr<void> userdata, u32 userId, PFuncDone funcDone)
{
	// TODO: check arguments

	// try to lock the mutex (not sure how it originally works; std::try_to_lock makes it non-blocking)
	std::unique_lock<std::mutex> lock(g_savedata_dialog->mutex, std::try_to_lock);

	if (!lock)
	{
		return CELL_SAVEDATA_ERROR_BUSY;
	}

	// path of the specified user (00000001 by default)
	const std::string base_dir = fmt::format("/dev_hdd0/home/%08d/savedata/", userId ? userId : 1u);

	vm::stackvar<CellSaveDataCBResult> result(ppu);

	result->userdata = userdata; // probably should be assigned only once (allows the callback to change it)

	SaveDataEntry save_entry;

	if (setList)
	{
		std::vector<SaveDataEntry> save_entries;

		vm::stackvar<CellSaveDataListGet> listGet(ppu);

		listGet->dirNum = 0;
		listGet->dirListNum = 0;
		listGet->dirList.set(setBuf->buf.addr());
		memset(listGet->reserved, 0, sizeof(listGet->reserved));

		const auto prefix_list = fmt::split(setList->dirNamePrefix.get_ptr(), { "|" });

		for (const auto entry : vfsDir(base_dir))
		{
			if (entry->flags & DirEntry_TypeFile)
			{
				continue;
			}

			for (const auto& prefix : prefix_list)
			{
				if (entry->name.substr(0, prefix.size()) == prefix)
				{
					// Count the amount of matches and the amount of listed directories
					if (listGet->dirListNum++ < setBuf->dirListMax)
					{
						listGet->dirNum++;

						// PSF parameters
						vfsFile f(base_dir + entry->name + "/PARAM.SFO");
						const PSFLoader psf(f);

						if (!psf)
						{
							break;
						}

						SaveDataEntry save_entry2;
						save_entry2.dirName = psf.GetString("SAVEDATA_DIRECTORY");
						save_entry2.listParam = psf.GetString("SAVEDATA_LIST_PARAM");
						save_entry2.title = psf.GetString("TITLE");
						save_entry2.subtitle = psf.GetString("SUB_TITLE");
						save_entry2.details = psf.GetString("DETAIL");

						save_entry2.size = 0;

						for (const auto entry2 : vfsDir(base_dir + entry->name))
						{
							save_entry2.size += entry2->size;
						}

						save_entry2.atime = entry->access_time;
						save_entry2.mtime = entry->modify_time;
						save_entry2.ctime = entry->create_time;
						//save_entry2.iconBuf = NULL; // TODO: Here should be the PNG buffer
						//save_entry2.iconBufSize = 0; // TODO: Size of the PNG file
						save_entry2.isNew = false;

						save_entries.push_back(save_entry2);
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
			memset(dir.reserved, 0, sizeof(dir.reserved));
		}

		s32 selected = -1;

		if (funcList)
		{
			vm::stackvar<CellSaveDataListSet> listSet(ppu);

			// List Callback
			funcList(ppu, result, listGet, listSet);

			if (result->result < 0)
			{
				cellSysutil.Warning("savedata_op(): funcList returned < 0.");
				return CELL_SAVEDATA_ERROR_CBRESULT;
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

			// Focus save data
			s32 focused = -1;

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
				break;
			}
			default:
			{
				cellSysutil.Error("savedata_op(): unknown listSet->focusPosition (0x%x)", pos_type);
				return CELL_SAVEDATA_ERROR_PARAM;
			}
			}

			// Display Save Data List
			selected = g_savedata_dialog->ShowSaveDataList(save_entries, focused, listSet);

			if (selected == -1)
			{
				if (listSet->newData)
				{
					save_entry.dirName = listSet->newData->dirName.get_ptr();
				}
				else
				{
					return CELL_OK; // ???
				}
			}
		}

		if (funcFixed)
		{
			vm::stackvar<CellSaveDataFixedSet> fixedSet(ppu);

			// Fixed Callback
			funcFixed(ppu, result, listGet, fixedSet);

			if (result->result < 0)
			{
				cellSysutil.Warning("savedata_op(): funcFixed returned < 0.");
				return CELL_SAVEDATA_ERROR_CBRESULT;
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
		}

		if (selected >= 0)
		{
			if (selected < save_entries.size())
			{
				save_entry.dirName = std::move(save_entries[selected].dirName);
			}
			else
			{
				throw EXCEPTION("Invalid savedata selected");
			}
		}
	}

	if (dirName)
	{
		save_entry.dirName = dirName.get_ptr();
	}

	std::string dir_path = base_dir + save_entry.dirName + "/";
	std::string sfo_path = dir_path + "PARAM.SFO";

	PSFLoader psf;

	// Load PARAM.SFO
	{
		vfsFile f(sfo_path);
		psf.Load(f);
	}

	// Get save stats
	{
		vm::stackvar<CellSaveDataStatGet> statGet(ppu);
		vm::stackvar<CellSaveDataStatSet> statSet(ppu);

		std::string dir_local_path;

		Emu.GetVFS().GetDevice(dir_path, dir_local_path);

		fs::stat_t dir_info;
		if (!fs::stat(dir_local_path, dir_info))
		{
			// error
		}

		statGet->hddFreeSizeKB = 40 * 1024 * 1024; // 40 GB
		statGet->isNewData = save_entry.isNew = !psf;

		statGet->dir.atime = save_entry.atime = dir_info.atime;
		statGet->dir.mtime = save_entry.mtime = dir_info.mtime;
		statGet->dir.ctime = save_entry.ctime = dir_info.ctime;
		strcpy_trunc(statGet->dir.dirName, save_entry.dirName);

		statGet->getParam.attribute = psf.GetInteger("ATTRIBUTE"); // ???
		strcpy_trunc(statGet->getParam.title, save_entry.title = psf.GetString("TITLE"));
		strcpy_trunc(statGet->getParam.subTitle, save_entry.subtitle = psf.GetString("SUB_TITLE"));
		strcpy_trunc(statGet->getParam.detail, save_entry.details = psf.GetString("DETAIL"));
		strcpy_trunc(statGet->getParam.listParam, save_entry.listParam = psf.GetString("SAVEDATA_LIST_PARAM"));

		statGet->bind = 0;
		statGet->sizeKB = save_entry.size / 1024;
		statGet->sysSizeKB = 0; // This is the size of system files, but PARAM.SFO is very small and PARAM.PDF is not used

		statGet->fileNum = 0;
		statGet->fileList.set(setBuf->buf.addr());
		statGet->fileListNum = 0;
		memset(statGet->reserved, 0, sizeof(statGet->reserved));

		auto file_list = statGet->fileList.get_ptr();

		for (const auto entry : vfsDir(dir_path))
		{
			// only files, system files ignored, fileNum is limited by setBuf->fileListMax
			if (entry->flags & DirEntry_TypeFile && entry->name != "PARAM.SFO" && statGet->fileListNum++ < setBuf->fileListMax)
			{
				statGet->fileNum++;

				auto& file = *file_list++;

				if (entry->name == "ICON0.PNG")
				{
					file.fileType = CELL_SAVEDATA_FILETYPE_CONTENT_ICON0;
				}
				else if (entry->name == "ICON1.PAM")
				{
					file.fileType = CELL_SAVEDATA_FILETYPE_CONTENT_ICON1;
				}
				else if (entry->name == "PIC1.PNG")
				{
					file.fileType = CELL_SAVEDATA_FILETYPE_CONTENT_PIC1;
				}
				else if (entry->name == "SND0.AT3")
				{
					file.fileType = CELL_SAVEDATA_FILETYPE_CONTENT_SND0;
				}
				else if (psf.GetInteger("*" + entry->name)) // let's put the list of protected files in PARAM.SFO (int param = 1 if protected)
				{
					file.fileType = CELL_SAVEDATA_FILETYPE_SECUREFILE;
				}
				else
				{
					file.fileType = CELL_SAVEDATA_FILETYPE_NORMALFILE;
				}

				file.size = entry->size;
				file.atime = entry->access_time;
				file.mtime = entry->modify_time;
				file.ctime = entry->create_time;
				strcpy_trunc(file.fileName, entry->name);
			}
		}

		// Stat Callback
		funcStat(ppu, result, statGet, statSet);

		if (result->result < 0)
		{
			cellSysutil.Warning("savedata_op(): funcStat returned < 0.");
			return CELL_SAVEDATA_ERROR_CBRESULT;
		}

		// Update PARAM.SFO
		if (statSet->setParam)
		{
			psf.Clear();
			psf.SetString("ACCOUNT_ID", ""); // ???
			psf.SetInteger("ATTRIBUTE", statSet->setParam->attribute);
			psf.SetString("CATEGORY", "SD"); // ???
			psf.SetString("PARAMS", ""); // ???
			psf.SetString("PARAMS2", ""); // ???
			psf.SetInteger("PARENTAL_LEVEL", 0); // ???
			psf.SetString("DETAIL", statSet->setParam->detail);
			psf.SetString("SAVEDATA_DIRECTORY", save_entry.dirName);
			psf.SetString("SAVEDATA_LIST_PARAM", statSet->setParam->listParam);
			psf.SetString("SUB_TITLE", statSet->setParam->subTitle);
			psf.SetString("TITLE", statSet->setParam->title);
		}

		switch (const u32 mode = statSet->reCreateMode & 0xffff)
		{
		case CELL_SAVEDATA_RECREATE_NO:
		case CELL_SAVEDATA_RECREATE_NO_NOBROKEN:
		{
			break;
		}
		case CELL_SAVEDATA_RECREATE_YES:
		case CELL_SAVEDATA_RECREATE_YES_RESET_OWNER:
		{
			// kill it with fire
			for (const auto entry : vfsDir(dir_path))
			{
				if (entry->flags & DirEntry_TypeFile)
				{
					Emu.GetVFS().RemoveFile(dir_path + entry->name);
				}
			}

			break;
		}
		default:
		{
			cellSysutil.Error("savedata_op(): unknown statSet->reCreateMode (0x%x)", statSet->reCreateMode);
			return CELL_SAVEDATA_ERROR_PARAM;
		}
		}
	}

	// Create save directory if necessary
	if (psf && save_entry.isNew && !Emu.GetVFS().CreateDir(dir_path))
	{
		// Let's ignore this error for now
	}

	// Enter the loop where the save files are read/created/deleted
	vm::stackvar<CellSaveDataFileGet> fileGet(ppu);
	vm::stackvar<CellSaveDataFileSet> fileSet(ppu);

	fileGet->excSize = 0;
	memset(fileGet->reserved, 0, sizeof(fileGet->reserved));

	while (funcFile)
	{
		funcFile(ppu, result, fileGet, fileSet);

		if (result->result < 0)
		{
			cellSysutil.Warning("savedata_op(): funcFile returned < 0.");
			return CELL_SAVEDATA_ERROR_CBRESULT;
		}

		if (result->result == CELL_SAVEDATA_CBRESULT_OK_LAST || result->result == CELL_SAVEDATA_CBRESULT_OK_LAST_NOCONFIRM)
		{
			break;
		}

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
			cellSysutil.Error("savedata_op(): unknown fileSet->fileType (0x%x)", type);
			return CELL_SAVEDATA_ERROR_PARAM;
		}
		}

		psf.SetInteger("*" + file_path, fileSet->fileType == CELL_SAVEDATA_FILETYPE_SECUREFILE);

		std::string local_path;

		Emu.GetVFS().GetDevice(dir_path + file_path, local_path);

		switch (const u32 op = fileSet->fileOperation)
		{
		case CELL_SAVEDATA_FILEOP_READ:
		{
			fs::file file(local_path, o_read);
			file.seek(fileSet->fileOffset);
			fileGet->excSize = static_cast<u32>(file.read(fileSet->fileBuf.get_ptr(), std::min<u32>(fileSet->fileSize, fileSet->fileBufSize)));
			break;
		}

		case CELL_SAVEDATA_FILEOP_WRITE:
		{
			fs::file file(local_path, o_write | o_create);
			file.seek(fileSet->fileOffset);
			fileGet->excSize = static_cast<u32>(file.write(fileSet->fileBuf.get_ptr(), std::min<u32>(fileSet->fileSize, fileSet->fileBufSize)));
			file.trunc(file.seek(0, from_cur)); // truncate
			break;
		}

		case CELL_SAVEDATA_FILEOP_DELETE:
		{
			fs::remove_file(local_path);
			fileGet->excSize = 0;
			break;
		}

		case CELL_SAVEDATA_FILEOP_WRITE_NOTRUNC:
		{
			fs::file file(local_path, o_write | o_create);
			file.seek(fileSet->fileOffset);
			fileGet->excSize = static_cast<u32>(file.write(fileSet->fileBuf.get_ptr(), std::min<u32>(fileSet->fileSize, fileSet->fileBufSize)));
			break;
		}

		default:
		{
			cellSysutil.Error("savedata_op(): unknown fileSet->fileOperation (0x%x)", op);
			return CELL_SAVEDATA_ERROR_PARAM;
		}
		}
	}

	// Write PARAM.SFO
	if (psf)
	{
		vfsFile f(sfo_path, vfsWriteNew);
		psf.Save(f);
	}

	return CELL_OK;
}

// Functions
s32 cellSaveDataListSave2(PPUThread& ppu, u32 version, PSetList setList, PSetBuf setBuf, PFuncList funcList, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSysutil.Warning("cellSaveDataListSave2(version=%d, setList=*0x%x, setBuf=*0x%x, funcList=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, setList, setBuf, funcList, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_LIST_SAVE, version, vm::null, 1, setList, setBuf, funcList, vm::null, funcStat, funcFile, container, 2, userdata, 0, vm::null);
}

s32 cellSaveDataListLoad2(PPUThread& ppu, u32 version, PSetList setList, PSetBuf setBuf, PFuncList funcList, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSysutil.Warning("cellSaveDataListLoad2(version=%d, setList=*0x%x, setBuf=*0x%x, funcList=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, setList, setBuf, funcList, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_LIST_LOAD, version, vm::null, 1, setList, setBuf, funcList, vm::null, funcStat, funcFile, container, 2, userdata, 0, vm::null);
}

s32 cellSaveDataListSave()
{
	throw EXCEPTION("");
}

s32 cellSaveDataListLoad()
{
	throw EXCEPTION("");
}

s32 cellSaveDataFixedSave2(PPUThread& ppu, u32 version, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSysutil.Warning("cellSaveDataFixedSave2(version=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_FIXED_SAVE, version, vm::null, 1, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 2, userdata, 0, vm::null);
}

s32 cellSaveDataFixedLoad2(PPUThread& ppu, u32 version, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSysutil.Warning("cellSaveDataFixedLoad2(version=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_FIXED_LOAD, version, vm::null, 1, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 2, userdata, 0, vm::null);
}

s32 cellSaveDataFixedSave()
{
	throw EXCEPTION("");
}

s32 cellSaveDataFixedLoad()
{
	throw EXCEPTION("");
}

s32 cellSaveDataAutoSave2(PPUThread& ppu, u32 version, vm::cptr<char> dirName, u32 errDialog, PSetBuf setBuf, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSysutil.Warning("cellSaveDataAutoSave2(version=%d, dirName=*0x%x, errDialog=%d, setBuf=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, dirName, errDialog, setBuf, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_AUTO_SAVE, version, dirName, errDialog, vm::null, setBuf, vm::null, vm::null, funcStat, funcFile, container, 2, userdata, 0, vm::null);
}

s32 cellSaveDataAutoLoad2(PPUThread& ppu, u32 version, vm::cptr<char> dirName, u32 errDialog, PSetBuf setBuf, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSysutil.Warning("cellSaveDataAutoLoad2(version=%d, dirName=*0x%x, errDialog=%d, setBuf=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, dirName, errDialog, setBuf, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_AUTO_LOAD, version, dirName, errDialog, vm::null, setBuf, vm::null, vm::null, funcStat, funcFile, container, 2, userdata, 0, vm::null);
}

s32 cellSaveDataAutoSave()
{
	throw EXCEPTION("");
}

s32 cellSaveDataAutoLoad()
{
	throw EXCEPTION("");
}

s32 cellSaveDataListAutoSave(PPUThread& ppu, u32 version, u32 errDialog, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSysutil.Warning("cellSaveDataListAutoSave(version=%d, errDialog=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, errDialog, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_LIST_AUTO_SAVE, version, vm::null, errDialog, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 0, userdata, 0, vm::null);
}

s32 cellSaveDataListAutoLoad(PPUThread& ppu, u32 version, u32 errDialog, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSysutil.Warning("cellSaveDataListAutoLoad(version=%d, errDialog=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, errDialog, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_LIST_AUTO_LOAD, version, vm::null, errDialog, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 0, userdata, 0, vm::null);
}

s32 cellSaveDataDelete2(u32 container)
{
	cellSysutil.Todo("cellSaveDataDelete2(container=0x%x)", container);

	return CELL_SAVEDATA_RET_CANCEL;
}

s32 cellSaveDataDelete()
{
	throw EXCEPTION("");
}

s32 cellSaveDataFixedDelete(PPUThread& ppu, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	cellSysutil.Todo("cellSaveDataFixedDelete(setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcDone=*0x%x, container=0x%x, userdata=*0x%x)",
		setList, setBuf, funcFixed, funcDone, container, userdata);

	return CELL_OK;
}

s32 cellSaveDataUserListSave(PPUThread& ppu, u32 version, u32 userId, PSetList setList, PSetBuf setBuf, PFuncList funcList, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSysutil.Error("cellSaveDataUserListSave(version=%d, userId=%d, setList=*0x%x, setBuf=*0x%x, funcList=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, setList, setBuf, funcList, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_LIST_SAVE, version, vm::null, 0, setList, setBuf, funcList, vm::null, funcStat, funcFile, container, 6, userdata, userId, vm::null);
}

s32 cellSaveDataUserListLoad(PPUThread& ppu, u32 version, u32 userId, PSetList setList, PSetBuf setBuf, PFuncList funcList, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSysutil.Error("cellSaveDataUserListLoad(version=%d, userId=%d, setList=*0x%x, setBuf=*0x%x, funcList=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, setList, setBuf, funcList, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_LIST_LOAD, version, vm::null, 0, setList, setBuf, funcList, vm::null, funcStat, funcFile, container, 6, userdata, userId, vm::null);
}

s32 cellSaveDataUserFixedSave(PPUThread& ppu, u32 version, u32 userId, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSysutil.Error("cellSaveDataUserFixedSave(version=%d, userId=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_FIXED_SAVE, version, vm::null, 0, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 6, userdata, userId, vm::null);
}

s32 cellSaveDataUserFixedLoad(PPUThread& ppu, u32 version, u32 userId, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSysutil.Error("cellSaveDataUserFixedLoad(version=%d, userId=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_FIXED_LOAD, version, vm::null, 0, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 6, userdata, userId, vm::null);
}

s32 cellSaveDataUserAutoSave(PPUThread& ppu, u32 version, u32 userId, vm::cptr<char> dirName, u32 errDialog, PSetBuf setBuf, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSysutil.Error("cellSaveDataUserAutoSave(version=%d, userId=%d, dirName=*0x%x, errDialog=%d, setBuf=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, dirName, errDialog, setBuf, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_AUTO_SAVE, version, dirName, errDialog, vm::null, setBuf, vm::null, vm::null, funcStat, funcFile, container, 6, userdata, userId, vm::null);
}

s32 cellSaveDataUserAutoLoad(PPUThread& ppu, u32 version, u32 userId, vm::cptr<char> dirName, u32 errDialog, PSetBuf setBuf, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSysutil.Error("cellSaveDataUserAutoLoad(version=%d, userId=%d, dirName=*0x%x, errDialog=%d, setBuf=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, dirName, errDialog, setBuf, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_AUTO_LOAD, version, dirName, errDialog, vm::null, setBuf, vm::null, vm::null, funcStat, funcFile, container, 6, userdata, userId, vm::null);
}

s32 cellSaveDataUserListAutoSave(PPUThread& ppu, u32 version, u32 userId, u32 errDialog, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSysutil.Error("cellSaveDataUserListAutoSave(version=%d, userId=%d, errDialog=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, errDialog, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_LIST_AUTO_SAVE, version, vm::null, errDialog, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 6, userdata, userId, vm::null);
}

s32 cellSaveDataUserListAutoLoad(PPUThread& ppu, u32 version, u32 userId, u32 errDialog, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed, PFuncStat funcStat, PFuncFile funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSysutil.Error("cellSaveDataUserListAutoLoad(version=%d, userId=%d, errDialog=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, errDialog, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return savedata_op(ppu, SAVEDATA_OP_LIST_AUTO_LOAD, version, vm::null, errDialog, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 6, userdata, userId, vm::null);
}

s32 cellSaveDataUserFixedDelete(PPUThread& ppu, u32 userId, PSetList setList, PSetBuf setBuf, PFuncFixed funcFixed, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	cellSysutil.Todo("cellSaveDataUserFixedDelete(userId=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcDone=*0x%x, container=0x%x, userdata=*0x%x)",
		userId, setList, setBuf, funcFixed, funcDone, container, userdata);

	return CELL_OK;
}

void cellSaveDataEnableOverlay(s32 enable)
{
	cellSysutil.Error("cellSaveDataEnableOverlay(enable=%d)", enable);

	return;
}


// Functions (Extensions) 
s32 cellSaveDataListDelete(PPUThread& ppu, PSetList setList, PSetBuf setBuf, PFuncList funcList, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSaveData);

	return CELL_OK;
}

s32 cellSaveDataListImport(PPUThread& ppu, PSetList setList, u32 maxSizeKB, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSaveData);

	return CELL_OK;
}

s32 cellSaveDataListExport(PPUThread& ppu, PSetList setList, u32 maxSizeKB, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSaveData);

	return CELL_OK;
}

s32 cellSaveDataFixedImport(PPUThread& ppu, vm::cptr<char> dirName, u32 maxSizeKB, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSaveData);

	return CELL_OK;
}

s32 cellSaveDataFixedExport(PPUThread& ppu, vm::cptr<char> dirName, u32 maxSizeKB, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSaveData);

	return CELL_OK;
}

s32 cellSaveDataGetListItem(vm::cptr<char> dirName, vm::ptr<CellSaveDataDirStat> dir, vm::ptr<CellSaveDataSystemFileParam> sysFileParam, vm::ptr<u32> bind, vm::ptr<u32> sizeKB)
{
	UNIMPLEMENTED_FUNC(cellSaveData);

	return CELL_OK;
}

s32 cellSaveDataUserListDelete(PPUThread& ppu, u32 userId, PSetList setList, PSetBuf setBuf, PFuncList funcList, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSaveData);

	return CELL_OK;
}

s32 cellSaveDataUserListImport(PPUThread& ppu, u32 userId, PSetList setList, u32 maxSizeKB, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSaveData);

	return CELL_OK;
}

s32 cellSaveDataUserListExport(PPUThread& ppu, u32 userId, PSetList setList, u32 maxSizeKB, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSaveData);

	return CELL_OK;
}

s32 cellSaveDataUserFixedImport(PPUThread& ppu, u32 userId, vm::cptr<char> dirName, u32 maxSizeKB, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSaveData);

	return CELL_OK;
}

s32 cellSaveDataUserFixedExport(PPUThread& ppu, u32 userId, vm::cptr<char> dirName, u32 maxSizeKB, PFuncDone funcDone, u32 container, vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSaveData);

	return CELL_OK;
}

s32 cellSaveDataUserGetListItem(u32 userId, vm::cptr<char> dirName, vm::ptr<CellSaveDataDirStat> dir, vm::ptr<CellSaveDataSystemFileParam> sysFileParam, vm::ptr<u32> bind, vm::ptr<u32> sizeKB)
{
	UNIMPLEMENTED_FUNC(cellSaveData);

	return CELL_OK;
}

void cellSysutil_SaveData_init()
{
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

Module cellSaveData("cellSaveData", []()
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

Module cellMinisSaveData("cellMinisSaveData", []()
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
