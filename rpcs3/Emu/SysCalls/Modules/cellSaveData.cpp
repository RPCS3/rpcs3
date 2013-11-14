#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
											
<<<<<<< HEAD
void cellSaveData_init ();
Module cellSaveData ("cellSaveData", cellSaveData_init);

//Error codes

=======
void cellSaveData_init();
Module cellSaveData("cellSaveData", cellSaveData_init);

// Error codes
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
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

<<<<<<< HEAD
//datatyps

struct CellSaveDataSetList
{ 
unsigned int sortType; 
unsigned int sortOrder; 
char *dirNamePrefix; 
void *reserved; 
=======
// Datatypes
struct CellSaveDataSetList
{ 
	unsigned int sortType;
	unsigned int sortOrder;
	char *dirNamePrefix;
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
};

struct CellSaveDataSetBuf
{ 
<<<<<<< HEAD
unsigned int dirListMax; 
unsigned int fileListMax; 
unsigned int reserved[6]; 
unsigned int bufSize; 
void *buf; 
=======
	unsigned int dirListMax;
	unsigned int fileListMax;
	unsigned int reserved[6];
	unsigned int bufSize;
	void *buf;
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
};

struct CellSaveDataNewDataIcon 
{ 
<<<<<<< HEAD
char *title; 
unsigned int iconBufSize; 
void *iconBuf; 
void *reserved; 
=======
	char *title;
	unsigned int iconBufSize;
	void *iconBuf;
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
};

struct CellSaveDataListNewData 
{ 
<<<<<<< HEAD
unsigned int iconPosition; 
char *dirName; 
CellSaveDataNewDataIcon *icon; 
void *reserved; 
=======
	unsigned int iconPosition;
	char *dirName;
	CellSaveDataNewDataIcon *icon;
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
};

struct CellSaveDataDirList
{ 
<<<<<<< HEAD
char dirName; //[CELL_SAVEDATA_DIRNAME_SIZE]; 
char listParam; //[CELL_SAVEDATA_SYSP_LPARAM_SIZE]; 
char reserved[8]; 
=======
	char dirName; //[CELL_SAVEDATA_DIRNAME_SIZE]; 
	char listParam; //[CELL_SAVEDATA_SYSP_LPARAM_SIZE]; 
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
};

struct CellSaveDataListGet
{ 
<<<<<<< HEAD
unsigned int dirNum; 
unsigned int dirListNum; 
CellSaveDataDirList *dirList; 
char reserved[64]; 
=======
	unsigned int dirNum;
	unsigned int dirListNum;
	CellSaveDataDirList *dirList;
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
};

struct CellSaveDataListSet
{ 
<<<<<<< HEAD
unsigned int focusPosition; 
char *focusDirName; 
unsigned int fixedListNum; 
CellSaveDataDirList *fixedList; 
CellSaveDataListNewData *newData; 
void *reserved; 
=======
	unsigned int focusPosition;
	char *focusDirName;
	unsigned int fixedListNum;
	CellSaveDataDirList *fixedList;
	CellSaveDataListNewData *newData;
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
};

struct CellSaveDataFixedSet
{ 
<<<<<<< HEAD
char *dirName; 
CellSaveDataNewDataIcon *newIcon; 
unsigned int option; 
=======
	char *dirName;
	CellSaveDataNewDataIcon *newIcon;
	unsigned int option;
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
};

struct CellSaveDataSystemFileParam 
{ 
<<<<<<< HEAD
char title; //[CELL_SAVEDATA_SYSP_TITLE_SIZE]; 
char subTitle; //[CELL_SAVEDATA_SYSP_SUBTITLE_SIZE]; 
char detail; //[CELL_SAVEDATA_SYSP_DETAIL_SIZE]; 
unsigned int attribute; 
char reserved2[4]; 
char listParam; //[CELL_SAVEDATA_SYSP_LPARAM_SIZE]; 
char reserved[256]; 
=======
	char title;			//[CELL_SAVEDATA_SYSP_TITLE_SIZE]; 
	char subTitle;		//[CELL_SAVEDATA_SYSP_SUBTITLE_SIZE]; 
	char detail;		//[CELL_SAVEDATA_SYSP_DETAIL_SIZE]; 
	unsigned int attribute; 
	char reserved2[4]; 
	char listParam;		//[CELL_SAVEDATA_SYSP_LPARAM_SIZE]; 
	char reserved[256]; 
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
};

struct CellSaveDataDirStat
{ 
<<<<<<< HEAD
s64 st_atime; 
s64 st_mtime; 
s64 st_ctime; 
char dirName; //[CELL_SAVEDATA_DIRNAME_SIZE]; 
=======
	s64 st_atime;
	s64 st_mtime;
	s64 st_ctime;
	char dirName; //[CELL_SAVEDATA_DIRNAME_SIZE]; 
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
};

struct CellSaveDataFileStat
{ 
<<<<<<< HEAD
unsigned int fileType; 
char reserved1[4]; 
u64 st_size; 
s64 st_atime; 
s64 st_mtime; 
s64 st_ctime; 
char fileName; //[CELL_SAVEDATA_FILENAME_SIZE]; 
char reserved2[3]; 
=======
	unsigned int fileType;
	char reserved1[4];
	u64 st_size;
	s64 st_atime;
	s64 st_mtime;
	s64 st_ctime;
	char fileName; //[CELL_SAVEDATA_FILENAME_SIZE]; 
	char reserved2[3];
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
};

struct CellSaveDataStatGet
{ 
<<<<<<< HEAD
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
=======
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
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
};

struct CellSaveDataAutoIndicator
{ 
<<<<<<< HEAD
unsigned int dispPosition;
unsigned int dispMode;
char *dispMsg;
unsigned int picBufSize;
void *picBuf;
void *reserved;
=======
	unsigned int dispPosition;
	unsigned int dispMode;
	char *dispMsg;
	unsigned int picBufSize;
	void *picBuf;
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
};

struct CellSaveDataStatSet 
{ 
<<<<<<< HEAD
CellSaveDataSystemFileParam *setParam; 
unsigned int reCreateMode; 
CellSaveDataAutoIndicator *indicator; 
=======
	CellSaveDataSystemFileParam *setParam;
	unsigned int reCreateMode;
	CellSaveDataAutoIndicator *indicator;
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
};

struct CellSaveDataFileGet
{ 
<<<<<<< HEAD
unsigned int excSize; 
char reserved[64]; 
=======
	unsigned int excSize;
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
}; 

struct CellSaveDataFileSet 
{ 
<<<<<<< HEAD
unsigned int fileOperation; 
void *reserved; 
unsigned int fileType; 
unsigned char secureFileId; //[CELL_SAVEDATA_SECUREFILEID_SIZE]; 
char *fileName; 
unsigned int fileOffset; 
unsigned int fileSize; 
unsigned int fileBufSize; 
void *fileBuf; 
=======
	unsigned int fileOperation;
	void *reserved;
	unsigned int fileType;
	unsigned char secureFileId; //[CELL_SAVEDATA_SECUREFILEID_SIZE]; 
	char *fileName;
	unsigned int fileOffset;
	unsigned int fileSize;
	unsigned int fileBufSize;
	void *fileBuf;
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
};

struct CellSaveDataCBResult 
{ 
<<<<<<< HEAD
int result; 
unsigned int progressBarInc; 
int errNeedSizeKB; 
char *invalidMsg; 
void *userdata; 
=======
	int result;
	unsigned int progressBarInc;
	int errNeedSizeKB;
	char *invalidMsg;
	void *userdata;
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
};

struct CellSaveDataDoneGet
{ 
<<<<<<< HEAD
int excResult; 
char dirName; //[CELL_SAVEDATA_DIRNAME_SIZE]; 
int sizeKB; 
int hddFreeSizeKB; 
char reserved[64]; 
};

//functions

int cellSaveDataListSave2 () //unsigned int version, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataListCallback funcList, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
=======
	int excResult;
	char dirName; //[CELL_SAVEDATA_DIRNAME_SIZE]; 
	int sizeKB;
	int hddFreeSizeKB;
};

