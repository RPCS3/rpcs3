#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/CB_FUNC.h"

#include "Emu/FS/VFS.h"
#include "Emu/FS/vfsFile.h"
#include "Emu/FS/vfsDir.h"
#include "Loader/PSF.h"
#include "cellSaveData.h"

#ifdef _WIN32
	#include <windows.h>
	#undef CreateFile
#else
	#include <sys/types.h>
	#include <sys/stat.h>
#endif

extern Module cellSysutil;

// Auxiliary Classes
class SortSaveDataEntry
{
	const u32 m_type;
	const u32 m_order;

public:
	SortSaveDataEntry(u32 type, u32 order)
		: m_type(type)
		, m_order(order)
	{
	}

	bool operator()(const SaveDataEntry& entry1, const SaveDataEntry& entry2) const
	{
		if (m_order == CELL_SAVEDATA_SORTORDER_DESCENT)
		{
			if (m_type == CELL_SAVEDATA_SORTTYPE_MODIFIEDTIME)
			{
				return entry1.mtime >= entry2.mtime;
			}

			if (m_type == CELL_SAVEDATA_SORTTYPE_SUBTITLE)
			{
				return entry1.subtitle >= entry2.subtitle;
			}
		}

		if (m_order == CELL_SAVEDATA_SORTORDER_ASCENT)
		{
			if (m_type == CELL_SAVEDATA_SORTTYPE_MODIFIEDTIME)
			{
				return entry1.mtime < entry2.mtime;
			}

			if (m_type == CELL_SAVEDATA_SORTTYPE_SUBTITLE)
			{
				return entry1.subtitle < entry2.subtitle;
			}
		}

		return true;
	}
};

// Auxiliary Functions
u64 get_save_data_size(const std::string& dir)
{
	u64 result = 0;

	for (const auto entry : vfsDir(dir))
	{
		if ((entry->flags & DirEntry_TypeMask) == DirEntry_TypeFile)
		{
			result += vfsFile(dir + "/" + entry->name).GetSize();
		}
	}

	return result;
}

void addNewSaveDataEntry(std::vector<SaveDataEntry>& saveEntries, vm::ptr<CellSaveDataListNewData> newData)
{
	SaveDataEntry saveEntry;
	saveEntry.dirName = newData->dirName.get_ptr();
	saveEntry.title = newData->icon->title.get_ptr();
	saveEntry.subtitle = newData->icon->title.get_ptr();
	//saveEntry.iconBuf = newData->icon->iconBuf.get_ptr();
	//saveEntry.iconBufSize = newData->icon->iconBufSize;
	saveEntry.isNew = true;
	// TODO: Add information stored in newData->iconPosition. (It's not very relevant)

	saveEntries.push_back(saveEntry);
}

u32 focusSaveDataEntry(const std::vector<SaveDataEntry>& saveEntries, u32 focusPosition)
{
	// TODO: Get the correct index. Right now, this returns the first element of the list.
	return 0;
}

void setSaveDataList(std::vector<SaveDataEntry>& saveEntries, vm::ptr<CellSaveDataDirList> fixedList, u32 fixedListNum)
{
	std::vector<SaveDataEntry>::iterator entry = saveEntries.begin();
	while (entry != saveEntries.end())
	{
		bool found = false;
		for (u32 j = 0; j < fixedListNum; j++)
		{
			if (entry->dirName == (char*)fixedList[j].dirName)
			{
				found = true;
				break;
			}
		}
		if (!found)
			entry = saveEntries.erase(entry);
		else
			entry++;
	}
}

void setSaveDataFixed(std::vector<SaveDataEntry>& saveEntries, vm::ptr<CellSaveDataFixedSet> fixedSet)
{
	std::vector<SaveDataEntry>::iterator entry = saveEntries.begin();
	while (entry != saveEntries.end())
	{
		if (entry->dirName == fixedSet->dirName.get_ptr())
			entry = saveEntries.erase(entry);
		else
			entry++;
	}

	if (saveEntries.size() == 0)
	{
		SaveDataEntry entry;
		entry.dirName = fixedSet->dirName.get_ptr();
		entry.isNew = true;
		saveEntries.push_back(entry);
	}

	if (fixedSet->newIcon)
	{
		//saveEntries[0].iconBuf = fixedSet->newIcon->iconBuf.get_ptr();
		//saveEntries[0].iconBufSize = fixedSet->newIcon->iconBufSize;
		saveEntries[0].title = fixedSet->newIcon->title.get_ptr();
		saveEntries[0].subtitle = fixedSet->newIcon->title.get_ptr();
	}
}

