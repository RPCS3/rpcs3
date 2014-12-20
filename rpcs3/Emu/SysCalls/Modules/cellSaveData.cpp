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

extern Module *cellSysutil;

// Auxiliary Classes
class sortSaveDataEntry
{
	u32 sortType;
	u32 sortOrder;
public:
	sortSaveDataEntry(u32 type, u32 order) : sortType(type), sortOrder(order) {}
	bool operator()(const SaveDataEntry& entry1, const SaveDataEntry& entry2) const
	{
		if (sortOrder == CELL_SAVEDATA_SORTORDER_DESCENT)
		{
			if (sortType == CELL_SAVEDATA_SORTTYPE_MODIFIEDTIME)
				return entry1.st_mtime_ >= entry2.st_mtime_;
			if (sortType == CELL_SAVEDATA_SORTTYPE_SUBTITLE)
				return entry1.subtitle >= entry2.subtitle;
		}
		if (sortOrder == CELL_SAVEDATA_SORTORDER_ASCENT)
		{
			if (sortType == CELL_SAVEDATA_SORTTYPE_MODIFIEDTIME)
				return entry1.st_mtime_ < entry2.st_mtime_;
			if (sortType == CELL_SAVEDATA_SORTTYPE_SUBTITLE)
				return entry1.subtitle < entry2.subtitle;
		}
		return true;
	}
};


// Auxiliary Functions
u64 getSaveDataSize(const std::string& dirName)
{
	vfsDir dir(dirName);
	if (!dir.IsOpened())
		return 0;

	u64 totalSize = 0;
	for(const DirEntryInfo* entry = dir.Read(); entry; entry = dir.Read()) {
		if (entry->flags & DirEntry_TypeFile) {
			vfsFile file(dirName+"/"+entry->name);
			totalSize += file.GetSize();
		}
	}
	return totalSize;
}

void addSaveDataEntry(std::vector<SaveDataEntry>& saveEntries, const std::string& saveDir)
{
	// PSF parameters
	vfsFile f(saveDir + "/PARAM.SFO");
	PSFLoader psf(f);
	if(!psf.Load(false))
		return;

	// PNG icon
	std::string localPath;
	Emu.GetVFS().GetDevice(saveDir + "/ICON0.PNG", localPath);

	SaveDataEntry saveEntry;
	saveEntry.dirName = psf.GetString("SAVEDATA_DIRECTORY");
	saveEntry.listParam = psf.GetString("SAVEDATA_LIST_PARAM");
	saveEntry.title = psf.GetString("TITLE");
	saveEntry.subtitle = psf.GetString("SUB_TITLE");
	saveEntry.details = psf.GetString("DETAIL");
	saveEntry.sizeKB = (u32)(getSaveDataSize(saveDir) / 1024);
	saveEntry.st_atime_ = 0; // TODO
	saveEntry.st_mtime_ = 0; // TODO
	saveEntry.st_ctime_ = 0; // TODO
	saveEntry.iconBuf = NULL; // TODO: Here should be the PNG buffer
	saveEntry.iconBufSize = 0; // TODO: Size of the PNG file
	saveEntry.isNew = false;

	saveEntries.push_back(saveEntry);
}

void addNewSaveDataEntry(std::vector<SaveDataEntry>& saveEntries, vm::ptr<CellSaveDataListNewData> newData)
{
	SaveDataEntry saveEntry;
	saveEntry.dirName = newData->dirName.get_ptr();
	saveEntry.title = newData->icon->title.get_ptr();
	saveEntry.subtitle = newData->icon->title.get_ptr();
	saveEntry.iconBuf = newData->icon->iconBuf.get_ptr();
	saveEntry.iconBufSize = newData->icon->iconBufSize;
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
		for (u32 j=0; j<fixedListNum; j++)
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
		saveEntries[0].iconBuf = fixedSet->newIcon->iconBuf.get_ptr();
		saveEntries[0].iconBufSize = fixedSet->newIcon->iconBufSize;
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
	statGet->fileList.set(be_t<u32>::make(0));
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

	statGet->fileList = vm::ptr<CellSaveDataFileStat>::make((u32)Memory.Alloc(sizeof(CellSaveDataFileStat) * fileEntries.size(), 8));
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
			cellSysutil->Error("modifySaveDataFiles: CellSaveDataFileCallback failed."); // TODO: Once we verify that the entire SysCall is working, delete this debug error message.
			return CELL_SAVEDATA_ERROR_CBRESULT;
		}
		if (result->result == CELL_SAVEDATA_CBRESULT_OK_LAST) {
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
			cellSysutil->Error("modifySaveDataFiles: Unknown fileType! Aborting...");
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
			cellSysutil->Warning("modifySaveDataFiles: File operation CELL_SAVEDATA_FILEOP_WRITE_NOTRUNC not yet implemented");
			break;

		default:
			cellSysutil->Error("modifySaveDataFiles: Unknown fileOperation! Aborting...");
			return CELL_SAVEDATA_ERROR_PARAM;
		}

		if (file && file->IsOpened())
			file->Close();
	}
	return CELL_SAVEDATA_RET_OK;
}


