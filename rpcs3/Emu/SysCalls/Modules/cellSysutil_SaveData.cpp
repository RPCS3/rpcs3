#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include <algorithm>

#include "cellSysutil_SaveData.h"
#include "Loader/PSF.h"

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
	saveEntry.sizeKB = getSaveDataSize(saveDir)/1024;
	saveEntry.st_atime_ = 0; // TODO
	saveEntry.st_mtime_ = 0; // TODO
	saveEntry.st_ctime_ = 0; // TODO
	saveEntry.iconBuf = NULL; // TODO: Here should be the PNG buffer
	saveEntry.iconBufSize = 0; // TODO: Size of the PNG file
	saveEntry.isNew = false;

	saveEntries.push_back(saveEntry);
}

void addNewSaveDataEntry(std::vector<SaveDataEntry>& saveEntries, mem_ptr_t<CellSaveDataListNewData> newData)
{
	SaveDataEntry saveEntry;
	saveEntry.dirName = (char*)Memory.VirtualToRealAddr(newData->dirName_addr);
	saveEntry.title = (char*)Memory.VirtualToRealAddr(newData->icon->title_addr);
	saveEntry.subtitle = (char*)Memory.VirtualToRealAddr(newData->icon->title_addr);
	saveEntry.iconBuf = Memory.VirtualToRealAddr(newData->icon->iconBuf_addr);
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

void setSaveDataList(std::vector<SaveDataEntry>& saveEntries, mem_ptr_t<CellSaveDataDirList> fixedList, u32 fixedListNum)
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

void setSaveDataFixed(std::vector<SaveDataEntry>& saveEntries, mem_ptr_t<CellSaveDataFixedSet> fixedSet)
{
	std::vector<SaveDataEntry>::iterator entry = saveEntries.begin();
	while (entry != saveEntries.end())
	{
		if (entry->dirName == (char*)Memory.VirtualToRealAddr(fixedSet->dirName_addr))
			entry = saveEntries.erase(entry);
		else
			entry++;
	}

	if (saveEntries.size() == 0)
	{
		SaveDataEntry entry;
		entry.dirName = (char*)Memory.VirtualToRealAddr(fixedSet->dirName_addr);
		entry.isNew = true;
		saveEntries.push_back(entry);
	}

	if (fixedSet->newIcon.IsGood())
	{
		saveEntries[0].iconBuf = Memory.VirtualToRealAddr(fixedSet->newIcon->iconBuf_addr);
		saveEntries[0].iconBufSize = fixedSet->newIcon->iconBufSize;
		saveEntries[0].title = (char*)Memory.VirtualToRealAddr(fixedSet->newIcon->title_addr);
		saveEntries[0].subtitle = (char*)Memory.VirtualToRealAddr(fixedSet->newIcon->title_addr);
	}
}

void getSaveDataStat(SaveDataEntry entry, mem_ptr_t<CellSaveDataStatGet> statGet)
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
	memcpy(statGet->dir.dirName, entry.dirName.c_str(), CELL_SAVEDATA_DIRNAME_SIZE);

	statGet->getParam.attribute = 0; // TODO ?
	memcpy(statGet->getParam.title, entry.title.c_str(), CELL_SAVEDATA_SYSP_TITLE_SIZE);
	memcpy(statGet->getParam.subTitle, entry.subtitle.c_str(), CELL_SAVEDATA_SYSP_SUBTITLE_SIZE);
	memcpy(statGet->getParam.detail, entry.details.c_str(), CELL_SAVEDATA_SYSP_DETAIL_SIZE);
	memcpy(statGet->getParam.listParam, entry.listParam.c_str(), CELL_SAVEDATA_SYSP_LPARAM_SIZE);

	statGet->fileNum = 0;
	statGet->fileList.SetAddr(0);
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

			if (dirEntry->name == "ICON0.PNG") fileEntry.fileType = CELL_SAVEDATA_FILETYPE_CONTENT_ICON0;
			if (dirEntry->name == "ICON1.PAM") fileEntry.fileType = CELL_SAVEDATA_FILETYPE_CONTENT_ICON1;
			if (dirEntry->name == "PIC1.PNG")  fileEntry.fileType = CELL_SAVEDATA_FILETYPE_CONTENT_PIC1;
			if (dirEntry->name == "SND0.AT3")  fileEntry.fileType = CELL_SAVEDATA_FILETYPE_CONTENT_SND0;
			fileEntry.st_size = file.GetSize();
			fileEntry.st_atime_ = 0; // TODO ?
			fileEntry.st_mtime_ = 0; // TODO ?
			fileEntry.st_ctime_ = 0; // TODO ?
			memcpy(fileEntry.fileName, dirEntry->name.c_str(), CELL_SAVEDATA_FILENAME_SIZE);

			fileEntries.push_back(fileEntry);
		}
	}

	statGet->fileList.SetAddr(Memory.Alloc(sizeof(CellSaveDataFileStat) * fileEntries.size(), sizeof(CellSaveDataFileStat)));
	for (u32 i=0; i<fileEntries.size(); i++)
		memcpy(&statGet->fileList[i], &fileEntries[i], sizeof(CellSaveDataFileStat));
}

