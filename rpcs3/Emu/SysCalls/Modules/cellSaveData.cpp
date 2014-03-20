#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
											
// TODO: Is this really a module? Or is everything part of cellSysutil?
void cellSaveData_init();
Module cellSaveData("cellSaveData", cellSaveData_init);

// Error codes
enum
{
	CELL_SAVEDATA_RET_OK = 0,
	CELL_SAVEDATA_RET_CANCEL = 1,
	CELL_SAVEDATA_ERROR_CBRESULT,
	CELL_SAVEDATA_ERROR_ACCESS_ERROR,
	CELL_SAVEDATA_ERROR_INTERNAL,
	CELL_SAVEDATA_ERROR_PARAM,
	CELL_SAVEDATA_ERROR_NOSPACE,
	CELL_SAVEDATA_ERROR_BROKEN,
	CELL_SAVEDATA_ERROR_FAILURE,
	CELL_SAVEDATA_ERROR_BUSY,
	CELL_SAVEDATA_ERROR_NOUSER,
};

// Constants
enum
{
	// CellSaveDataParamSize
	CELL_SAVEDATA_DIRNAME_SIZE          = 32,
	CELL_SAVEDATA_FILENAME_SIZE         = 13,
	CELL_SAVEDATA_SECUREFILEID_SIZE     = 16,
	CELL_SAVEDATA_PREFIX_SIZE           = 256,
	CELL_SAVEDATA_LISTITEM_MAX          = 2048,
	CELL_SAVEDATA_SECUREFILE_MAX        = 113,
	CELL_SAVEDATA_DIRLIST_MAX           = 2048,
	CELL_SAVEDATA_INVALIDMSG_MAX        = 256,
	CELL_SAVEDATA_INDICATORMSG_MAX      = 64,

	// CellSaveDataSystemParamSize
	CELL_SAVEDATA_SYSP_TITLE_SIZE       = 128,
	CELL_SAVEDATA_SYSP_SUBTITLE_SIZE    = 128,
	CELL_SAVEDATA_SYSP_DETAIL_SIZE      = 1024,
	CELL_SAVEDATA_SYSP_LPARAM_SIZE      = 8,
};

// Datatypes
struct CellSaveDataSetList
{ 
	be_t<u32> sortType;
	be_t<u32> sortOrder;
	be_t<u32> dirNamePrefix_addr; // char*
};

struct CellSaveDataSetBuf
{ 
	be_t<u32> dirListMax;
	be_t<u32> fileListMax;
	be_t<u32> reserved[6];
	be_t<u32> bufSize;
	be_t<u32> buf_addr; // void*
};

struct CellSaveDataNewDataIcon 
{ 
	be_t<u32> title_addr; // char*
	be_t<u32> iconBufSize;
	be_t<u32> iconBuf_addr; // void*
};

struct CellSaveDataListNewData 
{ 
	be_t<u32> iconPosition;
	be_t<u32> dirName_addr; // char*
	be_t<u32> icon_addr;    // CellSaveDataNewDataIcon*
};

struct CellSaveDataDirList
{ 
	s8 dirName[CELL_SAVEDATA_DIRNAME_SIZE]; 
	s8 listParam[CELL_SAVEDATA_SYSP_LPARAM_SIZE]; 
};

struct CellSaveDataListGet
{ 
	be_t<u32> dirNum;
	be_t<u32> dirListNum;
	be_t<u32> dirList_addr; // CellSaveDataDirList*
};

struct CellSaveDataListSet
{ 
	be_t<u32> focusPosition;
	be_t<u32> focusDirName_addr; // char*
	be_t<u32> fixedListNum;
	be_t<u32> fixedList_addr; // CellSaveDataDirList*
	be_t<u32> newData_addr;   // CellSaveDataListNewData*
};

struct CellSaveDataFixedSet
{ 
	be_t<u32> dirName_addr; // char*
	be_t<u32> newIcon_addr; // CellSaveDataNewDataIcon*
	be_t<u32> option;
};

struct CellSaveDataSystemFileParam 
{ 
	s8 title[CELL_SAVEDATA_SYSP_TITLE_SIZE]; 
	s8 subTitle[CELL_SAVEDATA_SYSP_SUBTITLE_SIZE]; 
	s8 detail[CELL_SAVEDATA_SYSP_DETAIL_SIZE]; 
	be_t<u32> attribute; 
	s8 reserved2[4]; 
	s8 listParam[CELL_SAVEDATA_SYSP_LPARAM_SIZE]; 
	s8 reserved[256]; 
};

