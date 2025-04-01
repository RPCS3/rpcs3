#pragma once

#include "util/types.hpp"
#include "Emu/Memory/vm_ptr.h"
#include <string>
#include <vector>

// Return codes
enum CellSaveDataError : u32
{
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

// Callback return codes
enum
{
	CELL_SAVEDATA_CBRESULT_OK_LAST_NOCONFIRM  =  2,
	CELL_SAVEDATA_CBRESULT_OK_LAST            =  1,
	CELL_SAVEDATA_CBRESULT_OK_NEXT            =  0,
	CELL_SAVEDATA_CBRESULT_ERR_NOSPACE        = -1,
	CELL_SAVEDATA_CBRESULT_ERR_FAILURE        = -2,
	CELL_SAVEDATA_CBRESULT_ERR_BROKEN         = -3,
	CELL_SAVEDATA_CBRESULT_ERR_NODATA         = -4,
	CELL_SAVEDATA_CBRESULT_ERR_INVALID        = -5,
};

// Bind stat return codes
enum
{
	CELL_SAVEDATA_BINDSTAT_OK             = 0,
	CELL_SAVEDATA_BINDSTAT_ERR_CONSOLE    = 1 << 0,
	CELL_SAVEDATA_BINDSTAT_ERR_DISC       = 1 << 1,
	CELL_SAVEDATA_BINDSTAT_ERR_PROGRAM    = 1 << 2,
	CELL_SAVEDATA_BINDSTAT_ERR_NOACCOUNTI = 1 << 3,
	CELL_SAVEDATA_BINDSTAT_ERR_NOUSER     = 1 << 3,
	CELL_SAVEDATA_BINDSTAT_ERR_ACCOUNTID  = 1 << 4,
	CELL_SAVEDATA_BINDSTAT_ERR_OTHERS     = 1 << 4,
	CELL_SAVEDATA_BINDSTAT_ERR_NOUSERID   = 1 << 5,
	CELL_SAVEDATA_BINDSTAT_ERR_USERID     = 1 << 6,
	CELL_SAVEDATA_BINDSTAT_ERR_NOOWNER    = 1 << 8,
	CELL_SAVEDATA_BINDSTAT_ERR_OWNER      = 1 << 9,
	CELL_SAVEDATA_BINDSTAT_ERR_LOCALOWNER = 1 << 10,
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

	// CellSaveDataIsNewData
	CELL_SAVEDATA_ISNEWDATA_NO          = 0,
	CELL_SAVEDATA_ISNEWDATA_YES         = 1,

	// CellSaveDataFocusPosition
	CELL_SAVEDATA_FOCUSPOS_DIRNAME      = 0,
	CELL_SAVEDATA_FOCUSPOS_LISTHEAD     = 1,
	CELL_SAVEDATA_FOCUSPOS_LISTTAIL     = 2,
	CELL_SAVEDATA_FOCUSPOS_LATEST       = 3,
	CELL_SAVEDATA_FOCUSPOS_OLDEST       = 4,
	CELL_SAVEDATA_FOCUSPOS_NEWDATA      = 5,

	// CellSaveDataFileOperation
	CELL_SAVEDATA_FILEOP_READ           = 0,
	CELL_SAVEDATA_FILEOP_WRITE          = 1,
	CELL_SAVEDATA_FILEOP_DELETE         = 2,
	CELL_SAVEDATA_FILEOP_WRITE_NOTRUNC  = 3,

	// CellSaveDataFileType
	CELL_SAVEDATA_FILETYPE_SECUREFILE     = 0,
	CELL_SAVEDATA_FILETYPE_NORMALFILE     = 1,
	CELL_SAVEDATA_FILETYPE_CONTENT_ICON0  = 2,
	CELL_SAVEDATA_FILETYPE_CONTENT_ICON1  = 3,
	CELL_SAVEDATA_FILETYPE_CONTENT_PIC1   = 4,
	CELL_SAVEDATA_FILETYPE_CONTENT_SND0   = 5,

	// CellSaveDataSystemFileParam attribute
	CELL_SAVEDATA_ATTR_NORMAL              = 0,
	CELL_SAVEDATA_ATTR_NODUPLICATE         = 1,

