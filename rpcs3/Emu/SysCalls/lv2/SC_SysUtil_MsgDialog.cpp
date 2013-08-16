#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"

SysCallBase sc_md("cellMsgDialog");

enum
{
	CELL_MSGDIALOG_BUTTON_NONE		= -1,
	CELL_MSGDIALOG_BUTTON_INVALID	= 0,
	CELL_MSGDIALOG_BUTTON_OK		= 1,
	CELL_MSGDIALOG_BUTTON_YES		= 1,
	CELL_MSGDIALOG_BUTTON_NO		= 2,
	CELL_MSGDIALOG_BUTTON_ESCAPE	= 3,
};

enum CellMsgDialogType
{
	CELL_MSGDIALOG_DIALOG_TYPE_ERROR	= 0x00000000,
	CELL_MSGDIALOG_DIALOG_TYPE_NORMAL	= 0x00000001,

	CELL_MSGDIALOG_BUTTON_TYPE_NONE		= 0x00000000,
	CELL_MSGDIALOG_BUTTON_TYPE_YESNO	= 0x00000010,

	CELL_MSGDIALOG_DEFAULT_CURSOR_YES	= 0x00000000,
	CELL_MSGDIALOG_DEFAULT_CURSOR_NO	= 0x00000100,
};

int cellMsgDialogOpen2(u32 type, u32 msgString_addr, u32 callback_addr, u32 userData, u32 extParam)
{
	long style = 0;

	if(type & CELL_MSGDIALOG_DIALOG_TYPE_NORMAL)
	{
		style |= wxICON_EXCLAMATION;
	}
	else
	{
		style |= wxICON_ERROR;
	}

	if(type & CELL_MSGDIALOG_BUTTON_TYPE_YESNO)
	{
		style |= wxYES_NO;
	}
	else
	{
		style |= wxOK;
	}

	int res = wxMessageBox(Memory.ReadString(msgString_addr), wxGetApp().GetAppName(), style);

	u64 status;

	switch(res)
	{
	case wxOK: status = CELL_MSGDIALOG_BUTTON_OK; break;
	case wxYES: status = CELL_MSGDIALOG_BUTTON_YES; break;
	case wxNO: status = CELL_MSGDIALOG_BUTTON_NO; break;

	default:
		if(res)
		{
			status = CELL_MSGDIALOG_BUTTON_INVALID;
			break;
		}

		status = CELL_MSGDIALOG_BUTTON_NONE;
	break;
	}

	Callback2 callback(0, callback_addr, userData);
	callback.Handle(status);
	callback.Branch(true);

	return CELL_OK;
}