s32 modifySaveDataFiles(mem_func_ptr_t<CellSaveDataFileCallback>& funcFile, mem_ptr_t<CellSaveDataCBResult> result, const std::string& saveDataDir)
{
	MemoryAllocator<CellSaveDataFileGet> fileGet;
	MemoryAllocator<CellSaveDataFileSet> fileSet;

	if (!Emu.GetVFS().ExistsDir(saveDataDir))
		Emu.GetVFS().CreateDir(saveDataDir);

	fileGet->excSize = 0;
	while (true)
	{
		funcFile(result.GetAddr(), fileGet.GetAddr(), fileSet.GetAddr());
		if (result->result < 0)	{
			ConLog.Error("modifySaveDataFiles: CellSaveDataFileCallback failed."); // TODO: Once we verify that the entire SysCall is working, delete this debug error message.
			return CELL_SAVEDATA_ERROR_CBRESULT;
		}
		if (result->result == CELL_SAVEDATA_CBRESULT_OK_LAST) {
			break;
		}

		std::string filepath = saveDataDir + '/';
		vfsStream* file = NULL;
		void* buf = Memory.VirtualToRealAddr(fileSet->fileBuf_addr);

		switch ((u32)fileSet->fileType)
		{
		case CELL_SAVEDATA_FILETYPE_SECUREFILE:     filepath += (char*)Memory.VirtualToRealAddr(fileSet->fileName_addr); break;
		case CELL_SAVEDATA_FILETYPE_NORMALFILE:     filepath += (char*)Memory.VirtualToRealAddr(fileSet->fileName_addr); break;
		case CELL_SAVEDATA_FILETYPE_CONTENT_ICON0:  filepath += "ICON0.PNG"; break;
		case CELL_SAVEDATA_FILETYPE_CONTENT_ICON1:  filepath += "ICON1.PAM"; break;
		case CELL_SAVEDATA_FILETYPE_CONTENT_PIC1:   filepath += "PIC1.PNG";  break;
		case CELL_SAVEDATA_FILETYPE_CONTENT_SND0:   filepath += "SND0.AT3";  break;

		default:
			ConLog.Error("modifySaveDataFiles: Unknown fileType! Aborting...");
			return CELL_SAVEDATA_ERROR_PARAM;
		}

		switch ((u32)fileSet->fileOperation)
		{
		case CELL_SAVEDATA_FILEOP_READ:
			file = Emu.GetVFS().OpenFile(filepath, vfsRead);
			fileGet->excSize = file->Read(buf, min(fileSet->fileSize, fileSet->fileBufSize)); // TODO: This may fail for big files because of the dest pointer.
			break;
		
		case CELL_SAVEDATA_FILEOP_WRITE:
			Emu.GetVFS().CreateFile(filepath);
			file = Emu.GetVFS().OpenFile(filepath, vfsWrite);
			fileGet->excSize = file->Write(buf, min(fileSet->fileSize, fileSet->fileBufSize)); // TODO: This may fail for big files because of the dest pointer.
			break;

		case CELL_SAVEDATA_FILEOP_DELETE:
			Emu.GetVFS().RemoveFile(filepath);
			fileGet->excSize = 0;
			break;

		case CELL_SAVEDATA_FILEOP_WRITE_NOTRUNC:
			ConLog.Warning("modifySaveDataFiles: File operation CELL_SAVEDATA_FILEOP_WRITE_NOTRUNC not yet implemented");
			break;

		default:
			ConLog.Error("modifySaveDataFiles: Unknown fileOperation! Aborting...");
			return CELL_SAVEDATA_ERROR_PARAM;
		}

		if (file && file->IsOpened())
			file->Close();
	}
	return CELL_SAVEDATA_RET_OK;
}


