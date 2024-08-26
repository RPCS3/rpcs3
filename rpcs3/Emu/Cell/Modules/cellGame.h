#pragma once

#include "Emu/Memory/vm_ptr.h"

// Return Codes
enum
{
	CELL_GAME_RET_OK                   = 0,
	CELL_GAME_RET_CANCEL               = 1,
	CELL_GAME_RET_NONE                 = 2,
};

enum CellGameError : u32
{
	CELL_GAME_ERROR_NOTFOUND           = 0x8002cb04,
	CELL_GAME_ERROR_BROKEN             = 0x8002cb05,
	CELL_GAME_ERROR_INTERNAL           = 0x8002cb06,
	CELL_GAME_ERROR_PARAM              = 0x8002cb07,
	CELL_GAME_ERROR_NOAPP              = 0x8002cb08,
	CELL_GAME_ERROR_ACCESS_ERROR       = 0x8002cb09,
	CELL_GAME_ERROR_NOSPACE            = 0x8002cb20,
	CELL_GAME_ERROR_NOTSUPPORTED       = 0x8002cb21,
	CELL_GAME_ERROR_FAILURE            = 0x8002cb22,
	CELL_GAME_ERROR_BUSY               = 0x8002cb23,
	CELL_GAME_ERROR_IN_SHUTDOWN        = 0x8002cb24,
	CELL_GAME_ERROR_INVALID_ID         = 0x8002cb25,
	CELL_GAME_ERROR_EXIST              = 0x8002cb26,
	CELL_GAME_ERROR_NOTPATCH           = 0x8002cb27,
	CELL_GAME_ERROR_INVALID_THEME_FILE = 0x8002cb28,
	CELL_GAME_ERROR_BOOTPATH           = 0x8002cb50,
};

enum CellGameDataError : u32
{
	CELL_GAMEDATA_ERROR_CBRESULT     = 0x8002b601,
	CELL_GAMEDATA_ERROR_ACCESS_ERROR = 0x8002b602,
	CELL_GAMEDATA_ERROR_INTERNAL     = 0x8002b603,
	CELL_GAMEDATA_ERROR_PARAM        = 0x8002b604,
	CELL_GAMEDATA_ERROR_NOSPACE      = 0x8002b605,
	CELL_GAMEDATA_ERROR_BROKEN       = 0x8002b606,
	CELL_GAMEDATA_ERROR_FAILURE      = 0x8002b607,
};

enum CellDiscGameError : u32
{
	CELL_DISCGAME_ERROR_INTERNAL      = 0x8002bd01,
	CELL_DISCGAME_ERROR_NOT_DISCBOOT  = 0x8002bd02,
	CELL_DISCGAME_ERROR_PARAM         = 0x8002bd03,
};

// Definitions
enum
{
	CELL_GAME_PATH_MAX           = 128,
	CELL_GAME_DIRNAME_SIZE       = 32,
	CELL_GAME_HDDGAMEPATH_SIZE   = 128,
	CELL_GAME_THEMEFILENAME_SIZE = 48,

	CELL_GAME_SYSP_LANGUAGE_NUM  = 20,
	CELL_GAME_SYSP_TITLE_SIZE    = 128,
	CELL_GAME_SYSP_TITLEID_SIZE  = 10,
	CELL_GAME_SYSP_VERSION_SIZE  = 6,
	CELL_GAME_SYSP_PS3_SYSTEM_VER_SIZE = 8,
	CELL_GAME_SYSP_APP_VER_SIZE  = 6,

	CELL_GAME_GAMETYPE_SYS      = 0,
	CELL_GAME_GAMETYPE_DISC     = 1,
	CELL_GAME_GAMETYPE_HDD      = 2,
	CELL_GAME_GAMETYPE_GAMEDATA = 3,
	CELL_GAME_GAMETYPE_HOME     = 4,

	CELL_GAME_SIZEKB_NOTCALC = -1,

	CELL_GAME_THEMEINSTALL_BUFSIZE_MIN = 4096,

	CELL_GAME_ATTRIBUTE_PATCH               = 0x1,
	CELL_GAME_ATTRIBUTE_APP_HOME            = 0x2,
	CELL_GAME_ATTRIBUTE_DEBUG               = 0x4,
	CELL_GAME_ATTRIBUTE_XMBBUY              = 0x8,
	CELL_GAME_ATTRIBUTE_COMMERCE2_BROWSER   = 0x10,
	CELL_GAME_ATTRIBUTE_INVITE_MESSAGE      = 0x20,
	CELL_GAME_ATTRIBUTE_CUSTOM_DATA_MESSAGE = 0x40,
	CELL_GAME_ATTRIBUTE_WEB_BROWSER         = 0x100,

