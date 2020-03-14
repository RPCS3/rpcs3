#pragma once

#include "Emu/Memory/vm_ptr.h"

// error codes
enum CellOskDialogError : u32
{
	CELL_OSKDIALOG_ERROR_IME_ALREADY_IN_USE = 0x8002b501,
	CELL_OSKDIALOG_ERROR_GET_SIZE_ERROR     = 0x8002b502,
	CELL_OSKDIALOG_ERROR_UNKNOWN            = 0x8002b503,
	CELL_OSKDIALOG_ERROR_PARAM              = 0x8002b504,
};

// OSK status for the callback
enum
{
	CELL_SYSUTIL_OSKDIALOG_LOADED               = 0x0502,
	CELL_SYSUTIL_OSKDIALOG_FINISHED             = 0x0503,
	CELL_SYSUTIL_OSKDIALOG_UNLOADED             = 0x0504,
	CELL_SYSUTIL_OSKDIALOG_INPUT_ENTERED        = 0x0505,
	CELL_SYSUTIL_OSKDIALOG_INPUT_CANCELED       = 0x0506,
	CELL_SYSUTIL_OSKDIALOG_INPUT_DEVICE_CHANGED = 0x0507,
	CELL_SYSUTIL_OSKDIALOG_DISPLAY_CHANGED      = 0x0508,
};

enum CellOskDialogInputFieldResult
{
	CELL_OSKDIALOG_INPUT_FIELD_RESULT_OK = 0,
	CELL_OSKDIALOG_INPUT_FIELD_RESULT_CANCELED = 1,
	CELL_OSKDIALOG_INPUT_FIELD_RESULT_ABORT = 2,
	CELL_OSKDIALOG_INPUT_FIELD_RESULT_NO_INPUT_TEXT = 3,
};

enum CellOskDialogInitialKeyLayout
{
	CELL_OSKDIALOG_INITIAL_PANEL_LAYOUT_SYSTEM = 0,
	CELL_OSKDIALOG_INITIAL_PANEL_LAYOUT_10KEY = 1,
	CELL_OSKDIALOG_INITIAL_PANEL_LAYOUT_FULLKEY = 2,
};

enum CellOskDialogInputDevice
{
	CELL_OSKDIALOG_INPUT_DEVICE_PAD = 0,
	CELL_OSKDIALOG_INPUT_DEVICE_KEYBOARD = 1,
};

enum CellOskDialogContinuousMode
{
	CELL_OSKDIALOG_CONTINUOUS_MODE_NONE = 0,
	CELL_OSKDIALOG_CONTINUOUS_MODE_REMAIN_OPEN = 1,
	CELL_OSKDIALOG_CONTINUOUS_MODE_HIDE = 2,
	CELL_OSKDIALOG_CONTINUOUS_MODE_SHOW = 3,
};

enum CellOskDialogDisplayStatus
{
	CELL_OSKDIALOG_DISPLAY_STATUS_HIDE = 0,
	CELL_OSKDIALOG_DISPLAY_STATUS_SHOW = 1,
};

enum CellOskDialogFilterCallbackReturnValue
{
	CELL_OSKDIALOG_NOT_CHANGE = 0,
	CELL_OSKDIALOG_CHANGE_WORD = 1,
};

enum CellOskDialogActionValue
{
	CELL_OSKDIALOG_CHANGE_NO_EVENT = 0,
	CELL_OSKDIALOG_CHANGE_EVENT_CANCEL = 1,
	CELL_OSKDIALOG_CHANGE_WORDS_INPUT = 3,
	CELL_OSKDIALOG_CHANGE_WORDS_INSERT = 4,
	CELL_OSKDIALOG_CHANGE_WORDS_REPLACE_ALL = 6,
};

enum CellOskDialogFinishReason
{
	CELL_OSKDIALOG_CLOSE_CONFIRM = 0,
	CELL_OSKDIALOG_CLOSE_CANCEL = 1,
};