// Functions
int cellSaveDataListSave2(u32 version, vm::ptr<CellSaveDataSetList> setList, vm::ptr<CellSaveDataSetBuf> setBuf,
						  vm::ptr<CellSaveDataListCallback> funcList, vm::ptr<CellSaveDataStatCallback> funcStat, vm::ptr<CellSaveDataFileCallback> funcFile,
						  u32 container, vm::ptr<void> userdata)
{
	cellSysutil->Warning("cellSaveDataListSave2(version=%d, setList_addr=0x%x, setBuf_addr=0x%x, funcList_addr=0x%x, funcStat_addr=0x%x, funcFile_addr=0x%x, container=0x%x, userdata_addr=0x%x)",
		version, setList.addr(), setBuf.addr(), funcList.addr(), funcStat.addr(), funcFile.addr(), container, userdata.addr());

	vm::var<CellSaveDataCBResult> result;
	vm::var<CellSaveDataListGet> listGet;
	vm::var<CellSaveDataListSet> listSet;
	vm::var<CellSaveDataStatGet> statGet;
	vm::var<CellSaveDataStatSet> statSet;

	std::string saveBaseDir = "/dev_hdd0/home/00000001/savedata/"; // TODO: Get the path of the current user
	vfsDir dir(saveBaseDir);
	if(!dir.IsOpened())
		return CELL_SAVEDATA_ERROR_INTERNAL;

	std::string dirNamePrefix = setList->dirNamePrefix.get_ptr();
	std::vector<SaveDataEntry> saveEntries;

	for(const DirEntryInfo* entry = dir.Read(); entry; entry = dir.Read())
	{
		if (entry->flags & DirEntry_TypeDir && entry->name.substr(0,dirNamePrefix.size()) == dirNamePrefix)
		{
			// Count the amount of matches and the amount of listed directories
			listGet->dirListNum++;
			if (listGet->dirListNum > setBuf->dirListMax)
				continue;
			listGet->dirNum++;

			std::string saveDir = saveBaseDir + entry->name;
			addSaveDataEntry(saveEntries, saveDir);
		}
	}

	// Sort the entries and fill the listGet->dirList array
	std::sort(saveEntries.begin(), saveEntries.end(), sortSaveDataEntry(setList->sortType, setList->sortOrder));
	listGet->dirList = vm::bptr<CellSaveDataDirList>::make(setBuf->buf.addr());
	auto dirList = vm::get_ptr<CellSaveDataDirList>(listGet->dirList.addr());

	for (u32 i=0; i<saveEntries.size(); i++) {
		strcpy_trunc(dirList[i].dirName, saveEntries[i].dirName);
		strcpy_trunc(dirList[i].listParam, saveEntries[i].listParam);
		*dirList[i].reserved = {};
	}

	funcList(result, listGet, listSet);

	if (result->result < 0)	{
		cellSysutil->Error("cellSaveDataListSave2: CellSaveDataListCallback failed."); // TODO: Once we verify that the entire SysCall is working, delete this debug error message.
		return CELL_SAVEDATA_ERROR_CBRESULT;
	}

	setSaveDataList(saveEntries, vm::ptr<CellSaveDataDirList>::make(listSet->fixedList.addr()), listSet->fixedListNum);
	if (listSet->newData)
		addNewSaveDataEntry(saveEntries, vm::ptr<CellSaveDataListNewData>::make(listSet->newData.addr()));
	if (saveEntries.size() == 0) {
		cellSysutil->Warning("cellSaveDataListSave2: No save entries found!"); // TODO: Find a better way to handle this error
		return CELL_SAVEDATA_RET_OK;
	}

	u32 focusIndex = focusSaveDataEntry(saveEntries, listSet->focusPosition);
	// TODO: Display the dialog here
	u32 selectedIndex = focusIndex; // TODO: Until the dialog is implemented, select always the focused entry
	getSaveDataStat(saveEntries[selectedIndex], statGet);
	result->userdata = userdata;

	funcStat(result, statGet, statSet);
	Memory.Free(statGet->fileList.addr());
	if (result->result < 0)	{
		cellSysutil->Error("cellSaveDataListLoad2: CellSaveDataStatCallback failed."); // TODO: Once we verify that the entire SysCall is working, delete this debug error message.
		return CELL_SAVEDATA_ERROR_CBRESULT;
	}

	/*if (statSet->setParam)
		addNewSaveDataEntry(saveEntries, (u32)listSet->newData.addr()); // TODO: This *is* wrong
	*/

	// Enter the loop where the save files are read/created/deleted.
	s32 ret = modifySaveDataFiles(funcFile, result, saveBaseDir + (char*)statGet->dir.dirName);

	return ret;
}

