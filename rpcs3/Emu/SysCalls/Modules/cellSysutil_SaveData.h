#pragma once

// Return codes
enum
{
	CELL_SAVEDATA_RET_OK                = 0,
	CELL_SAVEDATA_RET_CANCEL            = 1,
	CELL_SAVEDATA_ERROR_CBRESULT        = 0x8002b401,
	CELL_SAVEDATA_ERROR_ACCESS_ERROR    = 0x8002b402,
	CELL_SAVEDATA_ERROR_INTERNAL        = 0x8002b403,
	CELL_SAVEDATA_ERROR_PARAM           = 0x8002b404,
	CELL_SAVEDATA_ERROR_NOSPACE         = 0x8002b405,
	CELL_SAVEDATA_ERROR_BROKEN          = 0x8002b406,
	CELL_SAVEDATA_ERROR_FAILURE         = 0x8002b407,
	CELL_SAVEDATA_ERROR_BUSY            = 0x8002b408,
	CELL_SAVEDATA_ERROR_NOUSER          = 0x8002b409,
	CELL_SAVEDATA_ERROR_SIZEOVER        = 0x8002b40a,
	CELL_SAVEDATA_ERROR_NODATA          = 0x8002b40b,
	CELL_SAVEDATA_ERROR_NOTSUPPORTED    = 0x8002b40c,	
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

	// CellSaveDataSortType
	CELL_SAVEDATA_SORTTYPE_MODIFIEDTIME = 0,
	CELL_SAVEDATA_SORTTYPE_SUBTITLE     = 1,

	// CellSaveDataSortOrder
	CELL_SAVEDATA_SORTORDER_DESCENT     = 0,
	CELL_SAVEDATA_SORTORDER_ASCENT      = 1,
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


// Callback Functions
typedef void (*CellSaveDataFixedCallback)(mem_ptr_t<CellSaveDataCBResult> cbResult, mem_ptr_t<CellSaveDataListGet> get, mem_ptr_t<CellSaveDataFixedSet> set);
typedef void (*CellSaveDataListCallback) (mem_ptr_t<CellSaveDataCBResult> cbResult, mem_ptr_t<CellSaveDataListGet> get, mem_ptr_t<CellSaveDataListSet> set);
typedef void (*CellSaveDataStatCallback) (mem_ptr_t<CellSaveDataCBResult> cbResult, mem_ptr_t<CellSaveDataStatGet> get, mem_ptr_t<CellSaveDataStatSet> set);
typedef void (*CellSaveDataFileCallback) (mem_ptr_t<CellSaveDataCBResult> cbResult, mem_ptr_t<CellSaveDataFileGet> get, mem_ptr_t<CellSaveDataFileSet> set);
typedef void (*CellSaveDataDoneCallback) (mem_ptr_t<CellSaveDataCBResult> cbResult, mem_ptr_t<CellSaveDataDoneGet> get);


// Auxiliary Structs
struct SaveDataListEntry
{
	std::string dirName;
	std::string listParam;
	std::string title;
	std::string subtitle;
	std::string details;
	u32 sizeKb;
	u64 timestamp;
	void* iconBuffer;
};


// Function declarations
int cellSaveDataListSave2(u32 version, mem_ptr_t<CellSaveDataSetList> setList, mem_ptr_t<CellSaveDataSetBuf> setBuf,
						  mem_func_ptr_t<CellSaveDataListCallback> funcList, mem_func_ptr_t<CellSaveDataStatCallback> funcStat, mem_func_ptr_t<CellSaveDataFileCallback> funcFile,
						  u32 container, u32 userdata_addr);

int cellSaveDataListLoad2(u32 version, mem_ptr_t<CellSaveDataSetList> setList, mem_ptr_t<CellSaveDataSetBuf> setBuf,
						  mem_func_ptr_t<CellSaveDataListCallback> funcList, mem_func_ptr_t<CellSaveDataStatCallback> funcStat, mem_func_ptr_t<CellSaveDataFileCallback> funcFile,
						  u32 container, u32 userdata_addr);
