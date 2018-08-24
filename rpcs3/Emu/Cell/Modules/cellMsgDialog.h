#pragma once

#include "Utilities/BitField.h"



enum
{
	CELL_MSGDIALOG_ERROR_PARAM             = 0x8002b301,
	CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED = 0x8002b302,
};

enum : u32
{
	CELL_MSGDIALOG_TYPE_SE_TYPE_ERROR       = 0 << 0,
	CELL_MSGDIALOG_TYPE_SE_TYPE_NORMAL      = 1 << 0,

	CELL_MSGDIALOG_TYPE_SE_MUTE_OFF         = 0 << 1,
	CELL_MSGDIALOG_TYPE_SE_MUTE_ON          = 1 << 1,

	CELL_MSGDIALOG_TYPE_BG_VISIBLE          = 0 << 2,
	CELL_MSGDIALOG_TYPE_BG_INVISIBLE        = 1 << 2,

	CELL_MSGDIALOG_TYPE_BUTTON_TYPE_NONE    = 0 << 4,
	CELL_MSGDIALOG_TYPE_BUTTON_TYPE_YESNO   = 1 << 4,
	CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK      = 2 << 4,

	CELL_MSGDIALOG_TYPE_DISABLE_CANCEL_OFF  = 0 << 7,
	CELL_MSGDIALOG_TYPE_DISABLE_CANCEL_ON   = 1 << 7,

	CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR_NONE = 0 << 8,
	CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR_YES  = 0 << 8,
	CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR_NO   = 1 << 8,
	CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR_OK   = 0 << 8,

	CELL_MSGDIALOG_TYPE_PROGRESSBAR_NONE    = 0 << 12,
	CELL_MSGDIALOG_TYPE_PROGRESSBAR_SINGLE  = 1 << 12,
	CELL_MSGDIALOG_TYPE_PROGRESSBAR_DOUBLE  = 2 << 12,
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

union MsgDialogType
{
	u32 value;

	bf_t<u32, 0, 1> se_normal;
	bf_t<u32, 1, 1> se_mute_on;
	bf_t<u32, 2, 1> bg_invisible;
	bf_t<u32, 4, 3> button_type;
	bf_t<u32, 7, 1> disable_cancel;
	bf_t<u32, 8, 2> default_cursor;
	bf_t<u32, 12, 2> progress_bar_count;
};

enum class MsgDialogState
{
	Open,
	Abort,
	Close,
};

class MsgDialogBase
{
protected:
	// the progressbar that will be represented in the taskbar. Use -1 to combine the progress.
	s32 taskbar_index = 0;

public:
	atomic_t<MsgDialogState> state{ MsgDialogState::Open };

	MsgDialogType type{};

	std::function<void(s32 status)> on_close;
	std::function<void()> on_osk_input_entered;

	virtual ~MsgDialogBase();
	virtual void Create(const std::string& msg, const std::string& title = "") = 0;
	virtual void CreateOsk(const std::string& msg, char16_t* osk_text, u32 charlimit) = 0;
	virtual void SetMsg(const std::string& msg) = 0;
	virtual void ProgressBarSetMsg(u32 progressBarIndex, const std::string& msg) = 0;
	virtual void ProgressBarReset(u32 progressBarIndex) = 0;
	virtual void ProgressBarInc(u32 progressBarIndex, u32 delta) = 0;
	virtual void ProgressBarSetLimit(u32 index, u32 limit) = 0;

	void ProgressBarSetTaskbarIndex(s32 index)
	{
		taskbar_index = index;
	}
};