// Functions
int cellSaveDataListSave2(u32 version, mem_ptr_t<CellSaveDataSetList> setList, mem_ptr_t<CellSaveDataSetBuf> setBuf,
						  mem_func_ptr_t<CellSaveDataListCallback> funcList, mem_func_ptr_t<CellSaveDataStatCallback> funcStat, mem_func_ptr_t<CellSaveDataFileCallback> funcFile,
						  u32 container, u32 userdata_addr)
{
	cellSysutil->Warning("cellSaveDataListSave2(version=%d, setList_addr=0x%x, setBuf_addr=0x%x, funcList_addr=0x%x, funcStat_addr=0x%x, funcFile_addr=0x%x, container=%d, userdata_addr=0x%x)",
		version, setList.GetAddr(), setBuf.GetAddr(), funcList.GetAddr(), funcStat.GetAddr(), funcFile.GetAddr(), container, userdata_addr);

	if (!setList.IsGood() || !setBuf.IsGood() || !funcList.IsGood() || !funcStat.IsGood() || !funcFile.IsGood())
		return CELL_SAVEDATA_ERROR_PARAM;

	MemoryAllocator<CellSaveDataCBResult> result;
	MemoryAllocator<CellSaveDataListGet> listGet;
	MemoryAllocator<CellSaveDataListSet> listSet;
	MemoryAllocator<CellSaveDataStatGet> statGet;
	MemoryAllocator<CellSaveDataStatSet> statSet;

	std::string saveBaseDir = "/dev_hdd0/home/00000001/savedata/"; // TODO: Get the path of the current user
	vfsDir dir(saveBaseDir);
	if(!dir.IsOpened())
		return CELL_SAVEDATA_ERROR_INTERNAL;

	std::string dirNamePrefix = Memory.ReadString(setList->dirNamePrefix_addr);
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
	listGet->dirList.SetAddr(setBuf->buf_addr);
	CellSaveDataDirList* dirList = (CellSaveDataDirList*)Memory.VirtualToRealAddr(listGet->dirList.GetAddr());
	for (u32 i=0; i<saveEntries.size(); i++) {
		memcpy(dirList[i].dirName, saveEntries[i].dirName.c_str(), CELL_SAVEDATA_DIRNAME_SIZE);
		memcpy(dirList[i].listParam, saveEntries[i].listParam.c_str(), CELL_SAVEDATA_SYSP_LPARAM_SIZE);
	}

	funcList(result.GetAddr(), listGet.GetAddr(), listSet.GetAddr());
	if (result->result < 0)	{
		ConLog.Error("cellSaveDataListSave2: CellSaveDataListCallback failed."); // TODO: Once we verify that the entire SysCall is working, delete this debug error message.
		return CELL_SAVEDATA_ERROR_CBRESULT;
	}
	if (!listSet->fixedList.IsGood())
		return CELL_SAVEDATA_ERROR_PARAM;

	setSaveDataList(saveEntries, (u32)listSet->fixedList.GetAddr(), listSet->fixedListNum);
	if (listSet->newData.IsGood())
		addNewSaveDataEntry(saveEntries, (u32)listSet->newData.GetAddr());
	if (saveEntries.size() == 0) {
		ConLog.Warning("cellSaveDataListSave2: No save entries found!"); // TODO: Find a better way to handle this error
		return CELL_SAVEDATA_RET_OK;
	}

	u32 focusIndex = focusSaveDataEntry(saveEntries, listSet->focusPosition);
	// TODO: Display the dialog here
	u32 selectedIndex = focusIndex; // TODO: Until the dialog is implemented, select always the focused entry
	getSaveDataStat(saveEntries[selectedIndex], statGet.GetAddr());
	result->userdata_addr = userdata_addr;

	funcStat(result.GetAddr(), statGet.GetAddr(), statSet.GetAddr());
	Memory.Free(statGet->fileList.GetAddr());
	if (result->result < 0)	{
		ConLog.Error("cellSaveDataListLoad2: CellSaveDataStatCallback failed."); // TODO: Once we verify that the entire SysCall is working, delete this debug error message.
		return CELL_SAVEDATA_ERROR_CBRESULT;
	}
	/*if (statSet->setParam.IsGood())
		addNewSaveDataEntry(saveEntries, (u32)listSet->newData.GetAddr()); // TODO: This *is* wrong
	*/

	// Enter the loop where the save files are read/created/deleted.
	s32 ret = modifySaveDataFiles(funcFile, result.GetAddr(), saveBaseDir + (char*)statGet->dir.dirName);

	return ret;
}