// Functions
int cellSaveDataListSave2() //unsigned int version, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataListCallback funcList, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

<<<<<<< HEAD
int cellSaveDataListLoad2 () //unsigned int version, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataListCallback funcList, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
=======
int cellSaveDataListLoad2() //unsigned int version, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataListCallback funcList, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

<<<<<<< HEAD
int cellSaveDataFixedSave2 () //unsigned int version, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile,sys_memory_container_t container, void *userdata
=======
int cellSaveDataFixedSave2() //unsigned int version, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile,sys_memory_container_t container, void *userdata
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

<<<<<<< HEAD
int cellSaveDataFixedLoad2 () //unsigned int version, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
=======
int cellSaveDataFixedLoad2() //unsigned int version, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

<<<<<<< HEAD
int cellSaveDataAutoSave2 () //unsigned int version, const char *dirName, unsigned int errDialog, CellSaveDataSetBuf *setBuf, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
=======
int cellSaveDataAutoSave2() //unsigned int version, const char *dirName, unsigned int errDialog, CellSaveDataSetBuf *setBuf, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

<<<<<<< HEAD
int cellSaveDataAutoLoad2 () //unsigned int version, const char *dirName, unsigned int errDialog, CellSaveDataSetBuf *setBuf, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
=======
int cellSaveDataAutoLoad2() //unsigned int version, const char *dirName, unsigned int errDialog, CellSaveDataSetBuf *setBuf, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