void getSaveDataStat(SaveDataEntry entry, vm::ptr<CellSaveDataStatGet> statGet)
{
	if (entry.isNew)
		statGet->isNewData = CELL_SAVEDATA_ISNEWDATA_YES;
	else
		statGet->isNewData = CELL_SAVEDATA_ISNEWDATA_NO;

	statGet->bind = 0; // TODO ?
	statGet->sizeKB = entry.sizeKB;
	statGet->hddFreeSizeKB = 40000000; // 40 GB. TODO ?
	statGet->sysSizeKB = 0; // TODO: This is the size of PARAM.SFO + PARAM.PDF
	statGet->dir.st_atime_ = 0; // TODO ?
	statGet->dir.st_mtime_ = 0; // TODO ?
	statGet->dir.st_ctime_ = 0; // TODO ?
	strcpy_trunc(statGet->dir.dirName, entry.dirName);

	statGet->getParam.attribute = 0; // TODO ?
	strcpy_trunc(statGet->getParam.title, entry.title);
	strcpy_trunc(statGet->getParam.subTitle, entry.subtitle);
	strcpy_trunc(statGet->getParam.detail, entry.details);
	strcpy_trunc(statGet->getParam.listParam, entry.listParam);

	statGet->fileNum = 0;
	statGet->fileList.set(0);
	statGet->fileListNum = 0;
	std::string saveDir = "/dev_hdd0/home/00000001/savedata/" + entry.dirName; // TODO: Get the path of the current user
	vfsDir dir(saveDir);
	if (!dir.IsOpened())
		return;

	std::vector<CellSaveDataFileStat> fileEntries;
	for(const DirEntryInfo* dirEntry = dir.Read(); dirEntry; dirEntry = dir.Read()) {
		if (dirEntry->flags & DirEntry_TypeFile) {
			if (dirEntry->name == "PARAM.SFO" || dirEntry->name == "PARAM.PFD")
				continue;

			statGet->fileNum++;
			statGet->fileListNum++;
			CellSaveDataFileStat fileEntry;
			vfsFile file(saveDir + "/" + dirEntry->name);

			if (dirEntry->name == "ICON0.PNG")
				fileEntry.fileType = CELL_SAVEDATA_FILETYPE_CONTENT_ICON0;
			else if (dirEntry->name == "ICON1.PAM")
				fileEntry.fileType = CELL_SAVEDATA_FILETYPE_CONTENT_ICON1;
			else if (dirEntry->name == "PIC1.PNG") 
				fileEntry.fileType = CELL_SAVEDATA_FILETYPE_CONTENT_PIC1;
			else if (dirEntry->name == "SND0.AT3") 
				fileEntry.fileType = CELL_SAVEDATA_FILETYPE_CONTENT_SND0;

			fileEntry.st_size = file.GetSize();
			fileEntry.st_atime_ = 0; // TODO ?
			fileEntry.st_mtime_ = 0; // TODO ?
			fileEntry.st_ctime_ = 0; // TODO ?
			strcpy_trunc(fileEntry.fileName, dirEntry->name);

			fileEntries.push_back(fileEntry);
		}
	}

	statGet->fileList.set((u32)Memory.Alloc(sizeof(CellSaveDataFileStat) * fileEntries.size(), 8));
	for (u32 i = 0; i < fileEntries.size(); i++) {
		CellSaveDataFileStat *dst = &statGet->fileList[i];
		memcpy(dst, &fileEntries[i], sizeof(CellSaveDataFileStat));
	}
}