enum CellOskDialogType
{
	CELL_OSKDIALOG_TYPE_SINGLELINE_OSK = 0,
	CELL_OSKDIALOG_TYPE_MULTILINE_OSK = 1,
	CELL_OSKDIALOG_TYPE_FULL_KEYBOARD_SINGLELINE_OSK = 2,
	CELL_OSKDIALOG_TYPE_FULL_KEYBOARD_MULTILINE_OSK = 3,
	CELL_OSKDIALOG_TYPE_SEPARATE_SINGLELINE_TEXT_WINDOW = 4,
	CELL_OSKDIALOG_TYPE_SEPARATE_MULTILINE_TEXT_WINDOW = 5,
	CELL_OSKDIALOG_TYPE_SEPARATE_INPUT_PANEL_WINDOW = 6,
	CELL_OSKDIALOG_TYPE_SEPARATE_FULL_KEYBOARD_INPUT_PANEL_WINDOW = 7,
	CELL_OSKDIALOG_TYPE_SEPARATE_CANDIDATE_WINDOW = 8,
};

enum
{
	CELL_OSKDIALOG_STRING_SIZE = 512, // Theroretical maximum for osk input, games can specify a lower limit
};

enum
{
	CELL_OSKDIALOG_PANELMODE_DEFAULT             = 0x00000000,
	CELL_OSKDIALOG_PANELMODE_GERMAN              = 0x00000001,
	CELL_OSKDIALOG_PANELMODE_ENGLISH             = 0x00000002,
	CELL_OSKDIALOG_PANELMODE_SPANISH             = 0x00000004,
	CELL_OSKDIALOG_PANELMODE_FRENCH              = 0x00000008,
	CELL_OSKDIALOG_PANELMODE_ITALIAN             = 0x00000010,
	CELL_OSKDIALOG_PANELMODE_DUTCH               = 0x00000020,
	CELL_OSKDIALOG_PANELMODE_PORTUGUESE          = 0x00000040,
	CELL_OSKDIALOG_PANELMODE_RUSSIAN             = 0x00000080,
	CELL_OSKDIALOG_PANELMODE_JAPANESE            = 0x00000100,
	CELL_OSKDIALOG_PANELMODE_DEFAULT_NO_JAPANESE = 0x00000200,
	CELL_OSKDIALOG_PANELMODE_POLISH              = 0x00000400,
	CELL_OSKDIALOG_PANELMODE_KOREAN              = 0x00001000,
	CELL_OSKDIALOG_PANELMODE_TURKEY              = 0x00002000,
	CELL_OSKDIALOG_PANELMODE_TRADITIONAL_CHINESE = 0x00004000,
	CELL_OSKDIALOG_PANELMODE_SIMPLIFIED_CHINESE  = 0x00008000,
	CELL_OSKDIALOG_PANELMODE_PORTUGUESE_BRAZIL   = 0x00010000,
	CELL_OSKDIALOG_PANELMODE_DANISH              = 0x00020000,
	CELL_OSKDIALOG_PANELMODE_SWEDISH             = 0x00040000,
	CELL_OSKDIALOG_PANELMODE_NORWEGIAN           = 0x00080000,
	CELL_OSKDIALOG_PANELMODE_FINNISH             = 0x00100000,
	CELL_OSKDIALOG_PANELMODE_JAPANESE_HIRAGANA   = 0x00200000,
	CELL_OSKDIALOG_PANELMODE_JAPANESE_KATAKANA   = 0x00400000,
	CELL_OSKDIALOG_PANELMODE_ALPHABET_FULL_WIDTH = 0x00800000,
	CELL_OSKDIALOG_PANELMODE_ALPHABET            = 0x01000000,
	CELL_OSKDIALOG_PANELMODE_LATIN               = 0x02000000,
	CELL_OSKDIALOG_PANELMODE_NUMERAL_FULL_WIDTH  = 0x04000000,
	CELL_OSKDIALOG_PANELMODE_NUMERAL             = 0x08000000,
	CELL_OSKDIALOG_PANELMODE_URL                 = 0x10000000,
	CELL_OSKDIALOG_PANELMODE_PASSWORD            = 0x20000000
};

enum
{
	CELL_OSKDIALOG_LAYOUTMODE_X_ALIGN_LEFT   = 0x00000200,
	CELL_OSKDIALOG_LAYOUTMODE_X_ALIGN_CENTER = 0x00000400,
	CELL_OSKDIALOG_LAYOUTMODE_X_ALIGN_RIGHT  = 0x00000800,
	CELL_OSKDIALOG_LAYOUTMODE_Y_ALIGN_TOP    = 0x00001000,
	CELL_OSKDIALOG_LAYOUTMODE_Y_ALIGN_CENTER = 0x00002000,
	CELL_OSKDIALOG_LAYOUTMODE_Y_ALIGN_BOTTOM = 0x00004000
};