<<<<<<< HEAD
int cellSaveDataListAutoSave () //unsigned int version, unsigned int errDialog, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile,sys_memory_container_t container, void *userdata
=======
int cellSaveDataListAutoSave() //unsigned int version, unsigned int errDialog, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile,sys_memory_container_t container, void *userdata
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

<<<<<<< HEAD
int cellSaveDataListAutoLoad () //unsigned int version, unsigned int errDialog, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
=======
int cellSaveDataListAutoLoad() //unsigned int version, unsigned int errDialog, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

<<<<<<< HEAD
int cellSaveDataDelete2 () //sys_memory_container_t container
=======
int cellSaveDataDelete2() //sys_memory_container_t container
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{	 
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_CANCEL;
}

<<<<<<< HEAD
int cellSaveDataFixedDelete () //CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
=======
int cellSaveDataFixedDelete() //CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

<<<<<<< HEAD
int cellSaveDataUserListSave () //unsigned int version, CellSysutilUserId userId, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataListCallback funcList, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
=======
int cellSaveDataUserListSave() //unsigned int version, CellSysutilUserId userId, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataListCallback funcList, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

<<<<<<< HEAD
int cellSaveDataUserListLoad () //unsigned int version, CellSysutilUserId userId, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataListCallback funcList, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
=======
int cellSaveDataUserListLoad() //unsigned int version, CellSysutilUserId userId, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataListCallback funcList, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

<<<<<<< HEAD
int cellSaveDataUserFixedSave () //unsigned int version, CellSysutilUserId userId, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
=======
int cellSaveDataUserFixedSave() //unsigned int version, CellSysutilUserId userId, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

<<<<<<< HEAD
int cellSaveDataUserFixedLoad () //unsigned int version, CellSysutilUserId userId, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
=======
int cellSaveDataUserFixedLoad() //unsigned int version, CellSysutilUserId userId, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

<<<<<<< HEAD
int cellSaveDataUserAutoSave () //unsigned int version, CellSysutilUserId userId, const char *dirName, unsigned int errDialog, CellSaveDataSetBuf *setBuf, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
=======
int cellSaveDataUserAutoSave() //unsigned int version, CellSysutilUserId userId, const char *dirName, unsigned int errDialog, CellSaveDataSetBuf *setBuf, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

<<<<<<< HEAD
int cellSaveDataUserAutoLoad () //unsigned int version, CellSysutilUserId userId, const char *dirName, unsigned int errDialog, CellSaveDataSetBuf *setBuf, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
=======
int cellSaveDataUserAutoLoad() //unsigned int version, CellSysutilUserId userId, const char *dirName, unsigned int errDialog, CellSaveDataSetBuf *setBuf, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

<<<<<<< HEAD
int cellSaveDataUserListAutoSave () //unsigned int version, CellSysutilUserId userId, unsigned int errDialog, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
=======
int cellSaveDataUserListAutoSave() //unsigned int version, CellSysutilUserId userId, unsigned int errDialog, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

<<<<<<< HEAD
int cellSaveDataUserListAutoLoad () //unsigned int version, CellSysutilUserId userId, unsigned int errDialog, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
=======
int cellSaveDataUserListAutoLoad() //unsigned int version, CellSysutilUserId userId, unsigned int errDialog, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataStatCallback funcStat, CellSaveDataFileCallback funcFile, sys_memory_container_t container, void *userdata
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

