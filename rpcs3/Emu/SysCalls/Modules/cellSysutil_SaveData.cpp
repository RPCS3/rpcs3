#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include <algorithm>

#include "cellSysutil_SaveData.h"
#include "Loader/PSF.h"
#include "stblib/stb_image.h"

//#include "Emu/SysCalls/Dialogs/SaveDataList.h"

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
				return entry1.timestamp >= entry2.timestamp;
			else //if (sortType == CELL_SAVEDATA_SORTTYPE_SUBTITLE)
				return entry1.subtitle >= entry2.subtitle;
		}
		else //if (sortOrder == CELL_SAVEDATA_SORTORDER_ASCENT)
		{
			if (sortType == CELL_SAVEDATA_SORTTYPE_MODIFIEDTIME)
				return entry1.timestamp < entry2.timestamp;
			else //if (sortType == CELL_SAVEDATA_SORTTYPE_SUBTITLE)
				return entry1.subtitle < entry2.subtitle;
		}
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

void getSaveDataEntry(std::vector<SaveDataListEntry>& saveEntries, const std::string& saveDir)
{
	// PSF parameters
	vfsFile f(saveDir + "/PARAM.SFO");
	PSFLoader psf(f);
	if(!psf.Load(false))
		return;

	// PNG icon
	wxString localPath;
	int width, height, actual_components;
	Emu.GetVFS().GetDevice(saveDir + "/ICON0.PNG", localPath);

	SaveDataListEntry saveEntry;
	saveEntry.dirName = psf.GetString("SAVEDATA_DIRECTORY");
	saveEntry.listParam = psf.GetString("SAVEDATA_LIST_PARAM");
	saveEntry.title = psf.GetString("TITLE");
	saveEntry.subtitle = psf.GetString("SUB_TITLE");
	saveEntry.details = psf.GetString("DETAIL");
	saveEntry.sizeKb = getSaveDataSize(saveDir)/1024;
	saveEntry.timestamp = 0; // TODO
	saveEntry.iconBuffer = stbi_load(localPath.mb_str(), &width, &height, &actual_components, 3);

	saveEntries.push_back(saveEntry);
}


