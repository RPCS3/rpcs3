#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
											
void cellSaveData_init ();
Module cellSaveData ("cellSaveData", cellSaveData_init);

//Error codes

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

//datatyps

struct CellSaveDataSetList
{ 
unsigned int sortType; 
unsigned int sortOrder; 
char *dirNamePrefix; 
void *reserved; 
};

struct CellSaveDataSetBuf
{ 
unsigned int dirListMax; 
unsigned int fileListMax; 
unsigned int reserved[6]; 
unsigned int bufSize; 
void *buf; 
};

struct CellSaveDataNewDataIcon 
{ 
char *title; 
unsigned int iconBufSize; 
void *iconBuf; 
void *reserved; 
};

struct CellSaveDataListNewData 
{ 
unsigned int iconPosition; 
char *dirName; 
CellSaveDataNewDataIcon *icon; 
void *reserved; 
};

struct CellSaveDataDirList
{ 
char dirName; //[CELL_SAVEDATA_DIRNAME_SIZE]; 
char listParam; //[CELL_SAVEDATA_SYSP_LPARAM_SIZE]; 
char reserved[8]; 
};

struct CellSaveDataListGet
{ 
unsigned int dirNum; 
unsigned int dirListNum; 
CellSaveDataDirList *dirList; 
char reserved[64]; 
};

struct CellSaveDataListSet
{ 
unsigned int focusPosition; 
char *focusDirName; 
unsigned int fixedListNum; 
CellSaveDataDirList *fixedList; 
CellSaveDataListNewData *newData; 
void *reserved; 
};

struct CellSaveDataFixedSet
{ 
char *dirName; 
CellSaveDataNewDataIcon *newIcon; 
unsigned int option; 
};

struct CellSaveDataSystemFileParam 
{ 
char title; //[CELL_SAVEDATA_SYSP_TITLE_SIZE]; 
char subTitle; //[CELL_SAVEDATA_SYSP_SUBTITLE_SIZE]; 
char detail; //[CELL_SAVEDATA_SYSP_DETAIL_SIZE]; 
unsigned int attribute; 
char reserved2[4]; 
char listParam; //[CELL_SAVEDATA_SYSP_LPARAM_SIZE]; 
char reserved[256]; 
};

struct CellSaveDataDirStat
{ 
s64 st_atime; 
s64 st_mtime; 
s64 st_ctime; 
char dirName; //[CELL_SAVEDATA_DIRNAME_SIZE]; 
};

struct CellSaveDataFileStat
{ 
unsigned int fileType; 
char reserved1[4]; 
u64 st_size; 
s64 st_atime; 
s64 st_mtime; 
s64 st_ctime; 
char fileName; //[CELL_SAVEDATA_FILENAME_SIZE]; 
char reserved2[3]; 
};

struct CellSaveDataStatGet
{ 
int hddFreeSizeKB; 
unsigned int isNewData; 
CellSaveDataDirStat dir; 
CellSaveDataSystemFileParam getParam; 
unsigned int bind; 
int sizeKB; 
int sysSizeKB; 
unsigned int fileNum; 
unsigned int fileListNum; 
CellSaveDataFileStat *fileList; 
char reserved[64]; 
};

struct CellSaveDataAutoIndicator
{ 
unsigned int dispPosition;
unsigned int dispMode;
char *dispMsg;
unsigned int picBufSize;
void *picBuf;
void *reserved;
};

struct CellSaveDataStatSet 
{ 
CellSaveDataSystemFileParam *setParam; 
unsigned int reCreateMode; 
CellSaveDataAutoIndicator *indicator; 
};

struct CellSaveDataFileGet
{ 
unsigned int excSize; 
char reserved[64]; 
}; 

struct CellSaveDataFileSet 
{ 
unsigned int fileOperation; 
void *reserved; 
unsigned int fileType; 
unsigned char secureFileId; //[CELL_SAVEDATA_SECUREFILEID_SIZE]; 
char *fileName; 
unsigned int fileOffset; 
unsigned int fileSize; 
unsigned int fileBufSize; 
void *fileBuf; 
};

struct CellSaveDataCBResult 
{ 
int result; 
unsigned int progressBarInc; 
int errNeedSizeKB; 
char *invalidMsg; 
void *userdata; 
};

struct CellSaveDataDoneGet
{ 
int excResult; 
char dirName; //[CELL_SAVEDATA_DIRNAME_SIZE]; 
int sizeKB; 
int hddFreeSizeKB; 
char reserved[64]; 
};

//functions

int cellSaveDataListSave2 () //unsigned int version, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataListCallback funcList, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataListLoad2 () //unsigned int version, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataListCallback funcList, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataFixedSave2 () //unsigned int version, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile,sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataFixedLoad2 () //unsigned int version, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataAutoSave2 () //unsigned int version, const char *dirName, unsigned int errDialog, CellSaveDataSetBuf *setBuf, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataAutoLoad2 () //unsigned int version, const char *dirName, unsigned int errDialog, CellSaveDataSetBuf *setBuf, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataListAutoSave () //unsigned int version, unsigned int errDialog, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile,sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataListAutoLoad () //unsigned int version, unsigned int errDialog, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataDelete2 () //sys_memory_container_t container
{	 
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_CANCEL;
}