int cellSaveDataListLoad2(u32 version, mem_ptr_t<CellSaveDataSetList> setList, mem_ptr_t<CellSaveDataSetBuf> setBuf,
						  mem_func_ptr_t<CellSaveDataListCallback> funcList, mem_func_ptr_t<CellSaveDataStatCallback> funcStat, mem_func_ptr_t<CellSaveDataFileCallback> funcFile,
						  u32 container, u32 userdata_addr)
{
	cellSysutil->Warning("cellSaveDataListLoad2(version=%d, setList_addr=0x%x, setBuf_addr=0x%x, funcList_addr=0x%x, funcStat_addr=0x%x, funcFile_addr=0x%x, container=%d, userdata_addr=0x%x)",
		version, setList.GetAddr(), setBuf.GetAddr(), funcList.GetAddr(), funcStat.GetAddr(), funcFile.GetAddr(), container, userdata_addr);
	
	if (!setList.IsGood() || !setBuf.IsGood() || !funcList.IsGood() || !funcStat.IsGood() || !funcFile.IsGood())
		return CELL_SAVEDATA_ERROR_PARAM;

	MemoryAllocator<CellSaveDataCBResult> result;
	MemoryAllocator<CellSaveDataListGet> listGet;
	MemoryAllocator<CellSaveDataListSet> listSet;
	MemoryAllocator<CellSaveDataStatGet> statGet;
	MemoryAllocator<CellSaveDataStatSet> statSet;

	std::string saveBaseDir = "/dev_hdd0/home/00000001/savedata/"; // TODO: Get the path of the current user
	vfsDir dir(saveBaseDir);
	if(!dir.IsOpened())
		return CELL_SAVEDATA_ERROR_INTERNAL;

	std::string dirNamePrefix = Memory.ReadString(setList->dirNamePrefix_addr);
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
	listGet->dirList.SetAddr(setBuf->buf_addr);
	CellSaveDataDirList* dirList = (CellSaveDataDirList*)Memory.VirtualToRealAddr(listGet->dirList.GetAddr());
	for (u32 i=0; i<saveEntries.size(); i++) {
		memcpy(dirList[i].dirName, saveEntries[i].dirName.c_str(), CELL_SAVEDATA_DIRNAME_SIZE);
		memcpy(dirList[i].listParam, saveEntries[i].listParam.c_str(), CELL_SAVEDATA_SYSP_LPARAM_SIZE);
	}

	funcList(result.GetAddr(), listGet.GetAddr(), listSet.GetAddr());
	if (result->result < 0)	{
		ConLog.Error("cellSaveDataListLoad2: CellSaveDataListCallback failed."); // TODO: Once we verify that the entire SysCall is working, delete this debug error message.
		return CELL_SAVEDATA_ERROR_CBRESULT;
	}
	if (!listSet->fixedList.IsGood())
		return CELL_SAVEDATA_ERROR_PARAM;

	setSaveDataList(saveEntries, (u32)listSet->fixedList.GetAddr(), listSet->fixedListNum);
	if (listSet->newData.IsGood())
		addNewSaveDataEntry(saveEntries, (u32)listSet->newData.GetAddr());
	if (saveEntries.size() == 0) {
		ConLog.Warning("cellSaveDataListLoad2: No save entries found!"); // TODO: Find a better way to handle this error
		return CELL_SAVEDATA_RET_OK;
	}

	u32 focusIndex = focusSaveDataEntry(saveEntries, listSet->focusPosition);
	// TODO: Display the dialog here
	u32 selectedIndex = focusIndex; // TODO: Until the dialog is implemented, select always the focused entry
	getSaveDataStat(saveEntries[selectedIndex], statGet.GetAddr());
	result->userdata_addr = userdata_addr;

	funcStat(result.GetAddr(), statGet.GetAddr(), statSet.GetAddr());
	Memory.Free(statGet->fileList.GetAddr());
	if (result->result < 0)	{
		ConLog.Error("cellSaveDataListLoad2: CellSaveDataStatCallback failed."); // TODO: Once we verify that the entire SysCall is working, delete this debug error message.
		return CELL_SAVEDATA_ERROR_CBRESULT;
	}
	/*if (statSet->setParam.IsGood())
		// TODO: Write PARAM.SFO file
	*/

	// Enter the loop where the save files are read/created/deleted.
	s32 ret = modifySaveDataFiles(funcFile, result.GetAddr(), saveBaseDir + (char*)statGet->dir.dirName);

	return ret;
}