struct CellSaveDataDirStat
{ 
	be_t<s64> st_atime_;
	be_t<s64> st_mtime_;
	be_t<s64> st_ctime_;
	s8 dirName[CELL_SAVEDATA_DIRNAME_SIZE]; 
};

struct CellSaveDataFileStat
{ 
	be_t<u32> fileType;
	u8 reserved1[4];
	be_t<u64> st_size;
	be_t<s64> st_atime_;
	be_t<s64> st_mtime_;
	be_t<s64> st_ctime_;
	u8 fileName[CELL_SAVEDATA_FILENAME_SIZE]; 
	u8 reserved2[3];
};

struct CellSaveDataStatGet
{ 
	be_t<s32> hddFreeSizeKB;
	be_t<u32> isNewData;
	CellSaveDataDirStat dir;
	CellSaveDataSystemFileParam getParam;
	be_t<u32> bind;
	be_t<s32> sizeKB;
	be_t<s32> sysSizeKB;
	be_t<u32> fileNum;
	be_t<u32> fileListNum;
	be_t<u32> fileList_addr; // CellSaveDataFileStat*
};

struct CellSaveDataAutoIndicator
{ 
	be_t<u32> dispPosition;
	be_t<u32> dispMode;
	be_t<u32> dispMsg_addr; // char*
	be_t<u32> picBufSize;
	be_t<u32> picBuf_addr; // void*
};

struct CellSaveDataStatSet 
{ 
	be_t<u32> setParam_addr;  // CellSaveDataSystemFileParam*
	be_t<u32> reCreateMode;
	be_t<u32> indicator_addr; // CellSaveDataAutoIndicator*
};

struct CellSaveDataFileGet
{ 
	be_t<u32> excSize;
}; 

struct CellSaveDataFileSet 
{ 
	be_t<u32> fileOperation;
	be_t<u32> reserved_addr; // void*
	be_t<u32> fileType;
	u8 secureFileId[CELL_SAVEDATA_SECUREFILEID_SIZE]; 
	be_t<u32> fileName_addr; // char*
	be_t<u32> fileOffset;
	be_t<u32> fileSize;
	be_t<u32> fileBufSize;
	be_t<u32> fileBuf_addr; // void*
};

struct CellSaveDataCBResult 
{ 
	be_t<s32> result;
	be_t<u32> progressBarInc;
	be_t<s32> errNeedSizeKB;
	be_t<u32> invalidMsg_addr; // char*
	be_t<u32> userdata_addr;   // void*
};

struct CellSaveDataDoneGet
{ 
	be_t<s32> excResult;
	s8 dirName[CELL_SAVEDATA_DIRNAME_SIZE]; 
	be_t<s32> sizeKB;
	be_t<s32> hddFreeSizeKB;
};

// Functions
int cellSaveDataListSave2() //u32 version, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataListCallback funcList, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataListLoad2() //u32 version, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataListCallback funcList, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataFixedSave2() //u32 version, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile,sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataFixedLoad2() //u32 version, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataAutoSave2() //u32 version, const char *dirName, u32 errDialog, CellSaveDataSetBuf *setBuf, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataAutoLoad2() //u32 version, const char *dirName, u32 errDialog, CellSaveDataSetBuf *setBuf, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataListAutoSave() //u32 version, u32 errDialog, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile,sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataListAutoLoad() //u32 version, u32 errDialog, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataDelete2() //sys_memory_container_t container
{	 
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_CANCEL;
}

int cellSaveDataFixedDelete() //CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserListSave() //u32 version, CellSysutilUserId userId, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataListCallback funcList, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserListLoad() //u32 version, CellSysutilUserId userId, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataListCallback funcList, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserFixedSave() //u32 version, CellSysutilUserId userId, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserFixedLoad() //u32 version, CellSysutilUserId userId, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserAutoSave() //u32 version, CellSysutilUserId userId, const char *dirName, u32 errDialog, CellSaveDataSetBuf *setBuf, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserAutoLoad() //u32 version, CellSysutilUserId userId, const char *dirName, u32 errDialog, CellSaveDataSetBuf *setBuf, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserListAutoSave() //u32 version, CellSysutilUserId userId, u32 errDialog, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserListAutoLoad() //u32 version, CellSysutilUserId userId, u32 errDialog, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserFixedDelete() //CellSysutilUserId userId, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

//void cellSaveDataEnableOverlay(); //int enable