int cellSaveDataListLoad2(u32 version, vm::ptr<CellSaveDataSetList> setList, vm::ptr<CellSaveDataSetBuf> setBuf,
						  vm::ptr<CellSaveDataListCallback> funcList, vm::ptr<CellSaveDataStatCallback> funcStat, vm::ptr<CellSaveDataFileCallback> funcFile,
						  u32 container, vm::ptr<void> userdata)
{
	cellSysutil->Warning("cellSaveDataListLoad2(version=%d, setList_addr=0x%x, setBuf_addr=0x%x, funcList_addr=0x%x, funcStat_addr=0x%x, funcFile_addr=0x%x, container=0x%x, userdata_addr=0x%x)",
		version, setList.addr(), setBuf.addr(), funcList.addr(), funcStat.addr(), funcFile.addr(), container, userdata.addr());

	vm::var<CellSaveDataCBResult> result;
	vm::var<CellSaveDataListGet> listGet;
	vm::var<CellSaveDataListSet> listSet;
	vm::var<CellSaveDataStatGet> statGet;
	vm::var<CellSaveDataStatSet> statSet;

	std::string saveBaseDir = "/dev_hdd0/home/00000001/savedata/"; // TODO: Get the path of the current user
	vfsDir dir(saveBaseDir);

	if(!dir.IsOpened())
		return CELL_SAVEDATA_ERROR_INTERNAL;

	std::string dirNamePrefix = setList->dirNamePrefix.get_ptr();
	std::vector<SaveDataEntry> saveEntries;

	for(const DirEntryInfo* entry = dir.Read(); entry; entry = dir.Read())
	{
		if (entry->flags & DirEntry_TypeDir && entry->name.substr(0,dirNamePrefix.size()) == dirNamePrefix)
		{
			// Count the amount of matches and the amount of listed directories
			listGet->dirListNum++;
			if (listGet->dirListNum > setBuf->dirListMax)
				continue;
			listGet->dirNum++;

			std::string saveDir = saveBaseDir + entry->name;
			addSaveDataEntry(saveEntries, saveDir);
		}
	}

	// Sort the entries and fill the listGet->dirList array
	std::sort(saveEntries.begin(), saveEntries.end(), sortSaveDataEntry(setList->sortType, setList->sortOrder));
	listGet->dirList = vm::bptr<CellSaveDataDirList>::make(setBuf->buf.addr());
	auto dirList = vm::get_ptr<CellSaveDataDirList>(listGet->dirList.addr());

	for (u32 i=0; i<saveEntries.size(); i++) {
		strcpy_trunc(dirList[i].dirName, saveEntries[i].dirName);
		strcpy_trunc(dirList[i].listParam, saveEntries[i].listParam);
		*dirList[i].reserved = {};
	}

	funcList(result, listGet, listSet);

	if (result->result < 0)	{
		cellSysutil->Error("cellSaveDataListLoad2: CellSaveDataListCallback failed."); // TODO: Once we verify that the entire SysCall is working, delete this debug error message.
		return CELL_SAVEDATA_ERROR_CBRESULT;
	}

	setSaveDataList(saveEntries, vm::ptr<CellSaveDataDirList>::make(listSet->fixedList.addr()), listSet->fixedListNum);
	if (listSet->newData)
		addNewSaveDataEntry(saveEntries, vm::ptr<CellSaveDataListNewData>::make(listSet->newData.addr()));
	if (saveEntries.size() == 0) {
		cellSysutil->Warning("cellSaveDataListLoad2: No save entries found!"); // TODO: Find a better way to handle this error
		return CELL_SAVEDATA_RET_OK;
	}

	u32 focusIndex = focusSaveDataEntry(saveEntries, listSet->focusPosition);
	// TODO: Display the dialog here
	u32 selectedIndex = focusIndex; // TODO: Until the dialog is implemented, select always the focused entry
	getSaveDataStat(saveEntries[selectedIndex], statGet);
	result->userdata = userdata;

	funcStat(result, statGet, statSet);
	Memory.Free(statGet->fileList.addr());
	if (result->result < 0)	{
		cellSysutil->Error("cellSaveDataListLoad2: CellSaveDataStatCallback failed."); // TODO: Once we verify that the entire SysCall is working, delete this debug error message.
		return CELL_SAVEDATA_ERROR_CBRESULT;
	}

	/*if (statSet->setParam)
		// TODO: Write PARAM.SFO file
	*/

	// Enter the loop where the save files are read/created/deleted.
	s32 ret = modifySaveDataFiles(funcFile, result, saveBaseDir + (char*)statGet->dir.dirName);

	return ret;
}

int cellSaveDataFixedSave2(u32 version,  vm::ptr<CellSaveDataSetList> setList, vm::ptr<CellSaveDataSetBuf> setBuf,
						   vm::ptr<CellSaveDataFixedCallback> funcFixed, vm::ptr<CellSaveDataStatCallback> funcStat, vm::ptr<CellSaveDataFileCallback> funcFile,
						   u32 container, vm::ptr<void> userdata)
{
	cellSysutil->Warning("cellSaveDataFixedSave2(version=%d, setList_addr=0x%x, setBuf_addr=0x%x, funcFixed_addr=0x%x, funcStat_addr=0x%x, funcFile_addr=0x%x, container=0x%x, userdata_addr=0x%x)",
		version, setList.addr(), setBuf.addr(), funcFixed.addr(), funcStat.addr(), funcFile.addr(), container, userdata.addr());

	vm::var<CellSaveDataCBResult> result;
	vm::var<CellSaveDataListGet> listGet;
	vm::var<CellSaveDataFixedSet> fixedSet;
	vm::var<CellSaveDataStatGet> statGet;
	vm::var<CellSaveDataStatSet> statSet;

	std::string saveBaseDir = "/dev_hdd0/home/00000001/savedata/"; // TODO: Get the path of the current user
	vfsDir dir(saveBaseDir);
	if (!dir.IsOpened())
		return CELL_SAVEDATA_ERROR_INTERNAL;

	std::string dirNamePrefix = setList->dirNamePrefix.get_ptr();
	std::vector<SaveDataEntry> saveEntries;
	for (const DirEntryInfo* entry = dir.Read(); entry; entry = dir.Read())
	{
		if (entry->flags & DirEntry_TypeDir && entry->name.substr(0, dirNamePrefix.size()) == dirNamePrefix)
		{
			// Count the amount of matches and the amount of listed directories
			listGet->dirListNum++;
			if (listGet->dirListNum > setBuf->dirListMax)
				continue;
			listGet->dirNum++;

			std::string saveDir = saveBaseDir + entry->name;
			addSaveDataEntry(saveEntries, saveDir);
		}
	}

	// Sort the entries and fill the listGet->dirList array
	std::sort(saveEntries.begin(), saveEntries.end(), sortSaveDataEntry(setList->sortType, setList->sortOrder));
	listGet->dirList = vm::bptr<CellSaveDataDirList>::make(setBuf->buf.addr());
	auto dirList = vm::get_ptr<CellSaveDataDirList>(listGet->dirList.addr());
	for (u32 i = 0; i<saveEntries.size(); i++) {
		strcpy_trunc(dirList[i].dirName, saveEntries[i].dirName);
		strcpy_trunc(dirList[i].listParam, saveEntries[i].listParam);
		*dirList[i].reserved = {};
	}
	funcFixed(result, listGet, fixedSet);
	if (result->result < 0)	{
		cellSysutil->Error("cellSaveDataFixedSave2: CellSaveDataFixedCallback failed."); // TODO: Once we verify that the entire SysCall is working, delete this debug error message.
		return CELL_SAVEDATA_ERROR_CBRESULT;
	}
	setSaveDataFixed(saveEntries, fixedSet);
	getSaveDataStat(saveEntries[0], statGet); // There should be only one element in this list
	// TODO: Display the Yes|No dialog here
	result->userdata = userdata;

	funcStat(result, statGet, statSet);
	Memory.Free(statGet->fileList.addr());
	if (result->result < 0)	{
		cellSysutil->Error("cellSaveDataFixedSave2: CellSaveDataStatCallback failed."); // TODO: Once we verify that the entire SysCall is working, delete this debug error message.
		return CELL_SAVEDATA_ERROR_CBRESULT;
	}
	/*if (statSet->setParam)
		// TODO: Write PARAM.SFO file
	*/

	// Enter the loop where the save files are read/created/deleted.
	s32 ret = modifySaveDataFiles(funcFile, result, saveBaseDir + (char*)statGet->dir.dirName);

	return ret;
}