int cellSaveDataFixedSave2(u32 version,  mem_ptr_t<CellSaveDataSetList> setList, mem_ptr_t<CellSaveDataSetBuf> setBuf,
						   mem_func_ptr_t<CellSaveDataFixedCallback> funcFixed, mem_func_ptr_t<CellSaveDataStatCallback> funcStat, mem_func_ptr_t<CellSaveDataFileCallback> funcFile,
						   u32 container, u32 userdata_addr)
{
	cellSysutil->Warning("cellSaveDataFixedSave2(version=%d, setList_addr=0x%x, setBuf_addr=0x%x, funcFixed_addr=0x%x, funcStat_addr=0x%x, funcFile_addr=0x%x, container=%d, userdata_addr=0x%x)",
		version, setList.GetAddr(), setBuf.GetAddr(), funcFixed.GetAddr(), funcStat.GetAddr(), funcFile.GetAddr(), container, userdata_addr);

	if (!setList.IsGood() || !setBuf.IsGood() || !funcFixed.IsGood() || !funcStat.IsGood() || !funcFile.IsGood())
		return CELL_SAVEDATA_ERROR_PARAM;

	MemoryAllocator<CellSaveDataCBResult> result;
	MemoryAllocator<CellSaveDataListGet> listGet;
	MemoryAllocator<CellSaveDataFixedSet> fixedSet;
	MemoryAllocator<CellSaveDataStatGet> statGet;
	MemoryAllocator<CellSaveDataStatSet> statSet;

	std::string saveBaseDir = "/dev_hdd0/home/00000001/savedata/"; // TODO: Get the path of the current user
	vfsDir dir(saveBaseDir);
	if (!dir.IsOpened())
		return CELL_SAVEDATA_ERROR_INTERNAL;

	std::string dirNamePrefix = Memory.ReadString(setList->dirNamePrefix_addr);
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
	listGet->dirList.SetAddr(setBuf->buf_addr);
	CellSaveDataDirList* dirList = (CellSaveDataDirList*)Memory.VirtualToRealAddr(listGet->dirList.GetAddr());
	for (u32 i = 0; i<saveEntries.size(); i++) {
		memcpy(dirList[i].dirName, saveEntries[i].dirName.c_str(), CELL_SAVEDATA_DIRNAME_SIZE);
		memcpy(dirList[i].listParam, saveEntries[i].listParam.c_str(), CELL_SAVEDATA_SYSP_LPARAM_SIZE);
	}
	funcFixed(result.GetAddr(), listGet.GetAddr(), fixedSet.GetAddr());
	if (result->result < 0)	{
		ConLog.Error("cellSaveDataFixedSave2: CellSaveDataFixedCallback failed."); // TODO: Once we verify that the entire SysCall is working, delete this debug error message.
		return CELL_SAVEDATA_ERROR_CBRESULT;
	}
	setSaveDataFixed(saveEntries, fixedSet.GetAddr());
	getSaveDataStat(saveEntries[0], statGet.GetAddr()); // There should be only one element in this list
	// TODO: Display the Yes|No dialog here
	result->userdata_addr = userdata_addr;

	funcStat(result.GetAddr(), statGet.GetAddr(), statSet.GetAddr());
	Memory.Free(statGet->fileList.GetAddr());
	if (result->result < 0)	{
		ConLog.Error("cellSaveDataFixedSave2: CellSaveDataStatCallback failed."); // TODO: Once we verify that the entire SysCall is working, delete this debug error message.
		return CELL_SAVEDATA_ERROR_CBRESULT;
	}
	/*if (statSet->setParam.IsGood())
		// TODO: Write PARAM.SFO file
	*/

	// Enter the loop where the save files are read/created/deleted.
	s32 ret = modifySaveDataFiles(funcFile, result.GetAddr(), saveBaseDir + (char*)statGet->dir.dirName);

	return ret;
}

