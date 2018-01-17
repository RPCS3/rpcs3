#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/RSX/GSRender.h"

#include "cellSysutil.h"
#include "cellMsgDialog.h"

#include <thread>

extern logs::channel cellSysutil;

MsgDialogBase::~MsgDialogBase()
{
}

s32 cellMsgDialogOpen2(u32 type, vm::cptr<char> msgString, vm::ptr<CellMsgDialogCallback> callback, vm::ptr<void> userData, vm::ptr<void> extParam)
{
	cellSysutil.warning("cellMsgDialogOpen2(type=0x%x, msgString=%s, callback=*0x%x, userData=*0x%x, extParam=*0x%x)", type, msgString, callback, userData, extParam);

	if (!msgString || std::strlen(msgString.get_ptr()) >= 0x200 || type & -0x33f8)
	{
		return CELL_MSGDIALOG_ERROR_PARAM;
	}

	const MsgDialogType _type = {type ^ CELL_MSGDIALOG_TYPE_BG_INVISIBLE};

	switch (_type.button_type.unshifted())
	{
	case CELL_MSGDIALOG_TYPE_BUTTON_TYPE_NONE:
	{
		if (_type.default_cursor || _type.progress_bar_count > 2)
		{
			return CELL_MSGDIALOG_ERROR_PARAM;
		}

		break;
	}

	case CELL_MSGDIALOG_TYPE_BUTTON_TYPE_YESNO:
	{
		if (_type.default_cursor > 1 || _type.progress_bar_count)
		{
			return CELL_MSGDIALOG_ERROR_PARAM;
		}

		break;
	}

	case CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK:
	{
		if (_type.default_cursor || _type.progress_bar_count)
		{
			return CELL_MSGDIALOG_ERROR_PARAM;
		}

		break;
	}

	default: return CELL_MSGDIALOG_ERROR_PARAM;
	}

	if (_type.se_mute_on)
	{
		// TODO
	}

	if (_type.se_normal)
	{
		cellSysutil.warning(msgString.get_ptr());
	}
	else
	{
		cellSysutil.error(msgString.get_ptr());
	}

	if (auto rsxthr = fxm::get<GSRender>())
	{
		if (auto dlg = rsxthr->shell_open_message_dialog())
		{
			dlg->show(msgString.get_ptr(), _type, [callback, userData](s32 status)
			{
				if (callback)
				{
					sysutil_register_cb([=](ppu_thread& ppu) -> s32
					{
						callback(ppu, status, userData);
						return CELL_OK;
					});
				}
			});

			return CELL_OK;
		}
	}

	const auto dlg = fxm::import<MsgDialogBase>(Emu.GetCallbacks().get_msg_dialog);

	if (!dlg)
	{
		return CELL_SYSUTIL_ERROR_BUSY;
	}

	dlg->type = _type;

	dlg->on_close = [callback, userData, wptr = std::weak_ptr<MsgDialogBase>(dlg)](s32 status)
	{
		const auto dlg = wptr.lock();

		if (dlg && dlg->state.compare_and_swap_test(MsgDialogState::Open, MsgDialogState::Close))
		{
			if (callback)
			{
				sysutil_register_cb([=](ppu_thread& ppu) -> s32
				{
					callback(ppu, status, userData);
					return CELL_OK;
				});
			}

			fxm::remove<MsgDialogBase>();
		}
	};

	atomic_t<bool> result(false);

	// Run asynchronously in GUI thread
	Emu.CallAfter([&]()
	{
		dlg->Create(msgString.get_ptr());
		result = true;
	});

	while (!result)
	{
		thread_ctrl::wait_for(1000);
	}

	return CELL_OK;
}

s32 cellMsgDialogOpen(u32 type, vm::cptr<char> msgString, vm::ptr<CellMsgDialogCallback> callback, vm::ptr<void> userData, vm::ptr<void> extParam)
{
	//Note: This function needs proper implementation, solve first argument "type" conflict with MsgDialogOpen2 in cellMsgDialog.h.
	cellSysutil.todo("cellMsgDialogOpen(type=0x%x, msgString=%s, callback=*0x%x, userData=*0x%x, extParam=*0x%x)", type, msgString, callback, userData, extParam);
	return cellMsgDialogOpen2(type, msgString, callback, userData, extParam);
}

s32 cellMsgDialogOpenErrorCode(ppu_thread& ppu, u32 errorCode, vm::ptr<CellMsgDialogCallback> callback, vm::ptr<void> userData, vm::ptr<void> extParam)
{
	cellSysutil.warning("cellMsgDialogOpenErrorCode(errorCode=0x%x, callback=*0x%x, userData=*0x%x, extParam=*0x%x)", errorCode, callback, userData, extParam);

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

	return cellMsgDialogOpen2(CELL_MSGDIALOG_TYPE_SE_TYPE_ERROR | CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK, vm::make_str(error), callback, userData, extParam);
}

s32 cellMsgDialogOpenSimulViewWarning(vm::ptr<CellMsgDialogCallback> callback, vm::ptr<void> userData, vm::ptr<void> extParam)
{
	cellSysutil.todo("cellMsgDialogOpenSimulViewWarning(callback=*0x%x, userData=*0x%x, extParam=*0x%x)", callback, userData, extParam);
	return CELL_OK;
}