int cellSaveDataFixedLoad2(u32 version,  vm::ptr<CellSaveDataSetList> setList, vm::ptr<CellSaveDataSetBuf> setBuf,
						   vm::ptr<CellSaveDataFixedCallback> funcFixed, vm::ptr<CellSaveDataStatCallback> funcStat, vm::ptr<CellSaveDataFileCallback> funcFile,
						   u32 container, vm::ptr<void> userdata)
{
	cellSysutil->Warning("cellSaveDataFixedLoad2(version=%d, setList_addr=0x%x, setBuf=0x%x, funcList=0x%x, funcStat=0x%x, funcFile=0x%x, container=0x%x, userdata_addr=0x%x)",
		version, setList.addr(), setBuf.addr(), funcFixed.addr(), funcStat.addr(), funcFile.addr(), container, userdata.addr());

	vm::var<CellSaveDataCBResult> result;
	vm::var<CellSaveDataListGet> listGet;
	vm::var<CellSaveDataFixedSet> fixedSet;
	vm::var<CellSaveDataStatGet> statGet;
	vm::var<CellSaveDataStatSet> statSet;

	std::string saveBaseDir = "/dev_hdd0/home/00000001/savedata/"; // TODO: Get the path of the current user
	vfsDir dir(saveBaseDir);
	if (!dir.IsOpened())
		return CELL_SAVEDATA_ERROR_INTERNAL;

	std::string dirNamePrefix = setList->dirNamePrefix.get_ptr();
	std::vector<SaveDataEntry> saveEntries;
	for (const DirEntryInfo* entry = dir.Read(); entry; entry = dir.Read())
	{
		if (entry->flags & DirEntry_TypeDir && entry->name.substr(0, dirNamePrefix.size()) == dirNamePrefix)
		{
			// Count the amount of matches and the amount of listed directories
			listGet->dirListNum++;
			if (listGet->dirListNum > setBuf->dirListMax)
				continue;
			listGet->dirNum++;

			std::string saveDir = saveBaseDir + entry->name;
			addSaveDataEntry(saveEntries, saveDir);
		}
	}

	// Sort the entries and fill the listGet->dirList array
	std::sort(saveEntries.begin(), saveEntries.end(), sortSaveDataEntry(setList->sortType, setList->sortOrder));
	listGet->dirList = vm::bptr<CellSaveDataDirList>::make(setBuf->buf.addr());
	auto dirList = vm::get_ptr<CellSaveDataDirList>(listGet->dirList.addr());
	for (u32 i = 0; i<saveEntries.size(); i++) {
		strcpy_trunc(dirList[i].dirName, saveEntries[i].dirName);
		strcpy_trunc(dirList[i].listParam, saveEntries[i].listParam);
		*dirList[i].reserved = {};
	}
	funcFixed(result, listGet, fixedSet);
	if (result->result < 0)	{
		cellSysutil->Error("cellSaveDataFixedLoad2: CellSaveDataFixedCallback failed."); // TODO: Once we verify that the entire SysCall is working, delete this debug error message.
		return CELL_SAVEDATA_ERROR_CBRESULT;
	}
	setSaveDataFixed(saveEntries, fixedSet);
	getSaveDataStat(saveEntries[0], statGet); // There should be only one element in this list
	// TODO: Display the Yes|No dialog here
	result->userdata = userdata;

	funcStat(result, statGet, statSet);
	Memory.Free(statGet->fileList.addr());
	if (result->result < 0)	{
		cellSysutil->Error("cellSaveDataFixedLoad2: CellSaveDataStatCallback failed."); // TODO: Once we verify that the entire SysCall is working, delete this debug error message.
		return CELL_SAVEDATA_ERROR_CBRESULT;
	}
	/*if (statSet->setParam)
		// TODO: Write PARAM.SFO file
	*/

	// Enter the loop where the save files are read/created/deleted.
	s32 ret = modifySaveDataFiles(funcFile, result, saveBaseDir + (char*)statGet->dir.dirName);

	return ret;
}

