#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/Callback.h"
#include "Emu/SysCalls/CB_FUNC.h"

#include "Emu/SysCalls/lv2/sys_time.h"
#include "cellSysutil.h"
#include "cellMsgDialog.h"

extern Module cellSysutil;

std::unique_ptr<MsgDialogInstance> g_msg_dialog;

MsgDialogInstance::MsgDialogInstance()
	: state(msgDialogNone)
{
}

void MsgDialogInstance::Close()
{
	state = msgDialogClose;
	wait_until = get_system_time();
}

s32 cellMsgDialogOpen2(u32 type, vm::cptr<char> msgString, vm::ptr<CellMsgDialogCallback> callback, vm::ptr<void> userData, vm::ptr<void> extParam)
{
	cellSysutil.Warning("cellMsgDialogOpen2(type=0x%x, msgString=*0x%x, callback=*0x%x, userData=*0x%x, extParam=*0x%x)", type, msgString, callback, userData, extParam);

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
	if (!g_msg_dialog->state.compare_exchange_strong(old, msgDialogOpen))
	{
		return CELL_SYSUTIL_ERROR_BUSY;
	}

	g_msg_dialog->wait_until = get_system_time() + 31536000000000ull; // some big value

	switch (type & CELL_MSGDIALOG_TYPE_PROGRESSBAR)
	{
	case CELL_MSGDIALOG_TYPE_PROGRESSBAR_DOUBLE: g_msg_dialog->progress_bar_count = 2; break;
	case CELL_MSGDIALOG_TYPE_PROGRESSBAR_SINGLE: g_msg_dialog->progress_bar_count = 1; break;
	default: g_msg_dialog->progress_bar_count = 0; break;
	}

	switch (type & CELL_MSGDIALOG_TYPE_SE_MUTE) // TODO
	{
	case CELL_MSGDIALOG_TYPE_SE_MUTE_OFF: break;
	case CELL_MSGDIALOG_TYPE_SE_MUTE_ON: break;
	}

	std::string msg = msgString.get_ptr();

	thread_t t("MsgDialog Thread", [type, msg, callback, userData, extParam]()
	{
		switch (type & CELL_MSGDIALOG_TYPE_SE_TYPE)
		{
		case CELL_MSGDIALOG_TYPE_SE_TYPE_NORMAL: cellSysutil.Warning("%s", msg); break;
		case CELL_MSGDIALOG_TYPE_SE_TYPE_ERROR: cellSysutil.Error("%s", msg); break;
		}

		g_msg_dialog->status = CELL_MSGDIALOG_BUTTON_NONE;

		volatile bool m_signal = false;
		CallAfter([type, msg, &m_signal]()
		{
			if (Emu.IsStopped()) return;

			g_msg_dialog->Create(type, msg);

			m_signal = true;
		});

		while (!m_signal)
		{
			if (Emu.IsStopped())
			{
				cellSysutil.Warning("MsgDialog thread aborted");
				return;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
		}

		while (g_msg_dialog->state == msgDialogOpen || (s64)(get_system_time() - g_msg_dialog->wait_until) < 0)
		{
			if (Emu.IsStopped())
			{
				g_msg_dialog->state = msgDialogAbort;
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
		}

		if (callback && (g_msg_dialog->state != msgDialogAbort))
		{
			const s32 status = g_msg_dialog->status;

			Emu.GetCallbackManager().Register([=](PPUThread& PPU) -> s32
			{
				callback(PPU, status, userData);
				return CELL_OK;
			});
		}

		CallAfter([]()
		{
			g_msg_dialog->Destroy();
			g_msg_dialog->state = msgDialogNone;
		});
	});

	return CELL_OK;
}

s32 cellMsgDialogOpenErrorCode(PPUThread& CPU, u32 errorCode, vm::ptr<CellMsgDialogCallback> callback, vm::ptr<void> userData, vm::ptr<void> extParam)
{
	cellSysutil.Warning("cellMsgDialogOpenErrorCode(errorCode=0x%x, callback=*0x%x, userData=*0x%x, extParam=*0x%x)", errorCode, callback, userData, extParam);

	std::string error;

	switch (errorCode)
	{
	case 0x80010001: error = "The resource is temporarily unavailable."; break;
	case 0x80010002: error = "Invalid argument or flag."; break;
	case 0x80010003: error = "The feature is not yet implemented."; break;
	case 0x80010004: error = "Memory allocation failed."; break;
	case 0x80010005: error = "The resource with the specified identifier does not exist."; break;
	case 0x80010006: error = "The file does not exist."; break;
	case 0x80010007: error = "The file is in unrecognized format / The file is not a valid ELF file."; break;
	case 0x80010008: error = "Resource deadlock is avoided."; break;
	case 0x80010009: error = "Operation not permitted."; break;
	case 0x8001000A: error = "The device or resource is busy."; break;
	case 0x8001000B: error = "The operation is timed out."; break;
	case 0x8001000C: error = "The operation is aborted."; break;
	case 0x8001000D: error = "Invalid memory access."; break;
	case 0x8001000F: error = "State of the target thread is invalid."; break;
	case 0x80010010: error = "Alignment is invalid."; break;
	case 0x80010011: error = "Shortage of the kernel resources."; break;
	case 0x80010012: error = "The file is a directory."; break;
	case 0x80010013: error = "Operation cancelled."; break;
	case 0x80010014: error = "Entry already exists."; break;
	case 0x80010015: error = "Port is already connected."; break;
	case 0x80010016: error = "Port is not connected."; break;
	case 0x80010017: error = "Failure in authorizing SELF. Program authentication fail."; break;
	case 0x80010018: error = "The file is not MSELF."; break;
	case 0x80010019: error = "System version error."; break;
	case 0x8001001A: error = "Fatal system error occurred while authorizing SELF. SELF auth failure."; break;
	case 0x8001001B: error = "Math domain violation."; break;
	case 0x8001001C: error = "Math range violation."; break;
	case 0x8001001D: error = "Illegal multi-byte sequence in input."; break;
	case 0x8001001E: error = "File position error."; break;
	case 0x8001001F: error = "Syscall was interrupted."; break;
	case 0x80010020: error = "File too large."; break;
	case 0x80010021: error = "Too many links."; break;
	case 0x80010022: error = "File table overflow."; break;
	case 0x80010023: error = "No space left on device."; break;
	case 0x80010024: error = "Not a TTY."; break;
	case 0x80010025: error = "Broken pipe."; break;
	case 0x80010026: error = "Read-only filesystem."; break;
	case 0x80010027: error = "Illegal seek."; break;
	case 0x80010028: error = "Arg list too long."; break;
	case 0x80010029: error = "Access violation."; break;
	case 0x8001002A: error = "Invalid file descriptor."; break;
	case 0x8001002B: error = "Filesystem mounting failed."; break;
	case 0x8001002C: error = "Too many files open."; break;
	case 0x8001002D: error = "No device."; break;
	case 0x8001002E: error = "Not a directory."; break;
	case 0x8001002F: error = "No such device or IO."; break;
	case 0x80010030: error = "Cross-device link error."; break;
	case 0x80010031: error = "Bad Message."; break;
	case 0x80010032: error = "In progress."; break;
	case 0x80010033: error = "Message size error."; break;
	case 0x80010034: error = "Name too long."; break;
	case 0x80010035: error = "No lock."; break;
	case 0x80010036: error = "Not empty."; break;
	case 0x80010037: error = "Not supported."; break;
	case 0x80010038: error = "File-system specific error."; break;
	case 0x80010039: error = "Overflow occured."; break;
	case 0x8001003A: error = "Filesystem not mounted."; break;
	case 0x8001003B: error = "Not SData."; break;
	case 0x8001003C: error = "Incorrect version in sys_load_param."; break;
	case 0x8001003D: error = "Pointer is null."; break;
	case 0x8001003E: error = "Pointer is null."; break;
	default: error = "An error has occurred."; break;
	}

	error.append(fmt::format("\n(%08x)", errorCode));

	vm::stackvar<char> message(CPU, error.size() + 1);

	memcpy(message.get_ptr(), error.c_str(), message.size());

	return cellMsgDialogOpen2(CELL_MSGDIALOG_DIALOG_TYPE_ERROR | CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK, message, callback, userData, extParam);
}

s32 cellMsgDialogClose(float delay)
{
	cellSysutil.Warning("cellMsgDialogClose(delay=%f)", delay);

	MsgDialogState old = msgDialogOpen;

	if (!g_msg_dialog->state.compare_exchange_strong(old, msgDialogClose))
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

	g_msg_dialog->wait_until = get_system_time() + static_cast<u64>(std::max<float>(delay, 0.0f) * 1000);

	return CELL_OK;
}

s32 cellMsgDialogAbort()
{
	cellSysutil.Warning("cellMsgDialogAbort()");

	MsgDialogState old = msgDialogOpen;

	if (!g_msg_dialog->state.compare_exchange_strong(old, msgDialogAbort))
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

	g_msg_dialog->wait_until = get_system_time();

	return CELL_OK;
}

s32 cellMsgDialogProgressBarSetMsg(u32 progressBarIndex, vm::cptr<char> msgString)
{
	cellSysutil.Warning("cellMsgDialogProgressBarSetMsg(progressBarIndex=%d, msgString=*0x%x)", progressBarIndex, msgString);

	if (g_msg_dialog->state != msgDialogOpen)
	{
		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	if (progressBarIndex >= g_msg_dialog->progress_bar_count)
	{
		return CELL_MSGDIALOG_ERROR_PARAM;
	}

	g_msg_dialog->ProgressBarSetMsg(progressBarIndex, msgString.get_ptr());

	return CELL_OK;
}

s32 cellMsgDialogProgressBarReset(u32 progressBarIndex)
{
	cellSysutil.Warning("cellMsgDialogProgressBarReset(progressBarIndex=%d)", progressBarIndex);

	if (g_msg_dialog->state != msgDialogOpen)
	{
		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	if (progressBarIndex >= g_msg_dialog->progress_bar_count)
	{
		return CELL_MSGDIALOG_ERROR_PARAM;
	}

	g_msg_dialog->ProgressBarReset(progressBarIndex);

	return CELL_OK;
}

s32 cellMsgDialogProgressBarInc(u32 progressBarIndex, u32 delta)
{
	cellSysutil.Warning("cellMsgDialogProgressBarInc(progressBarIndex=%d, delta=%d)", progressBarIndex, delta);

	if (g_msg_dialog->state != msgDialogOpen)
	{
		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	if (progressBarIndex >= g_msg_dialog->progress_bar_count)
	{
		return CELL_MSGDIALOG_ERROR_PARAM;
	}

	g_msg_dialog->ProgressBarInc(progressBarIndex, delta);

	return CELL_OK;
}

void cellSysutil_MsgDialog_init()
{
	REG_FUNC(cellSysutil, cellMsgDialogOpen2);
	REG_FUNC(cellSysutil, cellMsgDialogOpenErrorCode);
	REG_FUNC(cellSysutil, cellMsgDialogProgressBarSetMsg);
	REG_FUNC(cellSysutil, cellMsgDialogProgressBarReset);
	REG_FUNC(cellSysutil, cellMsgDialogProgressBarInc);
	REG_FUNC(cellSysutil, cellMsgDialogClose);
	REG_FUNC(cellSysutil, cellMsgDialogAbort);
}
