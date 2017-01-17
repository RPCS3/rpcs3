#pragma once

namespace vm { using namespace ps3; }

//error codes
enum
{
};
 
//OSK status for the callback
enum 
{
	CELL_SYSUTIL_OSKDIALOG_LOADED				= 0x0502,
	CELL_SYSUTIL_OSKDIALOG_INPUT_ENTERED		= 0x0505,
	CELL_SYSUTIL_OSKDIALOG_INPUT_CANCELED		= 0x0506,
	CELL_SYSUTIL_OSKDIALOG_INPUT_DEVICE_CHANGED	= 0x0507,
	CELL_SYSUTIL_OSKDIALOG_DISPLAY_CHANGED		= 0x0508,
};

enum CellOskDialogInputFieldResult 
{
	CELL_OSKDIALOG_INPUT_FIELD_RESULT_OK			= 0,
	CELL_OSKDIALOG_INPUT_FIELD_RESULT_CANCELED		= 1,
	CELL_OSKDIALOG_INPUT_FIELD_RESULT_ABORT			= 2,
	CELL_OSKDIALOG_INPUT_FIELD_RESULT_NO_INPUT_TEXT = 3,
};

enum CellOskDialogInitialKeyLayout 
{
	CELL_OSKDIALOG_INITIAL_PANEL_LAYOUT_SYSTEM	= 0,
	CELL_OSKDIALOG_INITIAL_PANEL_LAYOUT_10KEY	= 1,
	CELL_OSKDIALOG_INITIAL_PANEL_LAYOUT_FULLKEY	= 2,
};

enum CellOskDialogInputDevice 
{
	CELL_OSKDIALOG_INPUT_DEVICE_KEYBOARD	= 1,
};

enum CellOskDialogContinuousMode 
{
	CELL_OSKDIALOG_CONTINUOUS_MODE_NONE			= 0,
	CELL_OSKDIALOG_CONTINUOUS_MODE_HIDE			= 2,
	CELL_OSKDIALOG_CONTINUOUS_MODE_SHOW			= 3,
};

enum CellOskDialogDisplayStatus 
{
	CELL_OSKDIALOG_DISPLAY_STATUS_HIDE = 0,
	CELL_OSKDIALOG_DISPLAY_STATUS_SHOW = 1,
};

enum CellOskDialogFilterCallbackReturnValue
{
};

enum CellOskDialogActionValue
{
	CELL_OSKDIALOG_CHANGE_NO_EVENT			= 0,
	CELL_OSKDIALOG_CHANGE_EVENT_CANCEL		= 1,
	CELL_OSKDIALOG_CHANGE_WORDS_INPUT		= 3,
	CELL_OSKDIALOG_CHANGE_WORDS_INSERT		= 4,
	CELL_OSKDIALOG_CHANGE_WORDS_REPLACE_ALL	= 6,
};

enum CellOskDialogFinishReason 
{
	CELL_OSKDIALOG_CLOSE_CANCEL	 = 1,
};

enum CellOskDialogType 
{
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

//Actual input data
struct CellOskDialogCallbackReturnParam
{
	CellOskDialogInputFieldResult result;
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
	CellOskDialogContinuousMode continuousMode;
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
	const vm::bptr<char> dictionaryPath;
};