s32 cellMsgDialogClose(f32 delay)
{
	cellSysutil.warning("cellMsgDialogClose(delay=%f)", delay);

	extern u64 get_system_time();
	const u64 wait_until = get_system_time() + static_cast<s64>(std::max<float>(delay, 0.0f) * 1000);

	if (auto rsxthr = fxm::get<GSRender>())
	{
		if (auto dlg = rsxthr->shell_get_current_dialog())
		{
			thread_ctrl::spawn("cellMsgDialogClose() Thread", [=]
			{
				while (get_system_time() < wait_until)
				{
					if (Emu.IsStopped())
						return;

					if (rsxthr->shell_get_current_dialog() != dlg)
						return;

					std::this_thread::sleep_for(1ms);
				}

				dlg->close();
			});

			return CELL_OK;
		}
	}

	const auto dlg = fxm::get<MsgDialogBase>();

	if (!dlg)
	{
		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	thread_ctrl::spawn("cellMsgDialogClose() Thread", [=]()
	{
		while (dlg->state == MsgDialogState::Open && get_system_time() < wait_until)
		{
			if (Emu.IsStopped()) return;

			std::this_thread::sleep_for(1ms);
		}

		dlg->on_close(CELL_MSGDIALOG_BUTTON_NONE);
	});

	return CELL_OK;
}

s32 cellMsgDialogAbort()
{
	cellSysutil.warning("cellMsgDialogAbort()");

	if (auto rsxthr = fxm::get<GSRender>())
	{
		if (rsxthr->shell_close_dialog())
		{
			return CELL_OK;
		}
	}

	const auto dlg = fxm::get<MsgDialogBase>();

	if (!dlg)
	{
		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	if (!dlg->state.compare_and_swap_test(MsgDialogState::Open, MsgDialogState::Abort))
	{
		return CELL_SYSUTIL_ERROR_BUSY;
	}

	verify(HERE), fxm::remove<MsgDialogBase>();
	return CELL_OK;
}

s32 cellMsgDialogProgressBarSetMsg(u32 progressBarIndex, vm::cptr<char> msgString)
{
	cellSysutil.warning("cellMsgDialogProgressBarSetMsg(progressBarIndex=%d, msgString=%s)", progressBarIndex, msgString);

	if (auto rsxthr = fxm::get<GSRender>())
	{
		if (auto dlg2 = rsxthr->shell_get_current_dialog())
		{
			if (auto casted = dynamic_cast<rsx::overlays::message_dialog*>(dlg2))
				return casted->progress_bar_set_message(progressBarIndex, msgString.get_ptr());
		}
	}

	const auto dlg = fxm::get<MsgDialogBase>();

	if (!dlg)
	{
		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	if (progressBarIndex >= dlg->type.progress_bar_count || !msgString)
	{
		return CELL_MSGDIALOG_ERROR_PARAM;
	}

	Emu.CallAfter([=, msg = std::string{ msgString.get_ptr() }]
	{
		dlg->ProgressBarSetMsg(progressBarIndex, msg);
	});

	return CELL_OK;
}

s32 cellMsgDialogProgressBarReset(u32 progressBarIndex)
{
	cellSysutil.warning("cellMsgDialogProgressBarReset(progressBarIndex=%d)", progressBarIndex);

	if (auto rsxthr = fxm::get<GSRender>())
	{
		if (auto dlg2 = rsxthr->shell_get_current_dialog())
		{
			if (auto casted = dynamic_cast<rsx::overlays::message_dialog*>(dlg2))
				return casted->progress_bar_reset(progressBarIndex);
		}
	}

	const auto dlg = fxm::get<MsgDialogBase>();

	if (!dlg)
	{
		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	if (progressBarIndex >= dlg->type.progress_bar_count)
	{
		return CELL_MSGDIALOG_ERROR_PARAM;
	}

	Emu.CallAfter([=]
	{
		dlg->ProgressBarReset(progressBarIndex);
	});

	return CELL_OK;
}

s32 cellMsgDialogProgressBarInc(u32 progressBarIndex, u32 delta)
{
	cellSysutil.warning("cellMsgDialogProgressBarInc(progressBarIndex=%d, delta=%d)", progressBarIndex, delta);

	if (auto rsxthr = fxm::get<GSRender>())
	{
		if (auto dlg2 = rsxthr->shell_get_current_dialog())
		{
			if (auto casted = dynamic_cast<rsx::overlays::message_dialog*>(dlg2))
				return casted->progress_bar_increment(progressBarIndex, (f32)delta);
		}
	}

	const auto dlg = fxm::get<MsgDialogBase>();

	if (!dlg)
	{
		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	if (progressBarIndex >= dlg->type.progress_bar_count)
	{
		return CELL_MSGDIALOG_ERROR_PARAM;
	}

	Emu.CallAfter([=]
	{
		dlg->ProgressBarInc(progressBarIndex, delta);
	});

	return CELL_OK;
}

void cellSysutil_MsgDialog_init()
{
	REG_FUNC(cellSysutil, cellMsgDialogOpen);
	REG_FUNC(cellSysutil, cellMsgDialogOpen2);
	REG_FUNC(cellSysutil, cellMsgDialogOpenErrorCode);
	REG_FUNC(cellSysutil, cellMsgDialogOpenSimulViewWarning);
	REG_FUNC(cellSysutil, cellMsgDialogProgressBarSetMsg);
	REG_FUNC(cellSysutil, cellMsgDialogProgressBarReset);
	REG_FUNC(cellSysutil, cellMsgDialogProgressBarInc);
	REG_FUNC(cellSysutil, cellMsgDialogClose);
	REG_FUNC(cellSysutil, cellMsgDialogAbort);
}