<<<<<<< HEAD
int cellSaveDataUserFixedDelete () //CellSysutilUserId userId, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
=======
int cellSaveDataUserFixedDelete() //CellSysutilUserId userId, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataFixedCallback funcFixed, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

<<<<<<< HEAD
//void cellSaveDataEnableOverlay (); //int enable


//Functions (Extensions) 

int cellSaveDataListDelete () //CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataListCallback funcList, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
=======
//void cellSaveDataEnableOverlay(); //int enable


// Functions (Extensions) 
int cellSaveDataListDelete() //CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataListCallback funcList, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

<<<<<<< HEAD
int cellSaveDataListImport () //CellSaveDataSetList *setList, unsigned int maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
=======
int cellSaveDataListImport() //CellSaveDataSetList *setList, unsigned int maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

<<<<<<< HEAD
int cellSaveDataListExport () //CellSaveDataSetList *setList, unsigned int maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
=======
int cellSaveDataListExport() //CellSaveDataSetList *setList, unsigned int maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

<<<<<<< HEAD
int cellSaveDataFixedImport () //const char *dirName, unsigned int maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
=======
int cellSaveDataFixedImport() //const char *dirName, unsigned int maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

<<<<<<< HEAD
int cellSaveDataFixedExport () //const char *dirName, unsigned int maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
=======
int cellSaveDataFixedExport() //const char *dirName, unsigned int maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

<<<<<<< HEAD
int cellSaveDataGetListItem () //const char *dirName, CellSaveDataDirStat *dir, CellSaveDataSystemFileParam *sysFileParam, unsigned int *bind, int *sizeKB
=======
int cellSaveDataGetListItem() //const char *dirName, CellSaveDataDirStat *dir, CellSaveDataSystemFileParam *sysFileParam, unsigned int *bind, int *sizeKB
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

<<<<<<< HEAD
int cellSaveDataUserListDelete () //CellSysutilUserId userId, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataListCallback funcList, CellSaveDataDoneCallback funcDone,sys_memory_container_t container, void *userdata
=======
int cellSaveDataUserListDelete() //CellSysutilUserId userId, CellSaveDataSetList *setList, CellSaveDataSetBuf *setBuf, CellSaveDataListCallback funcList, CellSaveDataDoneCallback funcDone,sys_memory_container_t container, void *userdata
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

<<<<<<< HEAD
int cellSaveDataUserListImport () //CellSysutilUserId userId, CellSaveDataSetList *setList, unsigned int maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
=======
int cellSaveDataUserListImport() //CellSysutilUserId userId, CellSaveDataSetList *setList, unsigned int maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

<<<<<<< HEAD
int cellSaveDataUserListExport () //CellSysutilUserId userId, CellSaveDataSetList *setList, unsigned int maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
=======
int cellSaveDataUserListExport() //CellSysutilUserId userId, CellSaveDataSetList *setList, unsigned int maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

<<<<<<< HEAD
int cellSaveDataUserFixedImport () //CellSysutilUserId userId, const char *dirName, unsigned int maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
=======
int cellSaveDataUserFixedImport() //CellSysutilUserId userId, const char *dirName, unsigned int maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

<<<<<<< HEAD
int cellSaveDataUserFixedExport () //CellSysutilUserId userId, const char *dirName, unsigned int maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
=======
int cellSaveDataUserFixedExport() //CellSysutilUserId userId, const char *dirName, unsigned int maxSizeKB, CellSaveDataDoneCallback funcDone, sys_memory_container_t container, void *userdata
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

<<<<<<< HEAD
int cellSaveDataUserGetListItem () //CellSysutilUserId userId, const char *dirName, CellSaveDataDirStat *dir, CellSaveDataSystemFileParam *sysFileParam, unsigned int *bind, int *sizeKB
=======
int cellSaveDataUserGetListItem() //CellSysutilUserId userId, const char *dirName, CellSaveDataDirStat *dir, CellSaveDataSystemFileParam *sysFileParam, unsigned int *bind, int *sizeKB
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellSaveData);
	return CELL_SAVEDATA_RET_OK;
}

<<<<<<< HEAD

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
=======
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
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
}