	CELL_GAME_THEME_OPTION_NONE  = 0x0,
	CELL_GAME_THEME_OPTION_APPLY = 0x1,

	CELL_GAME_DISCTYPE_OTHER = 0,
	CELL_GAME_DISCTYPE_PS3   = 1,
	CELL_GAME_DISCTYPE_PS2   = 2,
};

//Parameter IDs of PARAM.SFO
enum
{
	//Integers
	CELL_GAME_PARAMID_PARENTAL_LEVEL = 102,
	CELL_GAME_PARAMID_RESOLUTION     = 103,
	CELL_GAME_PARAMID_SOUND_FORMAT   = 104,

	//Strings
	CELL_GAME_PARAMID_TITLE                   = 0,
	CELL_GAME_PARAMID_TITLE_DEFAULT           = 1,
	CELL_GAME_PARAMID_TITLE_JAPANESE          = 2,
	CELL_GAME_PARAMID_TITLE_ENGLISH           = 3,
	CELL_GAME_PARAMID_TITLE_FRENCH            = 4,
	CELL_GAME_PARAMID_TITLE_SPANISH           = 5,
	CELL_GAME_PARAMID_TITLE_GERMAN            = 6,
	CELL_GAME_PARAMID_TITLE_ITALIAN           = 7,
	CELL_GAME_PARAMID_TITLE_DUTCH             = 8,
	CELL_GAME_PARAMID_TITLE_PORTUGUESE        = 9,
	CELL_GAME_PARAMID_TITLE_RUSSIAN           = 10,
	CELL_GAME_PARAMID_TITLE_KOREAN            = 11,
	CELL_GAME_PARAMID_TITLE_CHINESE_T         = 12,
	CELL_GAME_PARAMID_TITLE_CHINESE_S         = 13,
	CELL_GAME_PARAMID_TITLE_FINNISH           = 14,
	CELL_GAME_PARAMID_TITLE_SWEDISH           = 15,
	CELL_GAME_PARAMID_TITLE_DANISH            = 16,
	CELL_GAME_PARAMID_TITLE_NORWEGIAN         = 17,
	CELL_GAME_PARAMID_TITLE_POLISH            = 18,
	CELL_GAME_PARAMID_TITLE_PORTUGUESE_BRAZIL = 19, // FW 4.00
	CELL_GAME_PARAMID_TITLE_ENGLISH_UK        = 20, // FW 4.00
	CELL_GAME_PARAMID_TITLE_TURKISH           = 21, // FW 4.30
	CELL_GAME_PARAMID_TITLE_ID                = 100,
	CELL_GAME_PARAMID_VERSION                 = 101,
	CELL_GAME_PARAMID_PS3_SYSTEM_VER          = 105,
	CELL_GAME_PARAMID_APP_VER                 = 106,
};

//Error dialog types
enum
{
	CELL_GAME_ERRDIALOG_BROKEN_GAMEDATA      = 0,
	CELL_GAME_ERRDIALOG_BROKEN_HDDGAME       = 1,
	CELL_GAME_ERRDIALOG_NOSPACE              = 2,
	CELL_GAME_ERRDIALOG_BROKEN_EXIT_GAMEDATA = 100,
	CELL_GAME_ERRDIALOG_BROKEN_EXIT_HDDGAME  = 101,
	CELL_GAME_ERRDIALOG_NOSPACE_EXIT         = 102,
};

enum // CellGameResolution
{
	CELL_GAME_RESOLUTION_480   = 0x01,
	CELL_GAME_RESOLUTION_576   = 0x02,
	CELL_GAME_RESOLUTION_720   = 0x04,
	CELL_GAME_RESOLUTION_1080  = 0x08,
	CELL_GAME_RESOLUTION_480SQ = 0x10,
	CELL_GAME_RESOLUTION_576SQ = 0x20,
};

