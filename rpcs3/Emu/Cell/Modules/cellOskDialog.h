#pragma once



// error codes
enum
{
	CELL_OSKDIALOG_ERROR_IME_ALREADY_IN_USE = 0x8002b501,
	CELL_OSKDIALOG_ERROR_GET_SIZE_ERROR = 0x8002b502,
	CELL_OSKDIALOG_ERROR_UNKNOWN = 0x8002b503,
	CELL_OSKDIALOG_ERROR_PARAM = 0x8002b504,
};

// OSK status for the callback
enum
{
	CELL_SYSUTIL_OSKDIALOG_LOADED = 0x0502,
	CELL_SYSUTIL_OSKDIALOG_FINISHED = 0x0503,
	CELL_SYSUTIL_OSKDIALOG_UNLOADED = 0x0504,
	CELL_SYSUTIL_OSKDIALOG_INPUT_ENTERED = 0x0505,
	CELL_SYSUTIL_OSKDIALOG_INPUT_CANCELED = 0x0506,
	CELL_SYSUTIL_OSKDIALOG_INPUT_DEVICE_CHANGED = 0x0507,
	CELL_SYSUTIL_OSKDIALOG_DISPLAY_CHANGED = 0x0508,
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
	be_t<s32> continuousMode; //CellOskDialogContinuousMode
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
