#pragma once

namespace vm { using namespace ps3; }

enum
{
	CELL_SYSUTIL_ERROR_TYPE   = 0x8002b101,
	CELL_SYSUTIL_ERROR_VALUE  = 0x8002b102,
	CELL_SYSUTIL_ERROR_SIZE   = 0x8002b103,
	CELL_SYSUTIL_ERROR_NUM    = 0x8002b104,
	CELL_SYSUTIL_ERROR_BUSY   = 0x8002b105,
	CELL_SYSUTIL_ERROR_STATUS = 0x8002b106,
	CELL_SYSUTIL_ERROR_MEMORY = 0x8002b107,
};

// Parameter IDs
enum
{
	// Integers
	CELL_SYSUTIL_SYSTEMPARAM_ID_LANG                            = 0x0111,
	CELL_SYSUTIL_SYSTEMPARAM_ID_ENTER_BUTTON_ASSIGN             = 0x0112,
	CELL_SYSUTIL_SYSTEMPARAM_ID_DATE_FORMAT                     = 0x0114,
	CELL_SYSUTIL_SYSTEMPARAM_ID_TIME_FORMAT                     = 0x0115,
	CELL_SYSUTIL_SYSTEMPARAM_ID_TIMEZONE                        = 0x0116,
	CELL_SYSUTIL_SYSTEMPARAM_ID_SUMMERTIME                      = 0x0117,
	CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL             = 0x0121,
	CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL0_RESTRICT   = 0x0123,
	CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USER_HAS_NP_ACCOUNT     = 0x0141,
	CELL_SYSUTIL_SYSTEMPARAM_ID_CAMERA_PLFREQ                   = 0x0151,
	CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_RUMBLE                      = 0x0152,
	CELL_SYSUTIL_SYSTEMPARAM_ID_KEYBOARD_TYPE                   = 0x0153,
	CELL_SYSUTIL_SYSTEMPARAM_ID_JAPANESE_KEYBOARD_ENTRY_METHOD  = 0x0154,
	CELL_SYSUTIL_SYSTEMPARAM_ID_CHINESE_KEYBOARD_ENTRY_METHOD   = 0x0155,
	CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_AUTOOFF                     = 0x0156,

	// Strings
	CELL_SYSUTIL_SYSTEMPARAM_ID_NICKNAME                        = 0x0113,
	CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USERNAME                = 0x0131,
};

enum
{
	CELL_SYSUTIL_LANG_JAPANESE       = 0,
	CELL_SYSUTIL_LANG_ENGLISH_US     = 1,
	CELL_SYSUTIL_LANG_FRENCH         = 2,
	CELL_SYSUTIL_LANG_SPANISH        = 3,
	CELL_SYSUTIL_LANG_GERMAN         = 4,
	CELL_SYSUTIL_LANG_ITALIAN        = 5,
	CELL_SYSUTIL_LANG_DUTCH          = 6,
	CELL_SYSUTIL_LANG_PORTUGUESE_PT  = 7,
	CELL_SYSUTIL_LANG_RUSSIAN        = 8,
	CELL_SYSUTIL_LANG_KOREAN         = 9,
	CELL_SYSUTIL_LANG_CHINESE_T      = 10,
	CELL_SYSUTIL_LANG_CHINESE_S      = 11,
	CELL_SYSUTIL_LANG_FINNISH        = 12,
	CELL_SYSUTIL_LANG_SWEDISH        = 13,
	CELL_SYSUTIL_LANG_DANISH         = 14,
	CELL_SYSUTIL_LANG_NORWEGIAN      = 15,
	CELL_SYSUTIL_LANG_POLISH         = 16,
	CELL_SYSUTIL_LANG_PORTUGUESE_BR  = 17,
	CELL_SYSUTIL_LANG_ENGLISH_GB     = 18,
};

enum
{
	CELL_SYSUTIL_REQUEST_EXITGAME  = 0x0101,
	CELL_SYSUTIL_DRAWING_BEGIN     = 0x0121,
	CELL_SYSUTIL_DRAWING_END       = 0x0122,
	CELL_SYSUTIL_SYSTEM_MENU_OPEN  = 0x0131,
	CELL_SYSUTIL_SYSTEM_MENU_CLOSE = 0x0132,
	CELL_SYSUTIL_BGMPLAYBACK_PLAY  = 0x0141,
	CELL_SYSUTIL_BGMPLAYBACK_STOP  = 0x0142,

