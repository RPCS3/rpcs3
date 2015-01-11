#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/Callback.h"
#include "Emu/SysCalls/CB_FUNC.h"

#include "Utilities/Log.h"
#include "Utilities/rMsgBox.h"
#include "Emu/SysCalls/lv2/sys_time.h"
#include "cellSysutil.h"
#include "cellMsgDialog.h"

extern Module *cellSysutil;

enum MsgDialogState
{
	msgDialogNone,
	msgDialogOpen,
	msgDialogClose,
	msgDialogAbort,
};

std::atomic<MsgDialogState> g_msg_dialog_state(msgDialogNone);
u64 g_msg_dialog_status;
u64 g_msg_dialog_wait_until;
u32 g_msg_dialog_progress_bar_count;

MsgDialogCreateCb MsgDialogCreate = nullptr;
MsgDialogDestroyCb MsgDialogDestroy = nullptr;
MsgDialogProgressBarSetMsgCb MsgDialogProgressBarSetMsg = nullptr;
MsgDialogProgressBarResetCb MsgDialogProgressBarReset = nullptr;
MsgDialogProgressBarIncCb MsgDialogProgressBarInc = nullptr;

void SetMsgDialogCallbacks(MsgDialogCreateCb ccb, MsgDialogDestroyCb dcb, MsgDialogProgressBarSetMsgCb pbscb, MsgDialogProgressBarResetCb pbrcb, MsgDialogProgressBarIncCb pbicb)
{
	MsgDialogCreate = ccb;
	MsgDialogDestroy = dcb;
	MsgDialogProgressBarSetMsg = pbscb;
	MsgDialogProgressBarReset = pbrcb;
	MsgDialogProgressBarInc = pbicb;
}

void MsgDialogClose()
{
	g_msg_dialog_state = msgDialogClose;
	g_msg_dialog_wait_until = get_system_time();
}