int cellSaveDataFixedDelete () //CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserListSave () //unsigned int version, CellSysutilUserId userId, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataListCallback funcList, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserListLoad () //unsigned int version, CellSysutilUserId userId, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataListCallback funcList, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserFixedSave () //unsigned int version, CellSysutilUserId userId, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserFixedLoad () //unsigned int version, CellSysutilUserId userId, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserAutoSave () //unsigned int version, CellSysutilUserId userId, const char *dirName, unsigned int errDialog, CellSaveDataSetBuf *setBuf, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserAutoLoad () //unsigned int version, CellSysutilUserId userId, const char *dirName, unsigned int errDialog, CellSaveDataSetBuf *setBuf, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserListAutoSave () //unsigned int version, CellSysutilUserId userId, unsigned int errDialog, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserListAutoLoad () //unsigned int version, CellSysutilUserId userId, unsigned int errDialog, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserFixedDelete () //CellSysutilUserId userId, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

//void cellSaveDataEnableOverlay (); //int enable


//Functions (Extensions) 

int cellSaveDataListDelete () //CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataListCallback funcList, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataListImport () //CellSaveDataSetList *setList, unsigned int maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataListExport () //CellSaveDataSetList *setList, unsigned int maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataFixedImport () //const char *dirName, unsigned int maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataFixedExport () //const char *dirName, unsigned int maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataGetListItem () //const char *dirName, CellSaveDataDirStat *dir, CellSaveDataSystemFileParam *sysFileParam, unsigned int *bind, int *sizeKB
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserListDelete () //CellSysutilUserId userId, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataListCallback funcList, CellSaveDataDoneCallback funcDone,sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserListImport () //CellSysutilUserId userId, CellSaveDataSetList *setList, unsigned int maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserListExport () //CellSysutilUserId userId, CellSaveDataSetList *setList, unsigned int maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserFixedImport () //CellSysutilUserId userId, const char *dirName, unsigned int maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserFixedExport () //CellSysutilUserId userId, const char *dirName, unsigned int maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

int cellSaveDataUserGetListItem () //CellSysutilUserId userId, const char *dirName, CellSaveDataDirStat *dir, CellSaveDataSystemFileParam *sysFileParam, unsigned int *bind, int *sizeKB
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}


//Callback Functions

void (*CellSaveDataFixedCallback) (); //CellSaveDataCBResult *cbResult, CellSaveDataListGet *get, CellSaveDataFixedSet *set

void (*CellSaveDataListCallback) (); //CellSaveDataCBResult *cbResult, CellSaveDataListGet *get, CellSaveDataListSet *set

void (*CellSaveDataStatCallback) (); //CellSaveDataCBResult *cbResult, CellSaveDataStatGet *get, CellSaveDataStatSet *set

void (*CellSaveDataFileCallback) (); //CellSaveDataCBResult *cbResult, CellSaveDataFileGet *get, CellSaveDataFileSet *set

void (*CellSaveDataDoneCallback) (); //CellSaveDataCBResult *cbResult, CellSaveDataDoneGet *get


void cellSaveData_init ()
{	
	cellSaveData.AddFunc (0x04c06fc2, cellSaveDataGetListItem);
	cellSaveData.AddFunc (0x273d116a, cellSaveDataUserListExport);
	cellSaveData.AddFunc (0x27cb8bc2, cellSaveDataListDelete);
	cellSaveData.AddFunc (0x39d6ee43, cellSaveDataUserListImport);
	cellSaveData.AddFunc (0x46a2d878, cellSaveDataFixedExport);
	cellSaveData.AddFunc (0x491cc554, cellSaveDataListExport);
	cellSaveData.AddFunc (0x52541151, cellSaveDataFixedImport);
	cellSaveData.AddFunc (0x529231b0, cellSaveDataUserFixedImport);
	cellSaveData.AddFunc (0x6b4e0de6, cellSaveDataListImport);
	cellSaveData.AddFunc (0x7048a9ba, cellSaveDataUserListDelete);
	cellSaveData.AddFunc (0x95ae2cde, cellSaveDataUserFixedExport);
	cellSaveData.AddFunc (0xf6482036, cellSaveDataUserGetListItem);
	cellSaveData.AddFunc (0x2de0d663, cellSaveDataListSave2);
	cellSaveData.AddFunc (0x1dfbfdd6, cellSaveDataListLoad2);
	cellSaveData.AddFunc (0x2aae9ef5, cellSaveDataFixedSave2);
	cellSaveData.AddFunc (0x2a8eada2, cellSaveDataFixedLoad2);
	cellSaveData.AddFunc (0x8b7ed64b, cellSaveDataAutoSave2);
	cellSaveData.AddFunc (0xfbd5c856, cellSaveDataAutoLoad2);
	cellSaveData.AddFunc (0x4dd03a4e, cellSaveDataListAutoSave);
	cellSaveData.AddFunc (0x21425307, cellSaveDataListAutoLoad);
	cellSaveData.AddFunc (0xedadd797, cellSaveDataDelete2);
	cellSaveData.AddFunc (0x0f03cfb0, cellSaveDataUserListSave);
	cellSaveData.AddFunc (0x39dd8425, cellSaveDataUserListLoad);
	cellSaveData.AddFunc (0x40b34847, cellSaveDataUserFixedSave);
	cellSaveData.AddFunc (0x6e7264ed, cellSaveDataUserFixedLoad);
	cellSaveData.AddFunc (0x52aac4fa, cellSaveDataUserAutoSave);
	cellSaveData.AddFunc (0xcdc6aefd, cellSaveDataUserAutoLoad);
	cellSaveData.AddFunc (0x0e091c36, cellSaveDataUserListAutoSave);
	//cellSaveData.AddFunc (0xe7fa820b, cellSaveDataEnableOverlay);
}