	CELL_SYSUTIL_NP_INVITATION_SELECTED   = 0x0151,
	CELL_SYSUTIL_NP_DATA_MESSAGE_SELECTED = 0x0152,

	CELL_SYSUTIL_SYSCHAT_START                   = 0x0161,
	CELL_SYSUTIL_SYSCHAT_STOP                    = 0x0162,
	CELL_SYSUTIL_SYSCHAT_VOICE_STREAMING_RESUMED = 0x0163,
	CELL_SYSUTIL_SYSCHAT_VOICE_STREAMING_PAUSED  = 0x0164,
};

using CellSysutilCallback = func_def<void(u64 status, u64 param, vm::ptr<void> userdata)>;

void sysutilSendSystemCommand(u64 status, u64 param);

enum
{
	CELL_SYSUTIL_ENTER_BUTTON_ASSIGN_CIRCLE = 0,
	CELL_SYSUTIL_ENTER_BUTTON_ASSIGN_CROSS  = 1,
};

enum
{
	CELL_SYSUTIL_DATE_FMT_YYYYMMDD = 0,
	CELL_SYSUTIL_DATE_FMT_DDMMYYYY = 1,
	CELL_SYSUTIL_DATE_FMT_MMDDYYYY = 2,
};

enum
{
	CELL_SYSUTIL_TIME_FMT_CLOCK12 = 0,
	CELL_SYSUTIL_TIME_FMT_CLOCK24 = 1,
};

enum
{
	CELL_SYSUTIL_GAME_PARENTAL_OFF      = 0,
	CELL_SYSUTIL_GAME_PARENTAL_LEVEL01  = 1,
	CELL_SYSUTIL_GAME_PARENTAL_LEVEL02  = 2,
	CELL_SYSUTIL_GAME_PARENTAL_LEVEL03  = 3,
	CELL_SYSUTIL_GAME_PARENTAL_LEVEL04  = 4,
	CELL_SYSUTIL_GAME_PARENTAL_LEVEL05  = 5,
	CELL_SYSUTIL_GAME_PARENTAL_LEVEL06  = 6,
	CELL_SYSUTIL_GAME_PARENTAL_LEVEL07  = 7,
	CELL_SYSUTIL_GAME_PARENTAL_LEVEL08  = 8,
	CELL_SYSUTIL_GAME_PARENTAL_LEVEL09  = 9,
	CELL_SYSUTIL_GAME_PARENTAL_LEVEL10  = 10,
	CELL_SYSUTIL_GAME_PARENTAL_LEVEL11  = 11,
};

enum
{
	CELL_SYSUTIL_GAME_PARENTAL_LEVEL0_RESTRICT_OFF  = 0,
	CELL_SYSUTIL_GAME_PARENTAL_LEVEL0_RESTRICT_ON   = 1,
};

enum
{
	CELL_SYSUTIL_CAMERA_PLFREQ_DISABLED       = 0,
	CELL_SYSUTIL_CAMERA_PLFREQ_50HZ           = 1,
	CELL_SYSUTIL_CAMERA_PLFREQ_60HZ           = 2,
	CELL_SYSUTIL_CAMERA_PLFREQ_DEVCIE_DEPEND  = 4,
};

enum
{
	CELL_SYSUTIL_PAD_RUMBLE_OFF  = 0,
	CELL_SYSUTIL_PAD_RUMBLE_ON   = 1,
};

enum
{
	CELL_SYSCACHE_RET_OK_CLEARED      = 0,
	CELL_SYSCACHE_RET_OK_RELAYED      = 1, 

	CELL_SYSCACHE_ID_SIZE             = 32,	
	CELL_SYSCACHE_PATH_MAX            = 1055,

	CELL_SYSCACHE_ERROR_ACCESS_ERROR  = 0x8002bc01, // I don't think we need this
	CELL_SYSCACHE_ERROR_INTERNAL      = 0x8002bc02, // Not really useful, if we run out of HDD space sys_fs should handle that

