#pragma once

namespace vm { using namespace ps3; }

enum
{
	CELL_MSGDIALOG_ERROR_PARAM             = 0x8002b301,
	CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED = 0x8002b302,
};

enum CellMsgDialogType : u32
{
	CELL_MSGDIALOG_DIALOG_TYPE_ERROR  = 0x00000000,
	CELL_MSGDIALOG_DIALOG_TYPE_NORMAL = 0x00000001,
	CELL_MSGDIALOG_BUTTON_TYPE_NONE   = 0x00000000,
	CELL_MSGDIALOG_BUTTON_TYPE_YESNO  = 0x00000010,
	CELL_MSGDIALOG_DEFAULT_CURSOR_YES = 0x00000000,
	CELL_MSGDIALOG_DEFAULT_CURSOR_NO  = 0x00000100,
};

enum : u32
{
	CELL_MSGDIALOG_TYPE_SE_TYPE        = 0x1,
	CELL_MSGDIALOG_TYPE_SE_TYPE_ERROR  = 0 << 0,
	CELL_MSGDIALOG_TYPE_SE_TYPE_NORMAL = 1 << 0,
};

enum : u32
{
	CELL_MSGDIALOG_TYPE_SE_MUTE     = 0x2,
	CELL_MSGDIALOG_TYPE_SE_MUTE_OFF = 0 << 1,
	CELL_MSGDIALOG_TYPE_SE_MUTE_ON  = 1 << 1,
};

enum : u32
{
	CELL_MSGDIALOG_TYPE_BG           = 0x4,
	CELL_MSGDIALOG_TYPE_BG_VISIBLE   = 0 << 2,
	CELL_MSGDIALOG_TYPE_BG_INVISIBLE = 1 << 2,
};

enum : u32
{
	CELL_MSGDIALOG_TYPE_BUTTON_TYPE       = 0x70,
	CELL_MSGDIALOG_TYPE_BUTTON_TYPE_NONE  = 0 << 4,
	CELL_MSGDIALOG_TYPE_BUTTON_TYPE_YESNO = 1 << 4,
	CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK    = 2 << 4,
};

enum : u32
{
	CELL_MSGDIALOG_TYPE_DISABLE_CANCEL     = 0x80,
	CELL_MSGDIALOG_TYPE_DISABLE_CANCEL_OFF = 0 << 7,
	CELL_MSGDIALOG_TYPE_DISABLE_CANCEL_ON  = 1 << 7,
};

enum : u32
{
	CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR      = 0x300,
	CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR_NONE = 0 << 8,
	CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR_YES  = 0 << 8,
	CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR_NO   = 1 << 8,
	CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR_OK   = 0 << 8,
};

enum : u32
{
	CELL_MSGDIALOG_TYPE_PROGRESSBAR        = 0x3000,
	CELL_MSGDIALOG_TYPE_PROGRESSBAR_NONE   = 0 << 12,
	CELL_MSGDIALOG_TYPE_PROGRESSBAR_SINGLE = 1 << 12,
	CELL_MSGDIALOG_TYPE_PROGRESSBAR_DOUBLE = 2 << 12,
};

// MsgDialog Button Type
enum : s32
{
	CELL_MSGDIALOG_BUTTON_NONE    = -1,
	CELL_MSGDIALOG_BUTTON_INVALID = 0,
	CELL_MSGDIALOG_BUTTON_OK      = 1,
	CELL_MSGDIALOG_BUTTON_YES     = 1,
	CELL_MSGDIALOG_BUTTON_NO      = 2,
	CELL_MSGDIALOG_BUTTON_ESCAPE  = 3,
};

using CellMsgDialogCallback = void(s32 buttonType, vm::ptr<void> userData);

enum MsgDialogState
{
	msgDialogNone,
	msgDialogInit,
	msgDialogOpen,
	msgDialogClose,
	msgDialogAbort,
};

struct MsgDialogInstance
{
	std::atomic<MsgDialogState> state;

	s32 status;
	u64 wait_until;
	u32 progress_bar_count;

	MsgDialogInstance();
	virtual ~MsgDialogInstance() = default;

	virtual void Close();

	virtual void Create(u32 type, std::string msg) = 0;
	virtual void Destroy() = 0;
	virtual void ProgressBarSetMsg(u32 progressBarIndex, std::string msg) = 0;
	virtual void ProgressBarReset(u32 progressBarIndex) = 0;
	virtual void ProgressBarInc(u32 progressBarIndex, u32 delta) = 0;
};