enum // CellGameSoundFormat
{
	CELL_GAME_SOUNDFORMAT_2LPCM    = 0x01,
	CELL_GAME_SOUNDFORMAT_51LPCM   = 0x04,
	CELL_GAME_SOUNDFORMAT_71LPCM   = 0x10,
	CELL_GAME_SOUNDFORMAT_51DDENC  = 0x102,
	CELL_GAME_SOUNDFORMAT_51DTSENC = 0x202,
};

struct CellGameContentSize
{
	be_t<s32> hddFreeSizeKB;
	be_t<s32> sizeKB;
	be_t<s32> sysSizeKB;
};

struct CellGameSetInitParams
{
	char title[CELL_GAME_SYSP_TITLE_SIZE];
	char titleId[CELL_GAME_SYSP_TITLEID_SIZE];
	char reserved0[2];
	char version[CELL_GAME_SYSP_VERSION_SIZE];
	char reserved1[66];
};

struct CellGameDataCBResult
{
	be_t<s32> result;
	be_t<s32> errNeedSizeKB;
	vm::bptr<char> invalidMsg;
	vm::bptr<void> reserved;
};

enum // old consts
{
	CELL_GAMEDATA_CBRESULT_OK_CANCEL   = 1,
	CELL_GAMEDATA_CBRESULT_OK          = 0,
	CELL_GAMEDATA_CBRESULT_ERR_NOSPACE = -1,
	CELL_GAMEDATA_CBRESULT_ERR_BROKEN  = -3,
	CELL_GAMEDATA_CBRESULT_ERR_NODATA  = -4,
	CELL_GAMEDATA_CBRESULT_ERR_INVALID = -5,

	CELL_GAMEDATA_RET_OK     = 0,
	CELL_GAMEDATA_RET_CANCEL = 1,

	CELL_GAMEDATA_ATTR_NORMAL     = 0,
	CELL_GAMEDATA_VERSION_CURRENT = 0,

	CELL_GAMEDATA_INVALIDMSG_MAX = 256,
	CELL_GAMEDATA_PATH_MAX       = 1055,
	CELL_GAMEDATA_DIRNAME_SIZE   = 32,

	CELL_GAMEDATA_SIZEKB_NOTCALC = -1,

	CELL_GAMEDATA_SYSP_LANGUAGE_NUM = 20,
	CELL_GAMEDATA_SYSP_TITLE_SIZE   = 128,
	CELL_GAMEDATA_SYSP_TITLEID_SIZE = 10,
	CELL_GAMEDATA_SYSP_VERSION_SIZE = 6,

	CELL_GAMEDATA_ISNEWDATA_NO  = 0,
	CELL_GAMEDATA_ISNEWDATA_YES = 1,

	CELL_GAMEDATA_ERRDIALOG_NONE   = 0,
	CELL_GAMEDATA_ERRDIALOG_ALWAYS = 1,

	CELL_DISCGAME_SYSP_TITLEID_SIZE=10,
};

enum
{
	TITLEID_SFO_ENTRY_SIZE = 16, // This is the true length on PS3 (TODO: Fix in more places)
};

struct CellGameDataSystemFileParam
{
	char title[CELL_GAMEDATA_SYSP_TITLE_SIZE];
	char titleLang[CELL_GAMEDATA_SYSP_LANGUAGE_NUM][CELL_GAMEDATA_SYSP_TITLE_SIZE];
	char titleId[CELL_GAMEDATA_SYSP_TITLEID_SIZE];
	char reserved0[2];
	char dataVersion[CELL_GAMEDATA_SYSP_VERSION_SIZE];
	char reserved1[2];
	be_t<u32> parentalLevel;
	be_t<u32> attribute;
	be_t<u32> resolution; // cellHddGameCheck member: GD doesn't have this value
	be_t<u32> soundFormat; // cellHddGameCheck member: GD doesn't have this value
	char reserved2[248];
};

struct CellDiscGameSystemFileParam
{
	char titleId[CELL_DISCGAME_SYSP_TITLEID_SIZE];
	char reserved0[2];
	be_t<u32> parentalLevel;
	char reserved1[16];
};

struct CellGameDataStatGet
{
	be_t<s32> hddFreeSizeKB;
	be_t<u32> isNewData;
	char contentInfoPath[CELL_GAMEDATA_PATH_MAX];
	char gameDataPath[CELL_GAMEDATA_PATH_MAX];
	char reserved0[2];
	be_t<s64> st_atime_;
	be_t<s64> st_mtime_;
	be_t<s64> st_ctime_;
	CellGameDataSystemFileParam getParam;
	be_t<s32> sizeKB;
	be_t<s32> sysSizeKB;
	char reserved1[68];
};