	CELL_SYSCACHE_ERROR_PARAM         = 0x8002bc03,
	CELL_SYSCACHE_ERROR_NOTMOUNTED    = 0x8002bc04, // We don't really need to simulate the mounting, so this is probably useless
};

using CellWebBrowserCallback = func_def<void(s32 cb_type, vm::ptr<void> client_session, vm::ptr<void> usrdata)>;
using CellWebComponentCallback = func_def<void(s32 web_browser_id, s32 cb_type, vm::ptr<void> client_session, vm::ptr<void> usrdata)>;
using CellWebBrowserSystemCallback = func_def<void(s32 cb_type, vm::ptr<void> usrdata)>;

using CellWebBrowserMIMETypeCallback = func_def<void(vm::cptr<char> mimetype, vm::cptr<char> url, vm::ptr<void> usrdata)>;
using CellWebBrowserErrorCallback = func_def<void(s32 err_type, vm::ptr<void> usrdata)>;
using CellWebBrowserStatusCallback = func_def<void(s32 err_type, vm::ptr<void> usrdata)>;
using CellWebBrowserNotify = func_def<void(vm::cptr<char> message, vm::ptr<void> usrdata)>;
using CellWebBrowserUsrdata = func_def<void(vm::ptr<void> usrdata)>;

struct CellWebBrowserMimeSet
{
	vm::bcptr<char> type;
	vm::bcptr<char> directory;
};

struct CellWebBrowserPos
{
	be_t<s32> x;
	be_t<s32> y;
};

struct CellWebBrowserSize
{
	be_t<s32> width;
	be_t<s32> height;
};

struct CellWebBrowserRect
{
	CellWebBrowserPos pos;
	CellWebBrowserSize size;
};

struct CellWebBrowserConfig
{
	be_t<s32> version;
	be_t<s32> heap_size;
	vm::bcptr<CellWebBrowserMimeSet> mimesets;
	be_t<s32> mimeset_num;
	be_t<s32> functions;
	be_t<s32> tab_count;
	vm::bptr<CellWebBrowserCallback> exit_cb;
	vm::bptr<CellWebBrowserCallback> download_cb;
	vm::bptr<CellWebBrowserCallback> navigated_cb;
};

struct CellWebBrowserConfig2
{
	be_t<s32> version;
	be_t<s32> heap_size;
	be_t<s32> functions;
	be_t<s32> tab_count;
	be_t<s32> size_mode;
	be_t<s32> view_restriction;
	vm::bptr<CellWebBrowserMIMETypeCallback> unknown_mimetype_cb;
	vm::bptr<CellWebBrowserErrorCallback> error_cb;
	vm::bptr<CellWebBrowserStatusCallback> status_error_cb;
	vm::bptr<CellWebBrowserNotify> notify_cb;
	vm::bptr<CellWebBrowserCallback> request_cb;
	CellWebBrowserRect rect;
	be_t<float> resolution_factor;
	be_t<s32> magic_number_;
};

enum CellSysutilBgmPlaybackStatusState
{
	CELL_SYSUTIL_BGMPLAYBACK_STATUS_PLAY = 0,
	CELL_SYSUTIL_BGMPLAYBACK_STATUS_STOP = 1
};

enum CellSysutilBgmPlaybackStatusEnabled
{
	CELL_SYSUTIL_BGMPLAYBACK_STATUS_ENABLE = 0,
	CELL_SYSUTIL_BGMPLAYBACK_STATUS_DISABLE = 1
};

struct CellSysutilBgmPlaybackStatus
{
	u8 playerState;
	u8 enableState;
	char contentId[16];
	u8 currentFadeRatio;
	char reserved[13];
};

struct CellSysutilBgmPlaybackStatus2
{
	u8 playerState;
	char reserved[7];
};

struct CellSysutilBgmPlaybackExtraParam
{
	be_t<s32> systemBgmFadeInTime;
	be_t<s32> systemBgmFadeOutTime;
	be_t<s32> gameBgmFadeInTime;
	be_t<s32> gameBgmFadeOutTime;
	char reserved[8];
};