s32 modifySaveDataFiles(vm::ptr<CellSaveDataFileCallback> funcFile, vm::ptr<CellSaveDataCBResult> result, const std::string& saveDataDir)
{
	vm::var<CellSaveDataFileGet> fileGet;
	vm::var<CellSaveDataFileSet> fileSet;

	if (!Emu.GetVFS().ExistsDir(saveDataDir))
		Emu.GetVFS().CreateDir(saveDataDir);

	fileGet->excSize = 0;
	while (true)
	{
		funcFile(result, fileGet, fileSet);
		if (result->result < 0)	{
			cellSysutil.Error("modifySaveDataFiles: CellSaveDataFileCallback failed."); // TODO: Once we verify that the entire SysCall is working, delete this debug error message.
			return CELL_SAVEDATA_ERROR_CBRESULT;
		}
		if (result->result == CELL_SAVEDATA_CBRESULT_OK_LAST || result->result == CELL_SAVEDATA_CBRESULT_OK_LAST_NOCONFIRM) {
			break;
		}

		std::string filepath = saveDataDir + '/';
		vfsStream* file = NULL;
		void* buf = fileSet->fileBuf.get_ptr();

		switch ((u32)fileSet->fileType)
		{
		case CELL_SAVEDATA_FILETYPE_SECUREFILE:     filepath += fileSet->fileName.get_ptr(); break;
		case CELL_SAVEDATA_FILETYPE_NORMALFILE:     filepath += fileSet->fileName.get_ptr(); break;
		case CELL_SAVEDATA_FILETYPE_CONTENT_ICON0:  filepath += "ICON0.PNG"; break;
		case CELL_SAVEDATA_FILETYPE_CONTENT_ICON1:  filepath += "ICON1.PAM"; break;
		case CELL_SAVEDATA_FILETYPE_CONTENT_PIC1:   filepath += "PIC1.PNG";  break;
		case CELL_SAVEDATA_FILETYPE_CONTENT_SND0:   filepath += "SND0.AT3";  break;

		default:
			cellSysutil.Error("modifySaveDataFiles: Unknown fileType! Aborting...");
			return CELL_SAVEDATA_ERROR_PARAM;
		}

		switch ((u32)fileSet->fileOperation)
		{
		case CELL_SAVEDATA_FILEOP_READ:
			file = Emu.GetVFS().OpenFile(filepath, vfsRead);
			fileGet->excSize = (u32)file->Read(buf, (u32)std::min(fileSet->fileSize, fileSet->fileBufSize)); // TODO: This may fail for big files because of the dest pointer.
			break;
		
		case CELL_SAVEDATA_FILEOP_WRITE:
			Emu.GetVFS().CreateFile(filepath);
			file = Emu.GetVFS().OpenFile(filepath, vfsWrite);
			fileGet->excSize = (u32)file->Write(buf, (u32)std::min(fileSet->fileSize, fileSet->fileBufSize)); // TODO: This may fail for big files because of the dest pointer.
			break;

		case CELL_SAVEDATA_FILEOP_DELETE:
			Emu.GetVFS().RemoveFile(filepath);
			fileGet->excSize = 0;
			break;

		case CELL_SAVEDATA_FILEOP_WRITE_NOTRUNC:
			cellSysutil.Todo("modifySaveDataFiles: CELL_SAVEDATA_FILEOP_WRITE_NOTRUNC");
			break;

		default:
			cellSysutil.Error("modifySaveDataFiles: Unknown fileOperation! Aborting...");
			return CELL_SAVEDATA_ERROR_PARAM;
		}

		if (file && file->IsOpened())
			file->Close();
	}
	return CELL_OK;
}

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

__noinline s32 savedata_op(
	PPUThread& CPU,
	u32 operation,
	u32 version,
	vm::ptr<const char> dirName,
	u32 errDialog,
	vm::ptr<CellSaveDataSetList> setList,
	vm::ptr<CellSaveDataSetBuf> setBuf,
	vm::ptr<CellSaveDataListCallback> funcList,
	vm::ptr<CellSaveDataFixedCallback> funcFixed,
	vm::ptr<CellSaveDataStatCallback> funcStat,
	vm::ptr<CellSaveDataFileCallback> funcFile,
	u32 container,
	u32 unknown, // 0, 2, 6
	vm::ptr<void> userdata,
	u32 userId,
	vm::ptr<CellSaveDataDoneCallback> funcDone)
{
	static const std::string base_dir = "/dev_hdd0/home/00000001/savedata/"; // TODO: Get the path of the current or specified user

	vm::stackvar<CellSaveDataCBResult> result(CPU);
	vm::stackvar<CellSaveDataListGet> listGet(CPU);
	vm::stackvar<CellSaveDataListSet> listSet(CPU);
	vm::stackvar<CellSaveDataStatGet> statGet(CPU);
	vm::stackvar<CellSaveDataStatSet> statSet(CPU);

	result->userdata = userdata;

	std::vector<SaveDataEntry> save_entries;

	if (setList)
	{
		listGet->dirNum = 0;
		listGet->dirListNum = 0;
		listGet->dirList.set(setBuf->buf.addr());
		memset(listGet->reserved, 0, sizeof(listGet->reserved));

		const auto prefix_list = fmt::split(setList->dirNamePrefix.get_ptr(), { "|" });

		for (const auto entry : vfsDir(base_dir))
		{
			if ((entry->flags & DirEntry_TypeMask) != DirEntry_TypeDir)
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
						PSFLoader psf(f);
						if (!psf.Load(false))
						{
							break;
						}

						SaveDataEntry save_entry;
						save_entry.dirName = psf.GetString("SAVEDATA_DIRECTORY");
						save_entry.listParam = psf.GetString("SAVEDATA_LIST_PARAM");
						save_entry.title = psf.GetString("TITLE");
						save_entry.subtitle = psf.GetString("SUB_TITLE");
						save_entry.details = psf.GetString("DETAIL");
						save_entry.sizeKB = get_save_data_size(base_dir + entry->name) / 1024;
						save_entry.atime = entry->access_time;
						save_entry.mtime = entry->modify_time;
						save_entry.ctime = entry->create_time;
						//save_entry.iconBuf = NULL; // TODO: Here should be the PNG buffer
						//save_entry.iconBufSize = 0; // TODO: Size of the PNG file
						save_entry.isNew = false;

						save_entries.push_back(save_entry);
					}

					break;
				}
			}
		}

		// Sort the entries
		std::sort(save_entries.begin(), save_entries.end(), SortSaveDataEntry(setList->sortType, setList->sortOrder));

		// Fill the listGet->dirList array
		auto dir_list = listGet->dirList.get_ptr();

		for (const auto& entry : save_entries)
		{
			auto& dir = *dir_list++;
			strcpy_trunc(dir.dirName, entry.dirName);
			strcpy_trunc(dir.listParam, entry.listParam);
			memset(dir.reserved, 0, sizeof(dir.reserved));
		}

		// Data List Callback
		funcList(result, listGet, listSet);
	}
	

	

	setSaveDataList(save_entries, listSet->fixedList, listSet->fixedListNum);
	if (listSet->newData)
		addNewSaveDataEntry(save_entries, listSet->newData);
	if (save_entries.size() == 0) {
		cellSysutil.Error("cellSaveDataListLoad2: No save entries found!"); // TODO: Find a better way to handle this error
		return CELL_OK;
	}

	u32 focusIndex = focusSaveDataEntry(save_entries, listSet->focusPosition);
	// TODO: Display the dialog here
	u32 selectedIndex = focusIndex; // TODO: Until the dialog is implemented, select always the focused entry
	getSaveDataStat(save_entries[selectedIndex], statGet);
	result->userdata = userdata;

	funcStat(result, statGet, statSet);
	Memory.Free(statGet->fileList.addr());
	if (result->result < 0)	{
		cellSysutil.Error("cellSaveDataListLoad2: CellSaveDataStatCallback failed."); // TODO: Once we verify that the entire SysCall is working, delete this debug error message.
		return CELL_SAVEDATA_ERROR_CBRESULT;
	}

	/*if (statSet->setParam)
	// TODO: Write PARAM.SFO file
	*/

	// Enter the loop where the save files are read/created/deleted.
	s32 ret = modifySaveDataFiles(funcFile, result, base_dir + (char*)statGet->dir.dirName);

	return ret;
}