	// reCreateMode
	CELL_SAVEDATA_RECREATE_NO              = 0,
	CELL_SAVEDATA_RECREATE_NO_NOBROKEN     = 1,
	CELL_SAVEDATA_RECREATE_YES             = 2,
	CELL_SAVEDATA_RECREATE_YES_RESET_OWNER = 3,
	CELL_SAVEDATA_RECREATE_MASK            = 0xfffeffff,

	// Version
	CELL_SAVEDATA_VERSION_OLD = 0,
	CELL_SAVEDATA_VERSION_420 = 1,

	// Error Dialog
	CELL_SAVEDATA_ERRDIALOG_NONE     = 0,
	CELL_SAVEDATA_ERRDIALOG_ALWAYS   = 1,
	CELL_SAVEDATA_ERRDIALOG_NOREPEAT = 2,

	// Options for funcFixed dialog
	CELL_SAVEDATA_OPTION_NONE      = 0,
	CELL_SAVEDATA_OPTION_NOCONFIRM = 1,

	// CellSaveDataAutoIndicatorDispPosition
	CELL_SAVEDATA_INDICATORPOS_LOWER_RIGHT = 0,
	CELL_SAVEDATA_INDICATORPOS_LOWER_LEFT  = 1,
	CELL_SAVEDATA_INDICATORPOS_UPPER_RIGHT = 2,
	CELL_SAVEDATA_INDICATORPOS_UPPER_LEFT  = 3,
	CELL_SAVEDATA_INDICATORPOS_CENTER      = 4,

	// CellSaveDataAutoIndicatorDispMsgAlign
	CELL_SAVEDATA_INDICATORPOS_MSGALIGN_LEFT   = 0 << 4,
	CELL_SAVEDATA_INDICATORPOS_MSGALIGN_RIGHT  = 1 << 4,
	CELL_SAVEDATA_INDICATORPOS_MSGALIGN_CENTER = 2 << 4,

	// CellSaveDataAutoIndicatorDispMode
	CELL_SAVEDATA_INDICATORMODE_FIXED      = 0,
	CELL_SAVEDATA_INDICATORMODE_BLINK      = 1,
	CELL_SAVEDATA_INDICATORMODE_BLINK_FAST = 2,
	CELL_SAVEDATA_INDICATORMODE_BLINK_SLOW = 3,