enum
{
	CELL_OSKDIALOG_NO_SPACE          = 0x00000001,
	CELL_OSKDIALOG_NO_RETURN         = 0x00000002,
	CELL_OSKDIALOG_NO_INPUT_ANALOG   = 0x00000008,
	CELL_OSKDIALOG_NO_STARTUP_EFFECT = 0x00001000,
};

enum
{
	CELL_OSKDIALOG_10KEY_PANEL   = 0x00000001,
	CELL_OSKDIALOG_FULLKEY_PANEL = 0x00000002
};

enum
{
	CELL_OSKDIALOG_DEVICE_MASK_PAD = 0x000000ff
};

enum
{
	CELL_OSKDIALOG_EVENT_HOOK_TYPE_FUNCTION_KEY  = 0x00000001,
	CELL_OSKDIALOG_EVENT_HOOK_TYPE_ASCII_KEY     = 0x00000002,
	CELL_OSKDIALOG_EVENT_HOOK_TYPE_ONLY_MODIFIER = 0x00000004
};

static const f32 CELL_OSKDIALOG_SCALE_MIN = 0.80f;
static const f32 CELL_OSKDIALOG_SCALE_MAX = 1.05f;

struct CellOskDialogInputFieldInfo
{
	vm::bptr<u16> message;
	vm::bptr<u16> init_text;
	be_t<u32> limit_length;
};

struct CellOskDialogPoint
{
	be_t<f32> x;
	be_t<f32> y;
};

struct CellOskDialogParam
{
	be_t<u32> allowOskPanelFlg;
	be_t<u32> firstViewPanel;
	CellOskDialogPoint controlPoint;
	be_t<s32> prohibitFlgs;
};

// Actual input data
struct CellOskDialogCallbackReturnParam
{
	be_t<s32> result; // CellOskDialogInputFieldResult
	be_t<s32> numCharsResultString;
	vm::bptr<u16> pResultString;
};

struct CellOskDialogLayoutInfo
{
	be_t<s32> layoutMode;
	CellOskDialogPoint position;
};

struct CellOskDialogSeparateWindowOption
{
	be_t<u32> continuousMode; // CellOskDialogContinuousMode
	be_t<s32> deviceMask;
	be_t<s32> inputFieldWindowWidth;
	be_t<f32> inputFieldBackgroundTrans;
	vm::bptr<CellOskDialogLayoutInfo> inputFieldLayoutInfo;
	vm::bptr<CellOskDialogLayoutInfo> inputPanelLayoutInfo;
	vm::bptr<void> reserved;
};

struct CellOskDialogKeyMessage
{
	be_t<u32> led;
	be_t<u32> mkey;
	be_t<u16> keycode;
};

struct CellOskDialogImeDictionaryInfo
{
	be_t<u32> targetLanguage;
	vm::bcptr<char> dictionaryPath;
};

using cellOskDialogConfirmWordFilterCallback = int(vm::ptr<u16> pConfirmString, s32 wordLength);
using cellOskDialogHardwareKeyboardEventHookCallback = class b8(vm::ptr<CellOskDialogKeyMessage> keyMessage, vm::ptr<u32> action, vm::ptr<void> pActionInfo);
using cellOskDialogForceFinishCallback = class b8();

enum class OskDialogState
{
	Open,
	Abort,
	Close,
};

class OskDialogBase
{
public:
	virtual void Create(const std::string& title, const std::u16string& message, char16_t* init_text, u32 charlimit, u32 prohibit_flags, u32 panel_flag, u32 first_view_panel) = 0;
	virtual void Close(bool accepted) = 0;
	virtual ~OskDialogBase();

	std::function<void(s32 status)> on_osk_close;
	std::function<void()> on_osk_input_entered;

	atomic_t<OskDialogState> state{ OskDialogState::Close };

	atomic_t<CellOskDialogInputFieldResult> osk_input_result{ CellOskDialogInputFieldResult::CELL_OSKDIALOG_INPUT_FIELD_RESULT_OK };
	char16_t osk_text[CELL_OSKDIALOG_STRING_SIZE]{};
	char16_t osk_text_old[CELL_OSKDIALOG_STRING_SIZE]{};
};