// Functions
s32 cellSaveDataListSave2(
	PPUThread& CPU,
	u32 version,
	vm::ptr<CellSaveDataSetList> setList,
	vm::ptr<CellSaveDataSetBuf> setBuf,
	vm::ptr<CellSaveDataListCallback> funcList,
	vm::ptr<CellSaveDataStatCallback> funcStat,
	vm::ptr<CellSaveDataFileCallback> funcFile,
	u32 container,
	vm::ptr<void> userdata)
{
	cellSysutil.Warning("cellSaveDataListSave2(version=%d, setList=*0x%x, setBuf=*0x%x, funcList=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, setList, setBuf, funcList, funcStat, funcFile, container, userdata);

	return savedata_op(CPU, SAVEDATA_OP_LIST_SAVE, version, vm::null, 1, setList, setBuf, funcList, vm::null, funcStat, funcFile, container, 2, userdata, 0, vm::null);
}

s32 cellSaveDataListLoad2(
	PPUThread& CPU,
	u32 version,
	vm::ptr<CellSaveDataSetList> setList,
	vm::ptr<CellSaveDataSetBuf> setBuf,
	vm::ptr<CellSaveDataListCallback> funcList,
	vm::ptr<CellSaveDataStatCallback> funcStat,
	vm::ptr<CellSaveDataFileCallback> funcFile,
	u32 container,
	vm::ptr<void> userdata)
{
	cellSysutil.Warning("cellSaveDataListLoad2(version=%d, setList=*0x%x, setBuf=*0x%x, funcList=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, setList, setBuf, funcList, funcStat, funcFile, container, userdata);

	return savedata_op(CPU, SAVEDATA_OP_LIST_LOAD, version, vm::null, 1, setList, setBuf, funcList, vm::null, funcStat, funcFile, container, 2, userdata, 0, vm::null);
}

s32 cellSaveDataFixedSave2(
	PPUThread& CPU,
	u32 version,
	vm::ptr<CellSaveDataSetList> setList,
	vm::ptr<CellSaveDataSetBuf> setBuf,
	vm::ptr<CellSaveDataFixedCallback> funcFixed,
	vm::ptr<CellSaveDataStatCallback> funcStat,
	vm::ptr<CellSaveDataFileCallback> funcFile,
	u32 container,
	vm::ptr<void> userdata)
{
	cellSysutil.Warning("cellSaveDataFixedSave2(version=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return savedata_op(CPU, SAVEDATA_OP_FIXED_SAVE, version, vm::null, 1, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 2, userdata, 0, vm::null);
}

s32 cellSaveDataFixedLoad2(
	PPUThread& CPU,
	u32 version,
	vm::ptr<CellSaveDataSetList> setList,
	vm::ptr<CellSaveDataSetBuf> setBuf,
	vm::ptr<CellSaveDataFixedCallback> funcFixed,
	vm::ptr<CellSaveDataStatCallback> funcStat,
	vm::ptr<CellSaveDataFileCallback> funcFile,
	u32 container,
	vm::ptr<void> userdata)
{
	cellSysutil.Warning("cellSaveDataFixedLoad2(version=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return savedata_op(CPU, SAVEDATA_OP_FIXED_LOAD, version, vm::null, 1, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 2, userdata, 0, vm::null);
}

s32 cellSaveDataAutoSave2(
	PPUThread& CPU,
	u32 version,
	vm::ptr<const char> dirName,
	u32 errDialog,
	vm::ptr<CellSaveDataSetBuf> setBuf,
	vm::ptr<CellSaveDataStatCallback> funcStat,
	vm::ptr<CellSaveDataFileCallback> funcFile,
	u32 container,
	vm::ptr<void> userdata)
{
	cellSysutil.Warning("cellSaveDataAutoSave2(version=%d, dirName=*0x%x, errDialog=%d, setBuf=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, dirName, errDialog, setBuf, funcStat, funcFile, container, userdata);

	return savedata_op(CPU, SAVEDATA_OP_AUTO_SAVE, version, dirName, errDialog, vm::null, setBuf, vm::null, vm::null, funcStat, funcFile, container, 2, userdata, 0, vm::null);
}

s32 cellSaveDataAutoLoad2(
	PPUThread& CPU,
	u32 version,
	vm::ptr<const char> dirName,
	u32 errDialog,
	vm::ptr<CellSaveDataSetBuf> setBuf,
	vm::ptr<CellSaveDataStatCallback> funcStat,
	vm::ptr<CellSaveDataFileCallback> funcFile,
	u32 container,
	vm::ptr<void> userdata)
{
	cellSysutil.Warning("cellSaveDataAutoLoad2(version=%d, dirName=*0x%x, errDialog=%d, setBuf=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, dirName, errDialog, setBuf, funcStat, funcFile, container, userdata);

	return savedata_op(CPU, SAVEDATA_OP_AUTO_LOAD, version, dirName, errDialog, vm::null, setBuf, vm::null, vm::null, funcStat, funcFile, container, 2, userdata, 0, vm::null);
}

s32 cellSaveDataListAutoSave(
	PPUThread& CPU,
	u32 version,
	u32 errDialog,
	vm::ptr<CellSaveDataSetList> setList,
	vm::ptr<CellSaveDataSetBuf> setBuf,
	vm::ptr<CellSaveDataFixedCallback> funcFixed,
	vm::ptr<CellSaveDataStatCallback> funcStat,
	vm::ptr<CellSaveDataFileCallback> funcFile,
	u32 container,
	vm::ptr<void> userdata)
{
	cellSysutil.Warning("cellSaveDataListAutoSave(version=%d, errDialog=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, errDialog, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return savedata_op(CPU, SAVEDATA_OP_LIST_AUTO_SAVE, version, vm::null, errDialog, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 0, userdata, 0, vm::null);
}

s32 cellSaveDataListAutoLoad(
	PPUThread& CPU,
	u32 version,
	u32 errDialog,
	vm::ptr<CellSaveDataSetList> setList,
	vm::ptr<CellSaveDataSetBuf> setBuf,
	vm::ptr<CellSaveDataFixedCallback> funcFixed,
	vm::ptr<CellSaveDataStatCallback> funcStat,
	vm::ptr<CellSaveDataFileCallback> funcFile,
	u32 container,
	vm::ptr<void> userdata)
{
	cellSysutil.Warning("cellSaveDataListAutoLoad(version=%d, errDialog=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, errDialog, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return savedata_op(CPU, SAVEDATA_OP_LIST_AUTO_LOAD, version, vm::null, errDialog, setList, setBuf, vm::null, funcFixed, funcStat, funcFile, container, 0, userdata, 0, vm::null);
}

s32 cellSaveDataDelete2(u32 container)
{	 
	cellSysutil.Todo("cellSaveDataDelete2(container=0x%x)", container);

	return CELL_SAVEDATA_RET_CANCEL;
}

s32 cellSaveDataFixedDelete(
	PPUThread& CPU,
	vm::ptr<CellSaveDataSetList> setList,
	vm::ptr<CellSaveDataSetBuf> setBuf,
	vm::ptr<CellSaveDataFixedCallback> funcFixed,
	vm::ptr<CellSaveDataDoneCallback> funcDone,
	u32 container,
	vm::ptr<void> userdata)
{
	cellSysutil.Todo("cellSaveDataFixedDelete(setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcDone=*0x%x, container=0x%x, userdata=*0x%x)",
		setList, setBuf, funcFixed, funcDone, container, userdata);

	return CELL_OK;
}

s32 cellSaveDataUserListSave(
	PPUThread& CPU,
	u32 version,
	u32 userId,
	vm::ptr<CellSaveDataSetList> setList,
	vm::ptr<CellSaveDataSetBuf> setBuf,
	vm::ptr<CellSaveDataListCallback> funcList,
	vm::ptr<CellSaveDataStatCallback> funcStat,
	vm::ptr<CellSaveDataFileCallback> funcFile,
	u32 container,
	vm::ptr<void> userdata)
{
	cellSysutil.Todo("cellSaveDataUserListSave(version=%d, userId=%d, setList=*0x%x, setBuf=*0x%x, funcList=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, setList, setBuf, funcList, funcStat, funcFile, container, userdata);

	return CELL_OK;
}

s32 cellSaveDataUserListLoad(
	PPUThread& CPU,
	u32 version,
	u32 userId,
	vm::ptr<CellSaveDataSetList> setList,
	vm::ptr<CellSaveDataSetBuf> setBuf,
	vm::ptr<CellSaveDataListCallback> funcList,
	vm::ptr<CellSaveDataStatCallback> funcStat,
	vm::ptr<CellSaveDataFileCallback> funcFile,
	u32 container,
	vm::ptr<void> userdata)
{
	cellSysutil.Todo("cellSaveDataUserListLoad(version=%d, userId=%d, setList=*0x%x, setBuf=*0x%x, funcList=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, setList, setBuf, funcList, funcStat, funcFile, container, userdata);

	return CELL_OK;
}

s32 cellSaveDataUserFixedSave(
	PPUThread& CPU,
	u32 version,
	u32 userId,
	vm::ptr<CellSaveDataSetList> setList,
	vm::ptr<CellSaveDataSetBuf> setBuf,
	vm::ptr<CellSaveDataFixedCallback> funcFixed,
	vm::ptr<CellSaveDataStatCallback> funcStat,
	vm::ptr<CellSaveDataFileCallback> funcFile,
	u32 container,
	vm::ptr<void> userdata)
{
	cellSysutil.Todo("cellSaveDataUserFixedSave(version=%d, userId=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return CELL_OK;
}

s32 cellSaveDataUserFixedLoad(
	PPUThread& CPU,
	u32 version,
	u32 userId,
	vm::ptr<CellSaveDataSetList> setList,
	vm::ptr<CellSaveDataSetBuf> setBuf,
	vm::ptr<CellSaveDataFixedCallback> funcFixed,
	vm::ptr<CellSaveDataStatCallback> funcStat,
	vm::ptr<CellSaveDataFileCallback> funcFile,
	u32 container,
	vm::ptr<void> userdata)
{
	cellSysutil.Todo("cellSaveDataUserFixedLoad(version=%d, userId=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return CELL_OK;
}

s32 cellSaveDataUserAutoSave(
	PPUThread& CPU,
	u32 version,
	u32 userId,
	vm::ptr<const char> dirName,
	u32 errDialog,
	vm::ptr<CellSaveDataSetBuf> setBuf,
	vm::ptr<CellSaveDataStatCallback> funcStat,
	vm::ptr<CellSaveDataFileCallback> funcFile,
	u32 container,
	vm::ptr<void> userdata)
{
	cellSysutil.Todo("cellSaveDataUserAutoSave(version=%d, userId=%d, dirName=*0x%x, errDialog=%d, setBuf=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, dirName, errDialog, setBuf, funcStat, funcFile, container, userdata);

	return CELL_OK;
}

s32 cellSaveDataUserAutoLoad(
	PPUThread& CPU,
	u32 version,
	u32 userId,
	vm::ptr<const char> dirName,
	u32 errDialog,
	vm::ptr<CellSaveDataSetBuf> setBuf,
	vm::ptr<CellSaveDataStatCallback> funcStat,
	vm::ptr<CellSaveDataFileCallback> funcFile,
	u32 container,
	vm::ptr<void> userdata)
{
	cellSysutil.Todo("cellSaveDataUserAutoLoad(version=%d, userId=%d, dirName=*0x%x, errDialog=%d, setBuf=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, dirName, errDialog, setBuf, funcStat, funcFile, container, userdata);

	return CELL_OK;
}

s32 cellSaveDataUserListAutoSave(
	PPUThread& CPU,
	u32 version,
	u32 userId,
	u32 errDialog,
	vm::ptr<CellSaveDataSetList> setList,
	vm::ptr<CellSaveDataSetBuf> setBuf,
	vm::ptr<CellSaveDataFixedCallback> funcFixed,
	vm::ptr<CellSaveDataStatCallback> funcStat,
	vm::ptr<CellSaveDataFileCallback> funcFile,
	u32 container,
	vm::ptr<void> userdata)
{
	cellSysutil.Todo("cellSaveDataUserListAutoSave(version=%d, userId=%d, errDialog=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, errDialog, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return CELL_OK;
}

s32 cellSaveDataUserListAutoLoad(
	PPUThread& CPU,
	u32 version,
	u32 userId,
	u32 errDialog,
	vm::ptr<CellSaveDataSetList> setList,
	vm::ptr<CellSaveDataSetBuf> setBuf,
	vm::ptr<CellSaveDataFixedCallback> funcFixed,
	vm::ptr<CellSaveDataStatCallback> funcStat,
	vm::ptr<CellSaveDataFileCallback> funcFile,
	u32 container,
	vm::ptr<void> userdata)
{
	cellSysutil.Todo("cellSaveDataUserListAutoLoad(version=%d, userId=%d, errDialog=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcStat=*0x%x, funcFile=*0x%x, container=0x%x, userdata=*0x%x)",
		version, userId, errDialog, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata);

	return CELL_OK;
}

s32 cellSaveDataUserFixedDelete(
	PPUThread& CPU,
	u32 userId,
	vm::ptr<CellSaveDataSetList> setList,
	vm::ptr<CellSaveDataSetBuf> setBuf,
	vm::ptr<CellSaveDataFixedCallback> funcFixed,
	vm::ptr<CellSaveDataDoneCallback> funcDone,
	u32 container,
	vm::ptr<void> userdata)
{
	cellSysutil.Todo("cellSaveDataUserFixedDelete(userId=%d, setList=*0x%x, setBuf=*0x%x, funcFixed=*0x%x, funcDone=*0x%x, container=0x%x, userdata=*0x%x)",
		userId, setList, setBuf, funcFixed, funcDone, container, userdata);

	return CELL_OK;
}

void cellSaveDataEnableOverlay(s32 enable)
{
	cellSysutil.Todo("cellSaveDataEnableOverlay(enable=%d)", enable);

	return;
}


// Functions (Extensions) 
s32 cellSaveDataListDelete(
	PPUThread& CPU,
	vm::ptr<CellSaveDataSetList> setList,
	vm::ptr<CellSaveDataSetBuf> setBuf,
	vm::ptr<CellSaveDataListCallback> funcList,
	vm::ptr<CellSaveDataDoneCallback> funcDone,
	u32 container,
	vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSysutil);

	return CELL_OK;
}

s32 cellSaveDataListImport(
	PPUThread& CPU,
	vm::ptr<CellSaveDataSetList> setList,
	u32 maxSizeKB,
	vm::ptr<CellSaveDataDoneCallback> funcDone,
	u32 container,
	vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSysutil);

	return CELL_OK;
}

s32 cellSaveDataListExport(
	PPUThread& CPU,
	vm::ptr<CellSaveDataSetList> setList,
	u32 maxSizeKB,
	vm::ptr<CellSaveDataDoneCallback> funcDone,
	u32 container,
	vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSysutil);

	return CELL_OK;
}

s32 cellSaveDataFixedImport(
	PPUThread& CPU,
	vm::ptr<const char> dirName,
	u32 maxSizeKB,
	vm::ptr<CellSaveDataDoneCallback> funcDone,
	u32 container,
	vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSysutil);

	return CELL_OK;
}

s32 cellSaveDataFixedExport(
	PPUThread& CPU,
	vm::ptr<const char> dirName,
	u32 maxSizeKB,
	vm::ptr<CellSaveDataDoneCallback> funcDone,
	u32 container,
	vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSysutil);

	return CELL_OK;
}

s32 cellSaveDataGetListItem(
	vm::ptr<const char> dirName,
	vm::ptr<CellSaveDataDirStat> dir,
	vm::ptr<CellSaveDataSystemFileParam> sysFileParam,
	vm::ptr<u32> bind,
	vm::ptr<u32> sizeKB)
{
	UNIMPLEMENTED_FUNC(cellSysutil);

	return CELL_OK;
}

s32 cellSaveDataUserListDelete(
	PPUThread& CPU,
	u32 userId,
	vm::ptr<CellSaveDataSetList> setList,
	vm::ptr<CellSaveDataSetBuf> setBuf,
	vm::ptr<CellSaveDataListCallback> funcList,
	vm::ptr<CellSaveDataDoneCallback> funcDone,
	u32 container,
	vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSysutil);

	return CELL_OK;
}

s32 cellSaveDataUserListImport(
	PPUThread& CPU,
	u32 userId,
	vm::ptr<CellSaveDataSetList> setList,
	u32 maxSizeKB,
	vm::ptr<CellSaveDataDoneCallback> funcDone,
	u32 container,
	vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSysutil);

	return CELL_OK;
}

s32 cellSaveDataUserListExport(
	PPUThread& CPU,
	u32 userId,
	vm::ptr<CellSaveDataSetList> setList,
	u32 maxSizeKB,
	vm::ptr<CellSaveDataDoneCallback> funcDone,
	u32 container,
	vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSysutil);

	return CELL_OK;
}

s32 cellSaveDataUserFixedImport(
	PPUThread& CPU,
	u32 userId,
	vm::ptr<const char> dirName,
	u32 maxSizeKB,
	vm::ptr<CellSaveDataDoneCallback> funcDone,
	u32 container,
	vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSysutil);

	return CELL_OK;
}

s32 cellSaveDataUserFixedExport(
	PPUThread& CPU,
	u32 userId,
	vm::ptr<const char> dirName,
	u32 maxSizeKB,
	vm::ptr<CellSaveDataDoneCallback> funcDone,
	u32 container,
	vm::ptr<void> userdata)
{
	UNIMPLEMENTED_FUNC(cellSysutil);

	return CELL_OK;
}

s32 cellSaveDataUserGetListItem(
	u32 userId,
	vm::ptr<const char> dirName,
	vm::ptr<CellSaveDataDirStat> dir,
	vm::ptr<CellSaveDataSystemFileParam> sysFileParam,
	vm::ptr<u32> bind,
	vm::ptr<u32> sizeKB)
{
	UNIMPLEMENTED_FUNC(cellSysutil);

	return CELL_OK;
}

void cellSysutil_SaveData_init()
{
	// libsysutil functions:

	REG_FUNC(cellSysutil, cellSaveDataEnableOverlay);

	REG_FUNC(cellSysutil, cellSaveDataDelete2);
	//REG_FUNC(cellSysutil, cellSaveDataDelete);
	REG_FUNC(cellSysutil, cellSaveDataUserFixedDelete);
	REG_FUNC(cellSysutil, cellSaveDataFixedDelete);

	REG_FUNC(cellSysutil, cellSaveDataUserFixedLoad);
	REG_FUNC(cellSysutil, cellSaveDataUserFixedSave);
	REG_FUNC(cellSysutil, cellSaveDataFixedLoad2);
	REG_FUNC(cellSysutil, cellSaveDataFixedSave2);
	//REG_FUNC(cellSysutil, cellSaveDataFixedLoad);
	//REG_FUNC(cellSysutil, cellSaveDataFixedSave);

	REG_FUNC(cellSysutil, cellSaveDataUserListLoad);
	REG_FUNC(cellSysutil, cellSaveDataUserListSave);
	REG_FUNC(cellSysutil, cellSaveDataListLoad2);
	REG_FUNC(cellSysutil, cellSaveDataListSave2);
	//REG_FUNC(cellSysutil, cellSaveDataListLoad);
	//REG_FUNC(cellSysutil, cellSaveDataListSave);

	REG_FUNC(cellSysutil, cellSaveDataUserListAutoLoad);
	REG_FUNC(cellSysutil, cellSaveDataUserListAutoSave);
	REG_FUNC(cellSysutil, cellSaveDataListAutoLoad);
	REG_FUNC(cellSysutil, cellSaveDataListAutoSave);

	REG_FUNC(cellSysutil, cellSaveDataUserAutoLoad);
	REG_FUNC(cellSysutil, cellSaveDataUserAutoSave);
	REG_FUNC(cellSysutil, cellSaveDataAutoLoad2);
	REG_FUNC(cellSysutil, cellSaveDataAutoSave2);
	//REG_FUNC(cellSysutil, cellSaveDataAutoLoad);
	//REG_FUNC(cellSysutil, cellSaveDataAutoSave);

	// libsysutil_savedata functions:
	REG_FUNC(cellSysutil, cellSaveDataUserGetListItem);
	REG_FUNC(cellSysutil, cellSaveDataGetListItem);
	REG_FUNC(cellSysutil, cellSaveDataUserListDelete);
	REG_FUNC(cellSysutil, cellSaveDataListDelete);
	REG_FUNC(cellSysutil, cellSaveDataUserFixedExport);
	REG_FUNC(cellSysutil, cellSaveDataUserFixedImport);
	REG_FUNC(cellSysutil, cellSaveDataUserListExport);
	REG_FUNC(cellSysutil, cellSaveDataUserListImport);
	REG_FUNC(cellSysutil, cellSaveDataFixedExport);
	REG_FUNC(cellSysutil, cellSaveDataFixedImport);
	REG_FUNC(cellSysutil, cellSaveDataListExport);
	REG_FUNC(cellSysutil, cellSaveDataListImport);

	// libsysutil_savedata_psp functions:
}