s32 cellMsgDialogOpen2(u32 type, vm::ptr<const char> msgString, vm::ptr<CellMsgDialogCallback> callback, u32 userData, u32 extParam)
{
	cellSysutil->Warning("cellMsgDialogOpen2(type=0x%x, msgString_addr=0x%x, callback_addr=0x%x, userData=0x%x, extParam=0x%x)",
		type, msgString.addr(), callback.addr(), userData, extParam);

	if (!msgString || strlen(msgString.get_ptr()) >= 0x200 || type & -0x33f8)
	{
		return CELL_MSGDIALOG_ERROR_PARAM;
	}

	switch (type & CELL_MSGDIALOG_TYPE_BUTTON_TYPE)
	{
	case CELL_MSGDIALOG_TYPE_BUTTON_TYPE_NONE:
	{
		if (type & CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR)
		{
			return CELL_MSGDIALOG_ERROR_PARAM;
		}
		switch (type & CELL_MSGDIALOG_TYPE_PROGRESSBAR)
		{
		case CELL_MSGDIALOG_TYPE_PROGRESSBAR_NONE: break;
		case CELL_MSGDIALOG_TYPE_PROGRESSBAR_SINGLE: break;
		case CELL_MSGDIALOG_TYPE_PROGRESSBAR_DOUBLE: break;
		default: return CELL_MSGDIALOG_ERROR_PARAM;
		}
		break;
	}

	case CELL_MSGDIALOG_TYPE_BUTTON_TYPE_YESNO:
	{
		switch (type & CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR)
		{
		case CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR_YES: break;
		case CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR_NO: break;
		default: return CELL_MSGDIALOG_ERROR_PARAM;
		}
		if (type & CELL_MSGDIALOG_TYPE_PROGRESSBAR)
		{
			return CELL_MSGDIALOG_ERROR_PARAM;
		}
		break;
	}

	case CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK:
	{
		if (type & CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR)
		{
			return CELL_MSGDIALOG_ERROR_PARAM;
		}
		if (type & CELL_MSGDIALOG_TYPE_PROGRESSBAR)
		{
			return CELL_MSGDIALOG_ERROR_PARAM;
		}
		break;
	}

	default: return CELL_MSGDIALOG_ERROR_PARAM;
	}

	MsgDialogState old = msgDialogNone;
	if (!g_msg_dialog_state.compare_exchange_strong(old, msgDialogOpen))
	{
		return CELL_SYSUTIL_ERROR_BUSY;
	}

	g_msg_dialog_wait_until = get_system_time() + 31536000000000ull; // some big value

	switch (type & CELL_MSGDIALOG_TYPE_PROGRESSBAR)
	{
	case CELL_MSGDIALOG_TYPE_PROGRESSBAR_DOUBLE: g_msg_dialog_progress_bar_count = 2; break;
	case CELL_MSGDIALOG_TYPE_PROGRESSBAR_SINGLE: g_msg_dialog_progress_bar_count = 1; break;
	default: g_msg_dialog_progress_bar_count = 0; break;
	}

	std::string msg = msgString.get_ptr();

	thread t("MsgDialog thread", [type, msg, callback, userData, extParam]()
	{
		switch (type & CELL_MSGDIALOG_TYPE_SE_TYPE)
		{
		case CELL_MSGDIALOG_TYPE_SE_TYPE_NORMAL: LOG_WARNING(TTY, "\n%s", msg.c_str()); break;
		case CELL_MSGDIALOG_TYPE_SE_TYPE_ERROR: LOG_ERROR(TTY, "\n%s", msg.c_str()); break;
		}

		switch (type & CELL_MSGDIALOG_TYPE_SE_MUTE) // TODO
		{
		case CELL_MSGDIALOG_TYPE_SE_MUTE_OFF: break;
		case CELL_MSGDIALOG_TYPE_SE_MUTE_ON: break;
		}

		g_msg_dialog_status = CELL_MSGDIALOG_BUTTON_NONE;

		volatile bool m_signal = false;
		CallAfter([type, msg, &m_signal]()
		{
			if (Emu.IsStopped()) return;

			MsgDialogCreate(type, msg.c_str(), g_msg_dialog_status);

			m_signal = true;
		});

		while (!m_signal)
		{
			if (Emu.IsStopped())
			{
				cellSysutil->Warning("MsgDialog thread aborted");
				return;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
		}

		while (g_msg_dialog_state == msgDialogOpen || (s64)(get_system_time() - g_msg_dialog_wait_until) < 0)
		{
			if (Emu.IsStopped())
			{
				g_msg_dialog_state = msgDialogAbort;
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
		}

		if (callback && (g_msg_dialog_state != msgDialogAbort))
		{
			const s32 status = (s32)g_msg_dialog_status;
			Emu.GetCallbackManager().Register([callback, userData, status](PPUThread& PPU) -> s32
			{
				callback(PPU, status, userData);
				return CELL_OK;
			});
		}

		CallAfter([]()
		{
			MsgDialogDestroy();

			g_msg_dialog_state = msgDialogNone;
		});
	});
	t.detach();

	return CELL_OK;
}

s32 cellMsgDialogOpenErrorCode(u32 errorCode, vm::ptr<CellMsgDialogCallback> callback, u32 userData, u32 extParam)
{
	cellSysutil->Warning("cellMsgDialogOpenErrorCode(errorCode=0x%x, callback_addr=0x%x, userData=0x%x, extParam=%d)",
		errorCode, callback.addr(), userData, extParam);

	std::string errorMessage;
	switch (errorCode)
	{
		// Generic errors
	case 0x80010001: errorMessage = "The resource is temporarily unavailable."; break;
	case 0x80010002: errorMessage = "Invalid argument or flag."; break;
	case 0x80010003: errorMessage = "The feature is not yet implemented."; break;
	case 0x80010004: errorMessage = "Memory allocation failed."; break;
	case 0x80010005: errorMessage = "The resource with the specified identifier does not exist."; break;
	case 0x80010006: errorMessage = "The file does not exist."; break;
	case 0x80010007: errorMessage = "The file is in unrecognized format / The file is not a valid ELF file."; break;
	case 0x80010008: errorMessage = "Resource deadlock is avoided."; break;
	case 0x80010009: errorMessage = "Operation not permitted."; break;
	case 0x8001000A: errorMessage = "The device or resource is busy."; break;
	case 0x8001000B: errorMessage = "The operation is timed out."; break;
	case 0x8001000C: errorMessage = "The operation is aborted."; break;
	case 0x8001000D: errorMessage = "Invalid memory access."; break;
	case 0x8001000F: errorMessage = "State of the target thread is invalid."; break;
	case 0x80010010: errorMessage = "Alignment is invalid."; break;
	case 0x80010011: errorMessage = "Shortage of the kernel resources."; break;
	case 0x80010012: errorMessage = "The file is a directory."; break;
	case 0x80010013: errorMessage = "Operation cancelled."; break;
	case 0x80010014: errorMessage = "Entry already exists."; break;
	case 0x80010015: errorMessage = "Port is already connected."; break;
	case 0x80010016: errorMessage = "Port is not connected."; break;
	case 0x80010017: errorMessage = "Failure in authorizing SELF. Program authentication fail."; break;
	case 0x80010018: errorMessage = "The file is not MSELF."; break;
	case 0x80010019: errorMessage = "System version error."; break;
	case 0x8001001A: errorMessage = "Fatal system error occurred while authorizing SELF. SELF auth failure."; break;
	case 0x8001001B: errorMessage = "Math domain violation."; break;
	case 0x8001001C: errorMessage = "Math range violation."; break;
	case 0x8001001D: errorMessage = "Illegal multi-byte sequence in input."; break;
	case 0x8001001E: errorMessage = "File position error."; break;
	case 0x8001001F: errorMessage = "Syscall was interrupted."; break;
	case 0x80010020: errorMessage = "File too large."; break;
	case 0x80010021: errorMessage = "Too many links."; break;
	case 0x80010022: errorMessage = "File table overflow."; break;
	case 0x80010023: errorMessage = "No space left on device."; break;
	case 0x80010024: errorMessage = "Not a TTY."; break;
	case 0x80010025: errorMessage = "Broken pipe."; break;
	case 0x80010026: errorMessage = "Read-only filesystem."; break;
	case 0x80010027: errorMessage = "Illegal seek."; break;
	case 0x80010028: errorMessage = "Arg list too long."; break;
	case 0x80010029: errorMessage = "Access violation."; break;
	case 0x8001002A: errorMessage = "Invalid file descriptor."; break;
	case 0x8001002B: errorMessage = "Filesystem mounting failed."; break;
	case 0x8001002C: errorMessage = "Too many files open."; break;
	case 0x8001002D: errorMessage = "No device."; break;
	case 0x8001002E: errorMessage = "Not a directory."; break;
	case 0x8001002F: errorMessage = "No such device or IO."; break;
	case 0x80010030: errorMessage = "Cross-device link error."; break;
	case 0x80010031: errorMessage = "Bad Message."; break;
	case 0x80010032: errorMessage = "In progress."; break;
	case 0x80010033: errorMessage = "Message size error."; break;
	case 0x80010034: errorMessage = "Name too long."; break;
	case 0x80010035: errorMessage = "No lock."; break;
	case 0x80010036: errorMessage = "Not empty."; break;
	case 0x80010037: errorMessage = "Not supported."; break;
	case 0x80010038: errorMessage = "File-system specific error."; break;
	case 0x80010039: errorMessage = "Overflow occured."; break;
	case 0x8001003A: errorMessage = "Filesystem not mounted."; break;
	case 0x8001003B: errorMessage = "Not SData."; break;
	case 0x8001003C: errorMessage = "Incorrect version in sys_load_param."; break;
	case 0x8001003D: errorMessage = "Pointer is null."; break;
	case 0x8001003E: errorMessage = "Pointer is null."; break;
	default: errorMessage = "An error has occurred."; break;
	}

	char errorCodeHex[12];
	sprintf(errorCodeHex, "\n(%08x)", errorCode);
	errorMessage.append(errorCodeHex);

	u64 status;
	int res = rMessageBox(errorMessage, "Error", rICON_ERROR | rOK);
	switch (res)
	{
	case rOK: status = CELL_MSGDIALOG_BUTTON_OK; break;
	default:
		if (res)
		{
			status = CELL_MSGDIALOG_BUTTON_INVALID;
			break;
		}

		status = CELL_MSGDIALOG_BUTTON_NONE;
		break;
	}

	if (callback)
		callback((s32)status, userData);

	return CELL_OK;
}

s32 cellMsgDialogClose(float delay)
{
	cellSysutil->Warning("cellMsgDialogClose(delay=%f)", delay);

	MsgDialogState old = msgDialogOpen;
	if (!g_msg_dialog_state.compare_exchange_strong(old, msgDialogClose))
	{
		if (old == msgDialogNone)
		{
			return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
		}
		else
		{
			return CELL_SYSUTIL_ERROR_BUSY;
		}
	}

	if (delay < 0.0f) delay = 0.0f;
	g_msg_dialog_wait_until = get_system_time() + (u64)(delay * 1000);
	return CELL_OK;
}

s32 cellMsgDialogAbort()
{
	cellSysutil->Warning("cellMsgDialogAbort()");

	MsgDialogState old = msgDialogOpen;
	if (!g_msg_dialog_state.compare_exchange_strong(old, msgDialogAbort))
	{
		if (old == msgDialogNone)
		{
			return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
		}
		else
		{
			return CELL_SYSUTIL_ERROR_BUSY;
		}
	}

	g_msg_dialog_wait_until = get_system_time();
	return CELL_OK;
}

s32 cellMsgDialogProgressBarSetMsg(u32 progressBarIndex, vm::ptr<const char> msgString)
{
	cellSysutil->Warning("cellMsgDialogProgressBarSetMsg(progressBarIndex=%d, msgString_addr=0x%x ['%s'])",
		progressBarIndex, msgString.addr(), msgString.get_ptr());

	if (g_msg_dialog_state != msgDialogOpen)
	{
		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	if (progressBarIndex >= g_msg_dialog_progress_bar_count)
	{
		return CELL_MSGDIALOG_ERROR_PARAM;
	}

	std::string text = msgString.get_ptr();

	CallAfter([text, progressBarIndex]()
	{
		MsgDialogProgressBarSetMsg(progressBarIndex, text.c_str());
	});
	return CELL_OK;
}

s32 cellMsgDialogProgressBarReset(u32 progressBarIndex)
{
	cellSysutil->Warning("cellMsgDialogProgressBarReset(progressBarIndex=%d)", progressBarIndex);

	if (g_msg_dialog_state != msgDialogOpen)
	{
		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	if (progressBarIndex >= g_msg_dialog_progress_bar_count)
	{
		return CELL_MSGDIALOG_ERROR_PARAM;
	}

	CallAfter([=]()
	{
		MsgDialogProgressBarReset(progressBarIndex);
	});
	return CELL_OK;
}

s32 cellMsgDialogProgressBarInc(u32 progressBarIndex, u32 delta)
{
	cellSysutil->Warning("cellMsgDialogProgressBarInc(progressBarIndex=%d, delta=%d)", progressBarIndex, delta);

	if (g_msg_dialog_state != msgDialogOpen)
	{
		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	if (progressBarIndex >= g_msg_dialog_progress_bar_count)
	{
		return CELL_MSGDIALOG_ERROR_PARAM;
	}

	CallAfter([=]()
	{
		MsgDialogProgressBarInc(progressBarIndex, delta);
	});
	return CELL_OK;
}