int cellSaveDataAutoSave2(u32 version, vm::ptr<const char> dirName, u32 errDialog, vm::ptr<CellSaveDataSetBuf> setBuf,
						  vm::ptr<CellSaveDataStatCallback> funcStat, vm::ptr<CellSaveDataFileCallback> funcFile,
						  u32 container, vm::ptr<void> userdata)
{
	cellSysutil->Warning("cellSaveDataAutoSave2(version=%d, dirName_addr=0x%x, errDialog=%d, setBuf=0x%x, funcList=0x%x, funcStat=0x%x, funcFile=0x%x, container=0x%x, userdata_addr=0x%x)",
		version, dirName.addr(), errDialog, setBuf.addr(), funcStat.addr(), funcFile.addr(), container, userdata.addr());

	vm::var<CellSaveDataCBResult> result;
	vm::var<CellSaveDataStatGet> statGet;
	vm::var<CellSaveDataStatSet> statSet;

	std::string saveBaseDir = "/dev_hdd0/home/00000001/savedata/"; // TODO: Get the path of the current user
	vfsDir dir(saveBaseDir);
	if (!dir.IsOpened())
		return CELL_SAVEDATA_ERROR_INTERNAL;

	std::string dirN = dirName.get_ptr();
	std::vector<SaveDataEntry> saveEntries;
	for (const DirEntryInfo* entry = dir.Read(); entry; entry = dir.Read())
	{
		if (entry->flags & DirEntry_TypeDir && entry->name == dirN) {
			addSaveDataEntry(saveEntries, saveBaseDir + dirN);
		}
	}

	// The target entry does not exist
	if (saveEntries.size() == 0) {
		SaveDataEntry entry;
		entry.dirName = dirN;
		entry.sizeKB = 0;
		entry.isNew = true;
		saveEntries.push_back(entry);
	}

	getSaveDataStat(saveEntries[0], statGet); // There should be only one element in this list
	result->userdata = userdata;
	funcStat(result, statGet, statSet);

	if (statGet->fileList)
		Memory.Free(statGet->fileList.addr());

	if (result->result < 0)	{
		cellSysutil->Error("cellSaveDataAutoSave2: CellSaveDataStatCallback failed."); // TODO: Once we verify that the entire SysCall is working, delete this debug error message.
		return CELL_SAVEDATA_ERROR_CBRESULT;
	}
	/*if (statSet->setParam)
		// TODO: Write PARAM.SFO file
	*/

	// Enter the loop where the save files are read/created/deleted.
	s32 ret = modifySaveDataFiles(funcFile, result, saveBaseDir + (char*)statGet->dir.dirName);

	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataAutoLoad2(u32 version, vm::ptr<const char> dirName, u32 errDialog, vm::ptr<CellSaveDataSetBuf> setBuf,
						  vm::ptr<CellSaveDataStatCallback> funcStat, vm::ptr<CellSaveDataFileCallback> funcFile,
						  u32 container, vm::ptr<void> userdata)
{
	cellSysutil->Warning("cellSaveDataAutoLoad2(version=%d, dirName_addr=0x%x, errDialog=%d, setBuf=0x%x, funcList=0x%x, funcStat=0x%x, funcFile=0x%x, container=0x%x, userdata_addr=0x%x)",
		version, dirName.addr(), errDialog, setBuf.addr(), funcStat.addr(), funcFile.addr(), container, userdata.addr());

	vm::var<CellSaveDataCBResult> result;
	vm::var<CellSaveDataStatGet> statGet;
	vm::var<CellSaveDataStatSet> statSet;

	std::string saveBaseDir = "/dev_hdd0/home/00000001/savedata/"; // TODO: Get the path of the current user
	vfsDir dir(saveBaseDir);
	if (!dir.IsOpened())
		return CELL_SAVEDATA_ERROR_INTERNAL;

	std::string dirN = dirName.get_ptr();
	std::vector<SaveDataEntry> saveEntries;
	for (const DirEntryInfo* entry = dir.Read(); entry; entry = dir.Read())
	{
		if (entry->flags & DirEntry_TypeDir && entry->name == dirN) {
			addSaveDataEntry(saveEntries, saveBaseDir + dirN);
		}
	}

	// The target entry does not exist
	if (saveEntries.size() == 0) {
		cellSysutil->Warning("cellSaveDataAutoLoad2: Couldn't find save entry (%s)", dirN.c_str());
		return CELL_OK; // TODO: Can anyone check the actual behaviour of a PS3 when saves are not found?
	}

	getSaveDataStat(saveEntries[0], statGet); // There should be only one element in this list
	result->userdata = userdata;
	funcStat(result, statGet, statSet);

	Memory.Free(statGet->fileList.addr());
	if (result->result < 0)	{
		cellSysutil->Error("cellSaveDataAutoLoad2: CellSaveDataStatCallback failed."); // TODO: Once we verify that the entire SysCall is working, delete this debug error message.
		return CELL_SAVEDATA_ERROR_CBRESULT;
	}
	/*if (statSet->setParam)
		// TODO: Write PARAM.SFO file
	*/

	// Enter the loop where the save files are read/created/deleted.
	s32 ret = modifySaveDataFiles(funcFile, result, saveBaseDir + (char*)statGet->dir.dirName);

	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataListAutoSave(u32 version, u32 errDialog, vm::ptr<CellSaveDataSetList> setList, vm::ptr<CellSaveDataSetBuf> setBuf, vm::ptr<CellSaveDataFixedCallback> funcFixed,
							 vm::ptr<CellSaveDataStatCallback> funcStat, vm::ptr<CellSaveDataFileCallback> funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSysutil->Todo("cellSaveDataListAutoSave(version=%d, errDialog=%d, setBuf=0x%x, funcFixed=0x%x, funcStat=0x%x, funcFile=0x%x, container=0x%x, userdata_addr=0x%x)",
		version, errDialog, setList.addr(), setBuf.addr(), funcFixed.addr(), funcStat.addr(), funcFile.addr(), container, userdata.addr());

	//vm::var<CellSaveDataCBResult> result;
	//vm::var<CellSaveDataListGet> listGet;
	//vm::var<CellSaveDataFixedSet> fixedSet;
	//vm::var<CellSaveDataStatGet> statGet;
	//vm::var<CellSaveDataStatSet> statSet;

	//std::string saveBaseDir = "/dev_hdd0/home/00000001/savedata/"; // TODO: Get the path of the current user
	//vfsDir dir(saveBaseDir);

	//if (!dir.IsOpened())
	//	return CELL_SAVEDATA_ERROR_INTERNAL;

	//std::string dirNamePrefix = setList->dirNamePrefix.get_ptr();
	//std::vector<SaveDataEntry> saveEntries;

	//for (const DirEntryInfo* entry = dir.Read(); entry; entry = dir.Read())
	//{
	//	if (entry->flags & DirEntry_TypeDir && entry->name.substr(0, dirNamePrefix.size()) == dirNamePrefix)
	//	{
	//		// Count the amount of matches and the amount of listed directories
	//		listGet->dirListNum++;
	//		if (listGet->dirListNum > setBuf->dirListMax)
	//			continue;
	//		listGet->dirNum++;

	//		std::string saveDir = saveBaseDir + entry->name;
	//		addSaveDataEntry(saveEntries, saveDir);
	//	}
	//}

	//// Sort the entries and fill the listGet->dirList array
	//std::sort(saveEntries.begin(), saveEntries.end(), sortSaveDataEntry(setList->sortType, setList->sortOrder));
	//listGet->dirList = vm::bptr<CellSaveDataDirList>::make(setBuf->buf.addr());
	//auto dirList = vm::get_ptr<CellSaveDataDirList>(listGet->dirList.addr());

	//for (u32 i = 0; i<saveEntries.size(); i++) {
	//	strcpy_trunc(dirList[i].dirName, saveEntries[i].dirName.c_str());
	//	strcpy_trunc(dirList[i].listParam, saveEntries[i].listParam.c_str());
	//	*dirList[i].reserved = {};
	//}

	//funcFixed(result, listGet, fixedSet);

	//if (result->result < 0)	{
	//	cellSysutil->Error("cellSaveDataListAutoSave: CellSaveDataListCallback failed."); // TODO: Once we verify that the entire SysCall is working, delete this debug error message.
	//	return CELL_SAVEDATA_ERROR_CBRESULT;
	//}

	//setSaveDataList(saveEntries, (u32)listSet->fixedList.addr(), listSet->fixedListNum);
	//if (listSet->newData)
	//	addNewSaveDataEntry(saveEntries, (u32)listSet->newData.addr());
	//if (saveEntries.size() == 0) {
	//	cellSysutil->Warning("cellSaveDataListAutoSave: No save entries found!"); // TODO: Find a better way to handle this error
	//	return CELL_SAVEDATA_RET_OK;
	//}

	//u32 focusIndex = focusSaveDataEntry(saveEntries, listSet->focusPosition);
	//// TODO: Display the dialog here
	//u32 selectedIndex = focusIndex; // TODO: Until the dialog is implemented, select always the focused entry
	//getSaveDataStat(saveEntries[selectedIndex], statGet.addr());
	//result->userdata_addr = userdata_addr;

	//funcStat(result, statGet, statSet);
	//Memory.Free(statGet->fileList.addr());
	//if (result->result < 0)	{
	//	cellSysutil->Error("cellSaveDataListAutoSave: CellSaveDataStatCallback failed."); // TODO: Once we verify that the entire SysCall is working, delete this debug error message.
	//	return CELL_SAVEDATA_ERROR_CBRESULT;
	//}

	///*if (statSet->setParam)
	//addNewSaveDataEntry(saveEntries, (u32)listSet->newData.addr()); // TODO: This *is* wrong
	//*/

	//// Enter the loop where the save files are read/created/deleted.
	//s32 ret = modifySaveDataFiles(funcFile, result, saveBaseDir + (char*)statGet->dir.dirName);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataListAutoLoad(u32 version, u32 errDialog, vm::ptr<CellSaveDataSetList> setList, vm::ptr<CellSaveDataSetBuf> setBuf, vm::ptr<CellSaveDataFixedCallback> funcFixed,
							 vm::ptr<CellSaveDataStatCallback> funcStat, vm::ptr<CellSaveDataFileCallback> funcFile, u32 container, vm::ptr<void> userdata)
{
	cellSysutil->Todo("cellSaveDataListAutoLoad(version=%d, errDialog=%d, setBuf=0x%x, funcFixed=0x%x, funcStat=0x%x, funcFile=0x%x, container=0x%x, userdata_addr=0x%x)",
					  version, errDialog, setList.addr(), setBuf.addr(), funcFixed.addr(), funcStat.addr(), funcFile.addr(), container, userdata.addr());

	//vm::var<CellSaveDataCBResult> result;
	//vm::var<CellSaveDataListGet> listGet;
	//vm::var<CellSaveDataFixedSet> fixedSet;
	//vm::var<CellSaveDataStatGet> statGet;
	//vm::var<CellSaveDataStatSet> statSet;

	//std::string saveBaseDir = "/dev_hdd0/home/00000001/savedata/"; // TODO: Get the path of the current user
	//vfsDir dir(saveBaseDir);

	//if (!dir.IsOpened())
	//	return CELL_SAVEDATA_ERROR_INTERNAL;

	//std::string dirNamePrefix = setList->dirNamePrefix.get_ptr();
	//std::vector<SaveDataEntry> saveEntries;

	//for (const DirEntryInfo* entry = dir.Read(); entry; entry = dir.Read())
	//{
	//	if (entry->flags & DirEntry_TypeDir && entry->name.substr(0, dirNamePrefix.size()) == dirNamePrefix)
	//	{
	//		// Count the amount of matches and the amount of listed directories
	//		listGet->dirListNum++;
	//		if (listGet->dirListNum > setBuf->dirListMax)
	//			continue;
	//		listGet->dirNum++;

	//		std::string saveDir = saveBaseDir + entry->name;
	//		addSaveDataEntry(saveEntries, saveDir);
	//	}
	//}

	//// Sort the entries and fill the listGet->dirList array
	//std::sort(saveEntries.begin(), saveEntries.end(), sortSaveDataEntry(setList->sortType, setList->sortOrder));
	//listGet->dirList = vm::bptr<CellSaveDataDirList>::make(setBuf->buf.addr());
	//auto dirList = vm::get_ptr<CellSaveDataDirList>(listGet->dirList.addr());

	//for (u32 i = 0; i<saveEntries.size(); i++) {
	//	strcpy_trunc(dirList[i].dirName, saveEntries[i].dirName);
	//	strcpy_trunc(dirList[i].listParam, saveEntries[i].listParam);
	//	*dirList[i].reserved = {};
	//}

	//funcFixed(result, listGet, fixedSet);

	//if (result->result < 0)	{
	//	cellSysutil->Error("cellSaveDataListAutoLoad: CellSaveDataListCallback failed."); // TODO: Once we verify that the entire SysCall is working, delete this debug error message.
	//	return CELL_SAVEDATA_ERROR_CBRESULT;
	//}

	//setSaveDataList(saveEntries, (u32)listSet->fixedList.addr(), listSet->fixedListNum);
	//if (listSet->newData)
	//	addNewSaveDataEntry(saveEntries, (u32)listSet->newData.addr());
	//if (saveEntries.size() == 0) {
	//	cellSysutil->Warning("cellSaveDataListAutoLoad: No save entries found!"); // TODO: Find a better way to handle this error
	//	return CELL_SAVEDATA_RET_OK;
	//}

	//u32 focusIndex = focusSaveDataEntry(saveEntries, listSet->focusPosition);
	//// TODO: Display the dialog here
	//u32 selectedIndex = focusIndex; // TODO: Until the dialog is implemented, select always the focused entry
	//getSaveDataStat(saveEntries[selectedIndex], statGet.addr());
	//result->userdata_addr = userdata_addr;

	//funcStat(result.addr(), statGet.addr(), statSet.addr());
	//Memory.Free(statGet->fileList.addr());

	//if (result->result < 0)	{
	//	cellSysutil->Error("cellSaveDataListAutoLoad: CellSaveDataStatCallback failed."); // TODO: Once we verify that the entire SysCall is working, delete this debug error message.
	//	return CELL_SAVEDATA_ERROR_CBRESULT;
	//}

	///*if (statSet->setParam)
	//// TODO: Write PARAM.SFO file
	//*/

	//// Enter the loop where the save files are read/created/deleted.
	//s32 ret = modifySaveDataFiles(funcFile, result, saveBaseDir + (char*)statGet->dir.dirName);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataDelete2(u32 container)
{	 
	cellSysutil->Todo("cellSaveDataDelete2(container=0x%x)", container);
	return CELL_SAVEDATA_RET_CANCEL;
}

int cellSaveDataFixedDelete(vm::ptr<CellSaveDataSetList> setList, vm::ptr<CellSaveDataSetBuf> setBuf, vm::ptr<CellSaveDataFixedCallback> funcFixed, vm::ptr<CellSaveDataDoneCallback> funcDone,
							u32 container, u32 userdata_addr)
{
	cellSysutil->Todo("cellSaveDataFixedDelete(setList=0x%x, setBuf=0x%x, funcFixed=0x%x, funcDone=0x%x, container=0x%x, userdata_addr=0x%x)", setList.addr(), setBuf.addr(), funcFixed.addr(),
					  funcDone.addr(), container, userdata_addr);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserListSave(u32 version, u32 userId, vm::ptr<CellSaveDataSetList> setList, vm::ptr<CellSaveDataSetBuf> setBuf, vm::ptr<CellSaveDataListCallback> funcList,
							 vm::ptr<CellSaveDataStatCallback> funcStat, vm::ptr<CellSaveDataFileCallback> funcFile, u32 container, u32 userdata_addr)
{
	cellSysutil->Todo("cellSaveDataUserListSave(version=%d, userId=%d, setList=0x%x, setBuf=0x%x, funcList=0x%x, funcStat=0x%x, funcFile=0x%x, container=0x%x, userdata_addr=0x%x)", version,
					  userId, setList.addr(), setBuf.addr(), funcList.addr(), funcStat.addr(), funcFile.addr(), container, userdata_addr);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserListLoad(u32 version, u32 userId, vm::ptr<CellSaveDataSetList> setList, vm::ptr<CellSaveDataSetBuf> setBuf, vm::ptr<CellSaveDataListCallback> funcList,
							 vm::ptr<CellSaveDataStatCallback> funcStat, vm::ptr<CellSaveDataFileCallback> funcFile, u32 container, u32 userdata_addr)
{
	cellSysutil->Todo("cellSaveDataUserListLoad(version=%d, userId=%d, setList=0x%x, setBuf=0x%x, funcList=0x%x, funcStat=0x%x, funcFile=0x%x, container=0x%x, userdata_addr=0x%x)", version,
					  userId, setList.addr(), setBuf.addr(), funcList.addr(), funcStat.addr(), funcFile.addr(), container, userdata_addr);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserFixedSave(u32 version, u32 userId, vm::ptr<CellSaveDataSetList> setList, vm::ptr<CellSaveDataSetBuf> setBuf, vm::ptr<CellSaveDataFixedCallback> funcFixed,
							  vm::ptr<CellSaveDataStatCallback> funcStat, vm::ptr<CellSaveDataFileCallback> funcFile, u32 container, u32 userdata_addr)
{
	cellSysutil->Todo("cellSaveDataUserFixedSave(version=%d, userId=%d, setList=0x%x, setBuf=0x%x, funcFixed=0x%x, funcStat=0x%x, funcFile=0x%x, container=0x%x, userdata_addr=0x%x)", version,
					  userId, setList.addr(), setBuf.addr(), funcFixed.addr(), funcStat.addr(), funcFile.addr(), container, userdata_addr);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserFixedLoad(u32 version, u32 userId, vm::ptr<CellSaveDataSetList> setList, vm::ptr<CellSaveDataSetBuf> setBuf, vm::ptr<CellSaveDataFixedCallback> funcFixed,
							  vm::ptr<CellSaveDataStatCallback> funcStat, vm::ptr<CellSaveDataFileCallback> funcFile, u32 container, u32 userdata_addr)
{
	cellSysutil->Todo("cellSaveDataUserFixedLoad(version=%d, userId=%d, setList=0x%x, setBuf=0x%x, funcFixed=0x%x, funcStat=0x%x, funcFile=0x%x, container=0x%x, userdata_addr=0x%x)", version,
					  userId, setList.addr(), setBuf.addr(), funcFixed.addr(), funcStat.addr(), funcFile.addr(), container, userdata_addr);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserAutoSave(u32 version, u32 userId, u32 dirName_addr, u32 errDialog, vm::ptr<CellSaveDataSetBuf> setBuf, vm::ptr<CellSaveDataStatCallback> funcStat,
							 vm::ptr<CellSaveDataFileCallback> funcFile, u32 container, u32 userdata_addr)
{
	cellSysutil->Todo("cellSaveDataUserAutoSave(version=%d, userId=%d, dirName_addr=0x%x, errDialog=%d, setBuf=0x%x, funcStat=0x%x, funcFile=0x%x, container=0x%x, userdata_addr=0x%x)", version,
					  userId, dirName_addr, errDialog, setBuf.addr(), funcStat.addr(), funcFile.addr(), container, userdata_addr);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserAutoLoad(u32 version, u32 userId, u32 dirName_addr, u32 errDialog, vm::ptr<CellSaveDataSetBuf> setBuf, vm::ptr<CellSaveDataStatCallback> funcStat,
							 vm::ptr<CellSaveDataFileCallback> funcFile, u32 container, u32 userdata_addr)
{
	cellSysutil->Todo("cellSaveDataUserAutoLoad(version=%d, userId=%d, dirName_addr=0x%x, errDialog=%d, setBuf=0x%x, funcStat=0x%x, funcFile=0x%x, container=0x%x, userdata_addr=0x%x)", version,
					  userId, dirName_addr, errDialog, setBuf.addr(), funcStat.addr(), funcFile.addr(), container, userdata_addr);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserListAutoSave(u32 version, u32 userId, u32 errDialog, vm::ptr<CellSaveDataSetList> setList, vm::ptr<CellSaveDataSetBuf> setBuf, vm::ptr<CellSaveDataFixedCallback> funcFixed,
								 vm::ptr<CellSaveDataStatCallback> funcStat, vm::ptr<CellSaveDataFileCallback> funcFile, u32 container, u32 userdata_addr)
{
	cellSysutil->Todo("cellSaveDataUserListAutoSave(version=%d, userId=%d, errDialog=%d, setList=0x%x, setBuf=0x%x, funcFixed=0x%x, funcStat=0x%x, funcFile=0x%x, container=0x%x, userdata_addr=0x%x)",
					  version, userId, errDialog, setList.addr(), setBuf.addr(), funcFixed.addr(), funcStat.addr(), funcFile.addr(), container, userdata_addr);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserListAutoLoad(u32 version, u32 userId, u32 errDialog, vm::ptr<CellSaveDataSetList> setList, vm::ptr<CellSaveDataSetBuf> setBuf, vm::ptr<CellSaveDataFixedCallback> funcFixed,
								 vm::ptr<CellSaveDataStatCallback> funcStat, vm::ptr<CellSaveDataFileCallback> funcFile, u32 container, u32 userdata_addr)
{
	cellSysutil->Todo("cellSaveDataUserListAutoLoad(version=%d, userId=%d, errDialog=%D, setList=0x%x, setBuf=0x%x, funcFixed=0x%x, funcStat=0x%x, funcFile=0x%x, container=0x%x, userdata_addr=0x%x)",
					  version, userId,  errDialog, setList.addr(), setBuf.addr(), funcFixed.addr(), funcStat.addr(), funcFile.addr(), container, userdata_addr);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserFixedDelete() //CellSysutilUserId userId, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_SAVEDATA_RET_OK;
}

//void cellSaveDataEnableOverlay(); //int enable


// Functions (Extensions) 
int cellSaveDataListDelete() //CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataListCallback funcList, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataListImport() //CellSaveDataSetList *setList, u32 maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataListExport() //CellSaveDataSetList *setList, u32 maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataFixedImport() //const char *dirName, u32 maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataFixedExport() //const char *dirName, u32 maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataGetListItem() //const char *dirName, CellSaveDataDirStat *dir, CellSaveDataSystemFileParam *sysFileParam, vm::ptr<u32> bind, vm::ptr<u32> sizeKB
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserListDelete() //CellSysutilUserId userId, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataListCallback funcList, CellSaveDataDoneCallback funcDone,sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserListImport() //CellSysutilUserId userId, CellSaveDataSetList *setList, u32 maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserListExport() //CellSysutilUserId userId, CellSaveDataSetList *setList, u32 maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserFixedImport() //CellSysutilUserId userId, const char *dirName, u32 maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserFixedExport() //CellSysutilUserId userId, const char *dirName, u32 maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserGetListItem() //CellSysutilUserId userId, const char *dirName, CellSaveDataDirStat *dir, CellSaveDataSystemFileParam *sysFileParam, vm::ptr<u32> bind, vm::ptr<u32> sizeKB
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_SAVEDATA_RET_OK;
}