// Functions (Extensions) 
int cellSaveDataListDelete() //CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataListCallback funcList, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataListImport() //CellSaveDataSetList *setList, u32 maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataListExport() //CellSaveDataSetList *setList, u32 maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataFixedImport() //const char *dirName, u32 maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataFixedExport() //const char *dirName, u32 maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataGetListItem() //const char *dirName, CellSaveDataDirStat *dir, CellSaveDataSystemFileParam *sysFileParam, mem32_t bind, mem32_t sizeKB
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserListDelete() //CellSysutilUserId userId, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataListCallback funcList, CellSaveDataDoneCallback funcDone,sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserListImport() //CellSysutilUserId userId, CellSaveDataSetList *setList, u32 maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserListExport() //CellSysutilUserId userId, CellSaveDataSetList *setList, u32 maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserFixedImport() //CellSysutilUserId userId, const char *dirName, u32 maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserFixedExport() //CellSysutilUserId userId, const char *dirName, u32 maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserGetListItem() //CellSysutilUserId userId, const char *dirName, CellSaveDataDirStat *dir, CellSaveDataSystemFileParam *sysFileParam, mem32_t bind, mem32_t sizeKB
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

// Callback Functions
void (*CellSaveDataFixedCallback)(); //CellSaveDataCBResult *cbResult, CellSaveDataListGet *get, CellSaveDataFixedSet *set

void (*CellSaveDataListCallback)(); //CellSaveDataCBResult *cbResult, CellSaveDataListGet *get, CellSaveDataListSet *set

void (*CellSaveDataStatCallback)(); //CellSaveDataCBResult *cbResult, CellSaveDataStatGet *get, CellSaveDataStatSet *set

void (*CellSaveDataFileCallback)(); //CellSaveDataCBResult *cbResult, CellSaveDataFileGet *get, CellSaveDataFileSet *set

void (*CellSaveDataDoneCallback)(); //CellSaveDataCBResult *cbResult, CellSaveDataDoneGet *get

void cellSaveData_init()
{	
	cellSaveData.AddFunc(0x04c06fc2, cellSaveDataGetListItem);
	cellSaveData.AddFunc(0x273d116a, cellSaveDataUserListExport);
	cellSaveData.AddFunc(0x27cb8bc2, cellSaveDataListDelete);
	cellSaveData.AddFunc(0x39d6ee43, cellSaveDataUserListImport);
	cellSaveData.AddFunc(0x46a2d878, cellSaveDataFixedExport);
	cellSaveData.AddFunc(0x491cc554, cellSaveDataListExport);
	cellSaveData.AddFunc(0x52541151, cellSaveDataFixedImport);
	cellSaveData.AddFunc(0x529231b0, cellSaveDataUserFixedImport);
	cellSaveData.AddFunc(0x6b4e0de6, cellSaveDataListImport);
	cellSaveData.AddFunc(0x7048a9ba, cellSaveDataUserListDelete);
	cellSaveData.AddFunc(0x95ae2cde, cellSaveDataUserFixedExport);
	cellSaveData.AddFunc(0xf6482036, cellSaveDataUserGetListItem);
	cellSaveData.AddFunc(0x2de0d663, cellSaveDataListSave2);
	cellSaveData.AddFunc(0x1dfbfdd6, cellSaveDataListLoad2);
	cellSaveData.AddFunc(0x2aae9ef5, cellSaveDataFixedSave2);
	cellSaveData.AddFunc(0x2a8eada2, cellSaveDataFixedLoad2);
	cellSaveData.AddFunc(0x8b7ed64b, cellSaveDataAutoSave2);
	cellSaveData.AddFunc(0xfbd5c856, cellSaveDataAutoLoad2);
	cellSaveData.AddFunc(0x4dd03a4e, cellSaveDataListAutoSave);
	cellSaveData.AddFunc(0x21425307, cellSaveDataListAutoLoad);
	cellSaveData.AddFunc(0xedadd797, cellSaveDataDelete2);
	cellSaveData.AddFunc(0x0f03cfb0, cellSaveDataUserListSave);
	cellSaveData.AddFunc(0x39dd8425, cellSaveDataUserListLoad);
	cellSaveData.AddFunc(0x40b34847, cellSaveDataUserFixedSave);
	cellSaveData.AddFunc(0x6e7264ed, cellSaveDataUserFixedLoad);
	cellSaveData.AddFunc(0x52aac4fa, cellSaveDataUserAutoSave);
	cellSaveData.AddFunc(0xcdc6aefd, cellSaveDataUserAutoLoad);
	cellSaveData.AddFunc(0x0e091c36, cellSaveDataUserListAutoSave);
	//cellSaveData.AddFunc(0xe7fa820b, cellSaveDataEnableOverlay);
}