int cellSaveDataFixedLoad2(u32 version,  mem_ptr_t<CellSaveDataSetList> setList, mem_ptr_t<CellSaveDataSetBuf> setBuf,
						   mem_func_ptr_t<CellSaveDataFixedCallback> funcFixed, mem_func_ptr_t<CellSaveDataStatCallback> funcStat, mem_func_ptr_t<CellSaveDataFileCallback> funcFile,
						   u32 container, u32 userdata_addr)
{
	cellSysutil->Warning("cellSaveDataFixedLoad2(version=%d, setList_addr=0x%x, setBuf=0x%x, funcList=0x%x, funcStat=0x%x, funcFile=0x%x, container=%d, userdata_addr=0x%x)",
		version, setList.GetAddr(), setBuf.GetAddr(), funcFixed.GetAddr(), funcStat.GetAddr(), funcFile.GetAddr(), container, userdata_addr);

	if (!setList.IsGood() || !setBuf.IsGood() || !funcFixed.IsGood() || !funcStat.IsGood() || !funcFile.IsGood())
		return CELL_SAVEDATA_ERROR_PARAM;

	MemoryAllocator<CellSaveDataCBResult> result;
	MemoryAllocator<CellSaveDataListGet> listGet;
	MemoryAllocator<CellSaveDataFixedSet> fixedSet;
	MemoryAllocator<CellSaveDataStatGet> statGet;
	MemoryAllocator<CellSaveDataStatSet> statSet;

	std::string saveBaseDir = "/dev_hdd0/home/00000001/savedata/"; // TODO: Get the path of the current user
	vfsDir dir(saveBaseDir);
	if (!dir.IsOpened())
		return CELL_SAVEDATA_ERROR_INTERNAL;

	std::string dirNamePrefix = Memory.ReadString(setList->dirNamePrefix_addr);
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
	listGet->dirList.SetAddr(setBuf->buf_addr);
	CellSaveDataDirList* dirList = (CellSaveDataDirList*)Memory.VirtualToRealAddr(listGet->dirList.GetAddr());
	for (u32 i = 0; i<saveEntries.size(); i++) {
		memcpy(dirList[i].dirName, saveEntries[i].dirName.c_str(), CELL_SAVEDATA_DIRNAME_SIZE);
		memcpy(dirList[i].listParam, saveEntries[i].listParam.c_str(), CELL_SAVEDATA_SYSP_LPARAM_SIZE);
	}
	funcFixed(result.GetAddr(), listGet.GetAddr(), fixedSet.GetAddr());
	if (result->result < 0)	{
		ConLog.Error("cellSaveDataFixedLoad2: CellSaveDataFixedCallback failed."); // TODO: Once we verify that the entire SysCall is working, delete this debug error message.
		return CELL_SAVEDATA_ERROR_CBRESULT;
	}
	setSaveDataFixed(saveEntries, fixedSet.GetAddr());
	getSaveDataStat(saveEntries[0], statGet.GetAddr()); // There should be only one element in this list
	// TODO: Display the Yes|No dialog here
	result->userdata_addr = userdata_addr;

	funcStat(result.GetAddr(), statGet.GetAddr(), statSet.GetAddr());
	Memory.Free(statGet->fileList.GetAddr());
	if (result->result < 0)	{
		ConLog.Error("cellSaveDataFixedLoad2: CellSaveDataStatCallback failed."); // TODO: Once we verify that the entire SysCall is working, delete this debug error message.
		return CELL_SAVEDATA_ERROR_CBRESULT;
	}
	/*if (statSet->setParam.IsGood())
		// TODO: Write PARAM.SFO file
	*/

	// Enter the loop where the save files are read/created/deleted.
	s32 ret = modifySaveDataFiles(funcFile, result.GetAddr(), saveBaseDir + (char*)statGet->dir.dirName);

	return ret;
}