	// Trophy ownership
	CELL_SAVEDATA_DISABLE_TROPHY_OWNERSHIP_CHECK = 1 << 16,
};

// CellSaveDataListNewData::iconPosition
enum : u32
{
	CELL_SAVEDATA_ICONPOS_HEAD = 0,
	CELL_SAVEDATA_ICONPOS_TAIL = 1,
};

// Datatypes
struct CellSaveDataSetList
{
	be_t<u32> sortType;
	be_t<u32> sortOrder;
	vm::bptr<char> dirNamePrefix;
	vm::bptr<void> reserved;
};

struct CellSaveDataSetBuf
{
	be_t<u32> dirListMax;
	be_t<u32> fileListMax;
	be_t<u32> reserved[6];
	be_t<u32> bufSize;
	vm::bptr<void> buf;
};

struct CellSaveDataNewDataIcon
{
	vm::bptr<char> title;
	be_t<u32> iconBufSize;
	vm::bptr<void> iconBuf;
	vm::bptr<void> reserved;
};

struct CellSaveDataListNewData
{
	be_t<u32> iconPosition;
	vm::bptr<char> dirName;
	vm::bptr<CellSaveDataNewDataIcon> icon;
	vm::bptr<void> reserved;
};

struct CellSaveDataDirList
{
	char dirName[CELL_SAVEDATA_DIRNAME_SIZE];
	char listParam[CELL_SAVEDATA_SYSP_LPARAM_SIZE];
	char reserved[8];
};

struct CellSaveDataListGet
{
	be_t<u32> dirNum;
	be_t<u32> dirListNum;
	vm::bptr<CellSaveDataDirList> dirList;
	char reserved[64];
};

struct CellSaveDataListSet
{
	be_t<u32> focusPosition;
	vm::bptr<char> focusDirName;
	be_t<u32> fixedListNum;
	vm::bptr<CellSaveDataDirList> fixedList;
	vm::bptr<CellSaveDataListNewData> newData;
	vm::bptr<void> reserved;
};

struct CellSaveDataFixedSet
{
	vm::bptr<char> dirName;
	vm::bptr<CellSaveDataNewDataIcon> newIcon;
	be_t<u32> option;
};

struct CellSaveDataSystemFileParam
{
	char title[CELL_SAVEDATA_SYSP_TITLE_SIZE];
	char subTitle[CELL_SAVEDATA_SYSP_SUBTITLE_SIZE];
	char detail[CELL_SAVEDATA_SYSP_DETAIL_SIZE];
	be_t<u32> attribute;
	be_t<u32> parental_level; // char reserved2[4] in firmware 3.70 or higher
	char listParam[CELL_SAVEDATA_SYSP_LPARAM_SIZE];
	char reserved[256];
};

struct CellSaveDataDirStat
{
	be_t<s64> atime;
	be_t<s64> mtime;
	be_t<s64> ctime;
	char dirName[CELL_SAVEDATA_DIRNAME_SIZE];
};

struct CellSaveDataFileStat
{
	be_t<u32> fileType;
	char reserved1[4];
	be_t<u64> size;
	be_t<s64> atime;
	be_t<s64> mtime;
	be_t<s64> ctime;
	char fileName[CELL_SAVEDATA_FILENAME_SIZE];
	char reserved2[3];
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
	vm::bptr<CellSaveDataFileStat> fileList;
	char reserved[64];
};

struct CellSaveDataAutoIndicator
{
	be_t<u32> dispPosition;
	be_t<u32> dispMode;
	vm::bptr<char> dispMsg;
	be_t<u32> picBufSize;
	vm::bptr<void> picBuf;
	vm::bptr<void> reserved;
};

struct CellSaveDataStatSet
{
	vm::bptr<CellSaveDataSystemFileParam> setParam;
	be_t<u32> reCreateMode;
	vm::bptr<CellSaveDataAutoIndicator> indicator;
};

struct CellSaveDataFileGet
{
	be_t<u32> excSize;
	char reserved[64];
};

struct CellSaveDataFileSet
{
	be_t<u32> fileOperation;
	vm::bptr<void> reserved;
	be_t<u32> fileType;
	be_t<u128, 1> secureFileId;
	vm::bptr<char> fileName;
	be_t<u32> fileOffset;
	be_t<u32> fileSize;
	be_t<u32> fileBufSize;
	vm::bptr<void> fileBuf;
};

struct CellSaveDataCBResult
{
	be_t<s32> result;
	be_t<u32> progressBarInc;
	be_t<s32> errNeedSizeKB;
	vm::bptr<char> invalidMsg;
	vm::bptr<void> userdata;
};

struct CellSaveDataDoneGet
{
	be_t<s32> excResult;
	char dirName[CELL_SAVEDATA_DIRNAME_SIZE];
	be_t<s32> sizeKB;
	be_t<s32> hddFreeSizeKB;
	char reserved[64];
};


// Callback Functions
using CellSaveDataFixedCallback = void(vm::ptr<CellSaveDataCBResult> cbResult, vm::ptr<CellSaveDataListGet> get, vm::ptr<CellSaveDataFixedSet> set);
using CellSaveDataListCallback = void(vm::ptr<CellSaveDataCBResult> cbResult, vm::ptr<CellSaveDataListGet> get, vm::ptr<CellSaveDataListSet> set);
using CellSaveDataStatCallback = void(vm::ptr<CellSaveDataCBResult> cbResult, vm::ptr<CellSaveDataStatGet> get, vm::ptr<CellSaveDataStatSet> set);
using CellSaveDataFileCallback = void(vm::ptr<CellSaveDataCBResult> cbResult, vm::ptr<CellSaveDataFileGet> get, vm::ptr<CellSaveDataFileSet> set);
using CellSaveDataDoneCallback = void(vm::ptr<CellSaveDataCBResult> cbResult, vm::ptr<CellSaveDataDoneGet> get);


// Auxiliary Structs
struct SaveDataEntry
{
	std::string escaped;

	std::string dirName;
	std::string listParam;
	std::string title;
	std::string subtitle;
	std::string details;
	u64 size{0};
	s64 atime{0};
	s64 mtime{0};
	s64 ctime{0};
	std::vector<uchar> iconBuf;
	bool isNew{false};

	std::string date() const;
	std::string data_size() const;
};

class SaveDialogBase
{
public:
	virtual ~SaveDialogBase();

	virtual s32 ShowSaveDataList(const std::string& base_dir, std::vector<SaveDataEntry>& save_entries, s32 focused, u32 op, vm::ptr<CellSaveDataListSet> listSet, bool enable_overlay) = 0;
};
