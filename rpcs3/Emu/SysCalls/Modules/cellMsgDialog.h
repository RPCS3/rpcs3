#pragma once

enum
{
	CELL_MSGDIALOG_ERROR_PARAM             = 0x8002b301,
	CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED = 0x8002b302,
};

enum CellMsgDialogType
{
	CELL_MSGDIALOG_DIALOG_TYPE_ERROR  = 0x00000000,
	CELL_MSGDIALOG_DIALOG_TYPE_NORMAL = 0x00000001,
	CELL_MSGDIALOG_BUTTON_TYPE_NONE   = 0x00000000,
	CELL_MSGDIALOG_BUTTON_TYPE_YESNO  = 0x00000010,
	CELL_MSGDIALOG_DEFAULT_CURSOR_YES = 0x00000000,
	CELL_MSGDIALOG_DEFAULT_CURSOR_NO  = 0x00000100,
};

enum
{
	CELL_MSGDIALOG_TYPE_SE_TYPE        = 0x1,
	CELL_MSGDIALOG_TYPE_SE_TYPE_ERROR  = 0 << 0,
	CELL_MSGDIALOG_TYPE_SE_TYPE_NORMAL = 1 << 0,
};

enum
{
	CELL_MSGDIALOG_TYPE_SE_MUTE     = 0x2,
	CELL_MSGDIALOG_TYPE_SE_MUTE_OFF = 0 << 1,
	CELL_MSGDIALOG_TYPE_SE_MUTE_ON  = 1 << 1,
};

enum
{
	CELL_MSGDIALOG_TYPE_BG           = 0x4,
	CELL_MSGDIALOG_TYPE_BG_VISIBLE   = 0 << 2,
	CELL_MSGDIALOG_TYPE_BG_INVISIBLE = 1 << 2,
};

enum
{
	CELL_MSGDIALOG_TYPE_BUTTON_TYPE       = 0x70,
	CELL_MSGDIALOG_TYPE_BUTTON_TYPE_NONE  = 0 << 4,
	CELL_MSGDIALOG_TYPE_BUTTON_TYPE_YESNO = 1 << 4,
	CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK    = 2 << 4,
};

enum
{
	CELL_MSGDIALOG_TYPE_DISABLE_CANCEL     = 0x80,
	CELL_MSGDIALOG_TYPE_DISABLE_CANCEL_OFF = 0 << 7,
	CELL_MSGDIALOG_TYPE_DISABLE_CANCEL_ON  = 1 << 7,
};

enum
{
	CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR      = 0x300,
	CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR_NONE = 0 << 8,
	CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR_YES  = 0 << 8,
	CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR_NO   = 1 << 8,
	CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR_OK   = 0 << 8,
};

enum
{
	CELL_MSGDIALOG_TYPE_PROGRESSBAR        = 0x3000,
	CELL_MSGDIALOG_TYPE_PROGRESSBAR_NONE   = 0 << 12,
	CELL_MSGDIALOG_TYPE_PROGRESSBAR_SINGLE = 1 << 12,
	CELL_MSGDIALOG_TYPE_PROGRESSBAR_DOUBLE = 2 << 12,
};

enum
{
	CELL_MSGDIALOG_BUTTON_NONE    = -1,
	CELL_MSGDIALOG_BUTTON_INVALID = 0,
	CELL_MSGDIALOG_BUTTON_OK      = 1,
	CELL_MSGDIALOG_BUTTON_YES     = 1,
	CELL_MSGDIALOG_BUTTON_NO      = 2,
	CELL_MSGDIALOG_BUTTON_ESCAPE  = 3,
};

typedef void(CellMsgDialogCallback)(s32 buttonType, u32 userData);

s32 cellMsgDialogOpen2(u32 type, vm::ptr<const char> msgString, vm::ptr<CellMsgDialogCallback> callback, u32 userData, u32 extParam);
s32 cellMsgDialogOpenErrorCode(u32 errorCode, vm::ptr<CellMsgDialogCallback> callback, u32 userData, u32 extParam);

s32 cellMsgDialogProgressBarSetMsg(u32 progressBarIndex, vm::ptr<const char> msgString);
s32 cellMsgDialogProgressBarReset(u32 progressBarIndex);
s32 cellMsgDialogProgressBarInc(u32 progressBarIndex, u32 delta);
s32 cellMsgDialogClose(float delay);
s32 cellMsgDialogAbort();

typedef void(*MsgDialogCreateCb)(u32 type, const char* msg, u64& status);
typedef void(*MsgDialogDestroyCb)();
typedef void(*MsgDialogProgressBarSetMsgCb)(u32 progressBarIndex, const char* msg);
typedef void(*MsgDialogProgressBarResetCb)(u32 progressBarIndex);
typedef void(*MsgDialogProgressBarIncCb)(u32 progressBarIndex, u32 delta);

void SetMsgDialogCallbacks(MsgDialogCreateCb ccb, MsgDialogDestroyCb dcb, MsgDialogProgressBarSetMsgCb pbscb, MsgDialogProgressBarResetCb pbrcb, MsgDialogProgressBarIncCb pbicb);
void MsgDialogClose();