int cellSaveDataAutoSave2(u32 version, u32 dirName_addr, u32 errDialog, mem_ptr_t<CellSaveDataSetBuf> setBuf,
						  mem_func_ptr_t<CellSaveDataStatCallback> funcStat, mem_func_ptr_t<CellSaveDataFileCallback> funcFile,
						  u32 container, u32 userdata_addr)
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataAutoLoad2(u32 version, u32 dirName_addr, u32 errDialog, mem_ptr_t<CellSaveDataSetBuf> setBuf,
						  mem_func_ptr_t<CellSaveDataStatCallback> funcStat, mem_func_ptr_t<CellSaveDataFileCallback> funcFile,
						  u32 container, u32 userdata_addr)
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataListAutoSave() //u32 version, u32 errDialog, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile,sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataListAutoLoad() //u32 version, u32 errDialog, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataDelete2() //sys_memory_container_t container
{	 
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_SAVEDATA_RET_CANCEL;
}

int cellSaveDataFixedDelete() //CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserListSave() //u32 version, CellSysutilUserId userId, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataListCallback funcList, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserListLoad() //u32 version, CellSysutilUserId userId, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataListCallback funcList, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserFixedSave() //u32 version, CellSysutilUserId userId, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserFixedLoad() //u32 version, CellSysutilUserId userId, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserAutoSave() //u32 version, CellSysutilUserId userId, const char *dirName, u32 errDialog, CellSaveDataSetBuf *setBuf, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserAutoLoad() //u32 version, CellSysutilUserId userId, const char *dirName, u32 errDialog, CellSaveDataSetBuf *setBuf, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserListAutoSave() //u32 version, CellSysutilUserId userId, u32 errDialog, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserListAutoLoad() //u32 version, CellSysutilUserId userId, u32 errDialog, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSysutil);
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

int cellSaveDataGetListItem() //const char *dirName, CellSaveDataDirStat *dir, CellSaveDataSystemFileParam *sysFileParam, mem32_t bind, mem32_t sizeKB
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

int cellSaveDataUserGetListItem() //CellSysutilUserId userId, const char *dirName, CellSaveDataDirStat *dir, CellSaveDataSystemFileParam *sysFileParam, mem32_t bind, mem32_t sizeKB
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_SAVEDATA_RET_OK;
}
