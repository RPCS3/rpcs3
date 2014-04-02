#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include <algorithm>

#include "cellSysutil_SaveData.h"
#include "Loader/PSF.h"
#include "stblib/stb_image.h"

extern Module cellSysutil;

// Auxiliary Classes
class sortSaveDataEntry
{
	u32 sortType;
	u32 sortOrder;
public:
	sortSaveDataEntry(u32 type, u32 order) : sortType(type), sortOrder(order) {}
	bool operator()(const SaveDataListEntry& entry1, const SaveDataListEntry& entry2) const
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

void addSaveDataEntry(std::vector<SaveDataListEntry>& saveEntries, const std::string& saveDir)
{
	// PSF parameters
	vfsFile f(saveDir + "/PARAM.SFO");
	PSFLoader psf(f);
	if(!psf.Load(false))
		return;

	// PNG icon
	std::string localPath;
	int width, height, actual_components;
	Emu.GetVFS().GetDevice(saveDir + "/ICON0.PNG", localPath);

	SaveDataListEntry saveEntry;
	saveEntry.dirName = psf.GetString("SAVEDATA_DIRECTORY");
	saveEntry.listParam = psf.GetString("SAVEDATA_LIST_PARAM");
	saveEntry.title = psf.GetString("TITLE");
	saveEntry.subtitle = psf.GetString("SUB_TITLE");
	saveEntry.details = psf.GetString("DETAIL");
	saveEntry.sizeKB = getSaveDataSize(saveDir)/1024;
	saveEntry.st_atime_ = 0; // TODO
	saveEntry.st_mtime_ = 0; // TODO
	saveEntry.st_ctime_ = 0; // TODO
	saveEntry.iconBuf = stbi_load(localPath.c_str(), &width, &height, &actual_components, 3);
	saveEntry.iconBufSize = width * height * 3;
	saveEntry.isNew = false;

	saveEntries.push_back(saveEntry);
}

void addNewSaveDataEntry(std::vector<SaveDataListEntry>& saveEntries, mem_ptr_t<CellSaveDataListNewData> newData)
{
	SaveDataListEntry saveEntry;
	saveEntry.dirName = (char*)Memory.VirtualToRealAddr(newData->dirName_addr);
	saveEntry.title = (char*)Memory.VirtualToRealAddr(newData->icon->title_addr);
	saveEntry.subtitle = (char*)Memory.VirtualToRealAddr(newData->icon->title_addr);
	saveEntry.iconBuf = Memory.VirtualToRealAddr(newData->icon->iconBuf_addr);
	saveEntry.iconBufSize = newData->icon->iconBufSize;
	saveEntry.isNew = true;
	// TODO: Add information stored in newData->iconPosition. (It's not very relevant)

	saveEntries.push_back(saveEntry);
}

u32 focusSaveDataEntry(const std::vector<SaveDataListEntry>& saveEntries, u32 focusPosition)
{
	// TODO: Get the correct index. Right now, this returns the first element of the list.
	return 0;
}

void setSaveDataEntries(std::vector<SaveDataListEntry>& saveEntries, mem_ptr_t<CellSaveDataDirList> fixedList, u32 fixedListNum)
{
	std::vector<SaveDataListEntry>::iterator entry = saveEntries.begin();
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

void getSaveDataStat(SaveDataListEntry entry, mem_ptr_t<CellSaveDataStatGet> statGet)
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

s32 readSaveDataFile(mem_ptr_t<CellSaveDataFileSet> fileSet, const std::string& saveDir)
{
	void* dest = NULL;
	std::string filepath = saveDir + '/' + (char*)Memory.VirtualToRealAddr(fileSet->fileName_addr);
	vfsFile file(filepath);

	switch (fileSet->fileType)
	{
	case CELL_SAVEDATA_FILETYPE_SECUREFILE:
	case CELL_SAVEDATA_FILETYPE_NORMALFILE:
	case CELL_SAVEDATA_FILETYPE_CONTENT_ICON0:
	case CELL_SAVEDATA_FILETYPE_CONTENT_ICON1:
	case CELL_SAVEDATA_FILETYPE_CONTENT_PIC1:
	case CELL_SAVEDATA_FILETYPE_CONTENT_SND0:
		dest = Memory.VirtualToRealAddr(fileSet->fileBuf_addr);
		return file.Read(dest, min(fileSet->fileSize, fileSet->fileBufSize)); // TODO: This may fail for big files because of the dest pointer.

	default:
		ConLog.Error("readSaveDataFile: Unknown type! Aborting...");
		return -1;
	}
}


// Functions
int cellSaveDataListSave2(u32 version, mem_ptr_t<CellSaveDataSetList> setList, mem_ptr_t<CellSaveDataSetBuf> setBuf,
						  mem_func_ptr_t<CellSaveDataListCallback> funcList, mem_func_ptr_t<CellSaveDataStatCallback> funcStat, mem_func_ptr_t<CellSaveDataFileCallback> funcFile,
						  u32 container, u32 userdata_addr)
{
	cellSysutil.Warning("cellSaveDataListSave2(version=%d, setList_addr=0x%x, setBuf=0x%x, funcList=0x%x, funcStat=0x%x, funcFile=0x%x, container=%d, userdata_addr=0x%x)",
		version, setList.GetAddr(), setBuf.GetAddr(), funcList.GetAddr(), funcStat.GetAddr(), funcFile.GetAddr(), container, userdata_addr);

	if (!setList.IsGood() || !setBuf.IsGood() || !funcList.IsGood() || !funcStat.IsGood() || !funcFile.IsGood())
		return CELL_SAVEDATA_ERROR_PARAM;

	MemoryAllocator<CellSaveDataCBResult> result;
	MemoryAllocator<CellSaveDataListGet> listGet;
	MemoryAllocator<CellSaveDataListSet> listSet;

	std::string saveBaseDir = "/dev_hdd0/home/00000001/savedata/"; // TODO: Get the path of the current user
	vfsDir dir(saveBaseDir);
	if(!dir.IsOpened())
		return CELL_SAVEDATA_ERROR_INTERNAL;

	std::string dirNamePrefix = Memory.ReadString(setList->dirNamePrefix_addr);
	std::vector<SaveDataListEntry> saveEntries;
	for(const DirEntryInfo* entry = dir.Read(); entry; entry = dir.Read())
	{
		if (entry->flags & DirEntry_TypeDir || entry->name.substr(0,dirNamePrefix.size()) == dirNamePrefix)
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

	setSaveDataEntries(saveEntries, (u32)listSet->fixedList.GetAddr(), listSet->fixedListNum);
	if (listSet->newData.IsGood())
		addNewSaveDataEntry(saveEntries, (u32)listSet->newData.GetAddr());
	if (saveEntries.size() == 0) {
		ConLog.Warning("cellSaveDataListSave2: No save entries found!"); // TODO: Find a better way to handle this error
		return CELL_SAVEDATA_RET_OK;
	}

	MemoryAllocator<CellSaveDataStatGet> statGet;
	MemoryAllocator<CellSaveDataStatSet> statSet;
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

	MemoryAllocator<CellSaveDataFileGet> fileGet;
	MemoryAllocator<CellSaveDataFileSet> fileSet;
	funcFile(result.GetAddr(), fileGet.GetAddr(), fileSet.GetAddr());

	// TODO: There are other returns in this function that doesn't free the memory. Fix it (without using goto's, please).
	for (auto& entry : saveEntries) {
		delete[] entry.iconBuf;
		entry.iconBuf = nullptr;
	}

	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataListLoad2(u32 version, mem_ptr_t<CellSaveDataSetList> setList, mem_ptr_t<CellSaveDataSetBuf> setBuf,
						  mem_func_ptr_t<CellSaveDataListCallback> funcList, mem_func_ptr_t<CellSaveDataStatCallback> funcStat, mem_func_ptr_t<CellSaveDataFileCallback> funcFile,
						  u32 container, u32 userdata_addr)
{
	cellSysutil.Warning("cellSaveDataListLoad2(version=%d, setList_addr=0x%x, setBuf=0x%x, funcList=0x%x, funcStat=0x%x, funcFile=0x%x, container=%d, userdata_addr=0x%x)",
		version, setList.GetAddr(), setBuf.GetAddr(), funcList.GetAddr(), funcStat.GetAddr(), funcFile.GetAddr(), container, userdata_addr);
	
	if (!setList.IsGood() || !setBuf.IsGood() || !funcList.IsGood() || !funcStat.IsGood() || !funcFile.IsGood())
		return CELL_SAVEDATA_ERROR_PARAM;

	MemoryAllocator<CellSaveDataCBResult> result;
	MemoryAllocator<CellSaveDataListGet> listGet;
	MemoryAllocator<CellSaveDataListSet> listSet;
	MemoryAllocator<CellSaveDataStatGet> statGet;
	MemoryAllocator<CellSaveDataStatSet> statSet;
	MemoryAllocator<CellSaveDataFileGet> fileGet;
	MemoryAllocator<CellSaveDataFileSet> fileSet;

	std::string saveBaseDir = "/dev_hdd0/home/00000001/savedata/"; // TODO: Get the path of the current user
	vfsDir dir(saveBaseDir);
	if(!dir.IsOpened())
		return CELL_SAVEDATA_ERROR_INTERNAL;

	std::string dirNamePrefix = Memory.ReadString(setList->dirNamePrefix_addr);
	std::vector<SaveDataListEntry> saveEntries;
	for(const DirEntryInfo* entry = dir.Read(); entry; entry = dir.Read())
	{
		if (entry->flags & DirEntry_TypeDir || entry->name.substr(0,dirNamePrefix.size()) == dirNamePrefix)
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

	setSaveDataEntries(saveEntries, (u32)listSet->fixedList.GetAddr(), listSet->fixedListNum);
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
	if (statSet->setParam.IsGood())
		addNewSaveDataEntry(saveEntries, (u32)listSet->newData.GetAddr());

	fileGet->excSize = 0;
	while(true)
	{
		funcFile(result.GetAddr(), fileGet.GetAddr(), fileSet.GetAddr());
		if (result->result < 0)	{
			ConLog.Error("cellSaveDataListLoad2: CellSaveDataStatCallback failed."); // TODO: Once we verify that the entire SysCall is working, delete this debug error message.
			return CELL_SAVEDATA_ERROR_CBRESULT;
		}
		if (result->result == CELL_SAVEDATA_CBRESULT_OK_LAST)
			break;
		switch (fileSet->fileOperation)
		{
		case CELL_SAVEDATA_FILEOP_READ:
			fileGet->excSize = readSaveDataFile(fileSet.GetAddr(), saveBaseDir + (char*)statGet->dir.dirName);
			break;
		case CELL_SAVEDATA_FILEOP_WRITE:
		case CELL_SAVEDATA_FILEOP_DELETE:
		case CELL_SAVEDATA_FILEOP_WRITE_NOTRUNC:
			ConLog.Warning("cellSaveDataListLoad2: TODO: fileSet->fileOperation not yet implemented");
			break;
		}
	}

	// TODO: There are other returns in this function that doesn't free the memory. Fix it (without using goto's, please).
	for (auto& entry : saveEntries) {
		delete[] entry.iconBuf;
		entry.iconBuf = nullptr;
	}

	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataFixedSave2(u32 version,  mem_ptr_t<CellSaveDataSetList> setList, mem_ptr_t<CellSaveDataSetBuf> setBuf,
						   mem_func_ptr_t<CellSaveDataFixedCallback> funcFixed, mem_func_ptr_t<CellSaveDataStatCallback> funcStat, mem_func_ptr_t<CellSaveDataFileCallback> funcFile,
						   u32 container, u32 userdata_addr)
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataFixedLoad2(u32 version,  mem_ptr_t<CellSaveDataSetList> setList, mem_ptr_t<CellSaveDataSetBuf> setBuf,
						   mem_func_ptr_t<CellSaveDataFixedCallback> funcFixed, mem_func_ptr_t<CellSaveDataStatCallback> funcStat, mem_func_ptr_t<CellSaveDataFileCallback> funcFile,
						   u32 container, u32 userdata_addr)
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_SAVEDATA_RET_OK;
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