struct CellGameDataStatSet
{
	vm::bptr<CellGameDataSystemFileParam> setParam;
	be_t<u32> reserved;
};

typedef void(CellGameDataStatCallback)(vm::ptr<CellGameDataCBResult> cbResult, vm::ptr<CellGameDataStatGet> get, vm::ptr<CellGameDataStatSet> set);

// cellSysutil: cellHddGame

enum CellHddGameError : u32
{
	CELL_HDDGAME_ERROR_CBRESULT     = 0x8002ba01,
	CELL_HDDGAME_ERROR_ACCESS_ERROR = 0x8002ba02,
	CELL_HDDGAME_ERROR_INTERNAL     = 0x8002ba03,
	CELL_HDDGAME_ERROR_PARAM        = 0x8002ba04,
	CELL_HDDGAME_ERROR_NOSPACE      = 0x8002ba05,
	CELL_HDDGAME_ERROR_BROKEN       = 0x8002ba06,
	CELL_HDDGAME_ERROR_FAILURE      = 0x8002ba07,
};

enum
{
	// Return Codes
	CELL_HDDGAME_RET_OK     = 0,
	CELL_HDDGAME_RET_CANCEL = 1,

	// Callback Result
	CELL_HDDGAME_CBRESULT_OK_CANCEL    = 1,
	CELL_HDDGAME_CBRESULT_OK           = 0,
	CELL_HDDGAME_CBRESULT_ERR_NOSPACE  = -1,
	CELL_HDDGAME_CBRESULT_ERR_BROKEN   = -3,
	CELL_HDDGAME_CBRESULT_ERR_NODATA   = -4,
	CELL_HDDGAME_CBRESULT_ERR_INVALID  = -5,

	// Character Strings
	CELL_HDDGAME_INVALIDMSG_MAX        = 256,
	CELL_HDDGAME_PATH_MAX              = 1055,
	CELL_HDDGAME_SYSP_TITLE_SIZE       = 128,
	CELL_HDDGAME_SYSP_TITLEID_SIZE     = 10,
	CELL_HDDGAME_SYSP_VERSION_SIZE     = 6,
	CELL_HDDGAME_SYSP_SYSTEMVER_SIZE   = 8,

	// HDD Directory exists
	CELL_HDDGAME_ISNEWDATA_EXIST       = 0,
	CELL_HDDGAME_ISNEWDATA_NODIR       = 1,

	// Languages
	CELL_HDDGAME_SYSP_LANGUAGE_NUM     = 20,

	// Stat Get
	CELL_HDDGAME_SIZEKB_NOTCALC        = -1,
};

using CellHddGameStatGet = CellGameDataStatGet;
using CellHddGameStatSet = CellGameDataStatSet;
using CellHddGameSystemFileParam = CellGameDataSystemFileParam;
using CellHddGameCBResult = CellGameDataCBResult;

typedef void(CellHddGameStatCallback)(vm::ptr<CellHddGameCBResult> cbResult, vm::ptr<CellHddGameStatGet> get, vm::ptr<CellHddGameStatSet> set);
typedef s32(CellGameThemeInstallCallback)(u32 fileOffset, u32 readSize, vm::ptr<void> buf);
typedef void(CellGameDiscEjectCallback)();
typedef void(CellGameDiscInsertCallback)(u32 discType, vm::ptr<char> titleId);
using CellDiscGameDiscEjectCallback = CellGameDiscEjectCallback;
using CellDiscGameDiscInsertCallback = CellGameDiscInsertCallback;

struct disc_change_manager
{
	disc_change_manager();
	virtual ~disc_change_manager();

	std::mutex mtx;
	atomic_t<bool> is_inserting = false;
	vm::ptr<CellGameDiscEjectCallback> eject_callback = vm::null;
	vm::ptr<CellGameDiscInsertCallback> insert_callback = vm::null;

	enum class eject_state
	{
		unknown,
		inserted,
		ejected,
		busy
	};
	atomic_t<eject_state> state = eject_state::unknown;

	error_code register_callbacks(vm::ptr<CellGameDiscEjectCallback> func_eject, vm::ptr<CellGameDiscInsertCallback> func_insert);
	error_code unregister_callbacks();

	void eject_disc();
	void insert_disc(u32 disc_type, std::string title_id);
};