// Functions
int cellSaveDataListSave2(u32 version, mem_ptr_t<CellSaveDataSetList> setList, mem_ptr_t<CellSaveDataSetBuf> setBuf,
						  mem_func_ptr_t<CellSaveDataListCallback> funcList, mem_func_ptr_t<CellSaveDataStatCallback> funcStat, mem_func_ptr_t<CellSaveDataFileCallback> funcFile,
						  u32 container, u32 userdata)
{
	cellSysutil.Warning("cellSaveDataListSave2(version=%d, setList_addr=0x%x, setBuf=0x%x, funcList=0x%x, funcStat=0x%x, funcFile=0x%x, container=%d, userdata_addr=0x%x)",
		version, setList.GetAddr(), setBuf.GetAddr(), funcList.GetAddr(), funcStat.GetAddr(), funcFile.GetAddr(), container, userdata);

	if (!setList.IsGood() || !setBuf.IsGood() || !funcList.IsGood() || !funcStat.IsGood() || !funcFile.IsGood())
		return CELL_SAVEDATA_ERROR_PARAM;

	MemoryAllocator<CellSaveDataCBResult> result;
	MemoryAllocator<CellSaveDataListGet> listGet;
	MemoryAllocator<CellSaveDataListGet> listSet;

	std::string saveBaseDir = "/dev_hdd0/home/00000001/savedata/"; // TODO: Get the path of the current user
	vfsDir dir(saveBaseDir);
	if(!dir.IsOpened())
		return CELL_SAVEDATA_ERROR_INTERNAL;

	std::string dirNamePrefix = Memory.ReadString(setList->dirNamePrefix_addr).mb_str();
	std::vector<SaveDataListEntry> saveEntries;
	for(const DirEntryInfo* entry = dir.Read(); entry; entry = dir.Read())
	{
		if (entry->flags & DirEntry_TypeDir || entry->name.Left(dirNamePrefix.size()) == dirNamePrefix)
		{
			// Count the amount of matches and the amount of listed directories
			listGet->dirListNum++;
			if (listGet->dirListNum > setBuf->dirListMax)
				continue;
			listGet->dirNum++;

			std::string saveDir = saveBaseDir + (const char*)(entry->name.mb_str());
			getSaveDataEntry(saveEntries, saveDir);
		}
	}

	// Sort the entries and fill the listGet->dirList array
	std::sort(saveEntries.begin(), saveEntries.end(), sortSaveDataEntry(setList->sortType, setList->sortOrder));
	listGet->dirList_addr = setBuf->buf_addr;
	CellSaveDataDirList* dirList = (CellSaveDataDirList*)Memory.VirtualToRealAddr(listGet->dirList_addr);
	for (u32 i=0; i<saveEntries.size(); i++) {
		memcpy(dirList[i].dirName, saveEntries[i].dirName.c_str(), CELL_SAVEDATA_DIRNAME_SIZE);
		memcpy(dirList[i].listParam, saveEntries[i].listParam.c_str(), CELL_SAVEDATA_SYSP_LPARAM_SIZE);
	}

	funcList(result.GetAddr(), listGet.GetAddr(), listSet.GetAddr());

	MemoryAllocator<CellSaveDataStatGet> statGet;
	MemoryAllocator<CellSaveDataStatSet> statSet;
	funcStat(result.GetAddr(), statGet.GetAddr(), statSet.GetAddr());

	MemoryAllocator<CellSaveDataFileGet> fileGet;
	MemoryAllocator<CellSaveDataFileSet> fileSet;
	funcFile(result.GetAddr(), fileGet.GetAddr(), fileSet.GetAddr());

	for (auto& entry : saveEntries) {
		delete[] entry.iconBuffer;
		entry.iconBuffer = nullptr;
	}

	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataListLoad2(u32 version, mem_ptr_t<CellSaveDataSetList> setList, mem_ptr_t<CellSaveDataSetBuf> setBuf,
						  mem_func_ptr_t<CellSaveDataListCallback> funcList, mem_func_ptr_t<CellSaveDataStatCallback> funcStat, mem_func_ptr_t<CellSaveDataFileCallback> funcFile,
						  u32 container, u32 userdata)
{
	cellSysutil.Warning("cellSaveDataListLoad2(version=%d, setList_addr=0x%x, setBuf=0x%x, funcList=0x%x, funcStat=0x%x, funcFile=0x%x, container=%d, userdata_addr=0x%x)",
		version, setList.GetAddr(), setBuf.GetAddr(), funcList.GetAddr(), funcStat.GetAddr(), funcFile.GetAddr(), container, userdata);
	
	if (!setList.IsGood() || !setBuf.IsGood() || !funcList.IsGood() || !funcStat.IsGood() || !funcFile.IsGood())
		return CELL_SAVEDATA_ERROR_PARAM;

	MemoryAllocator<CellSaveDataCBResult> result;
	MemoryAllocator<CellSaveDataListGet> listGet;
	MemoryAllocator<CellSaveDataListGet> listSet;

	std::string saveBaseDir = "/dev_hdd0/home/00000001/savedata/"; // TODO: Get the path of the current user
	vfsDir dir(saveBaseDir);
	if(!dir.IsOpened())
		return CELL_SAVEDATA_ERROR_INTERNAL;

	std::string dirNamePrefix = Memory.ReadString(setList->dirNamePrefix_addr).mb_str();
	std::vector<SaveDataListEntry> saveEntries;
	for(const DirEntryInfo* entry = dir.Read(); entry; entry = dir.Read())
	{
		if (entry->flags & DirEntry_TypeDir || entry->name.Left(dirNamePrefix.size()) == dirNamePrefix)
		{
			// Count the amount of matches and the amount of listed directories
			listGet->dirListNum++;
			if (listGet->dirListNum > setBuf->dirListMax)
				continue;
			listGet->dirNum++;

			std::string saveDir = saveBaseDir + (const char*)(entry->name.mb_str());
			getSaveDataEntry(saveEntries, saveDir);
		}
	}

	// Sort the entries and fill the listGet->dirList array
	std::sort(saveEntries.begin(), saveEntries.end(), sortSaveDataEntry(setList->sortType, setList->sortOrder));
	listGet->dirList_addr = setBuf->buf_addr;
	CellSaveDataDirList* dirList = (CellSaveDataDirList*)Memory.VirtualToRealAddr(listGet->dirList_addr);
	for (u32 i=0; i<saveEntries.size(); i++) {
		memcpy(dirList[i].dirName, saveEntries[i].dirName.c_str(), CELL_SAVEDATA_DIRNAME_SIZE);
		memcpy(dirList[i].listParam, saveEntries[i].listParam.c_str(), CELL_SAVEDATA_SYSP_LPARAM_SIZE);
	}

	funcList(result.GetAddr(), listGet.GetAddr(), listSet.GetAddr());

	MemoryAllocator<CellSaveDataStatGet> statGet;
	MemoryAllocator<CellSaveDataStatSet> statSet;
	funcStat(result.GetAddr(), statGet.GetAddr(), statSet.GetAddr());

	MemoryAllocator<CellSaveDataFileGet> fileGet;
	MemoryAllocator<CellSaveDataFileSet> fileSet;
	funcFile(result.GetAddr(), fileGet.GetAddr(), fileSet.GetAddr());

	for (auto& entry : saveEntries) {
		delete[] entry.iconBuffer;
		entry.iconBuffer = nullptr;
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
