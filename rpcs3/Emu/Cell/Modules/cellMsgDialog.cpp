#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/lv2/sys_sync.h"
#include "Emu/RSX/Overlays/overlay_message_dialog.h"

#include "Input/pad_thread.h"

#include "cellSysutil.h"
#include "cellMsgDialog.h"

#include <thread>

#include "util/init_mutex.hpp"

LOG_CHANNEL(cellSysutil);

extern u64 get_guest_system_time();

template<>
void fmt_class_string<CellMsgDialogError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_MSGDIALOG_ERROR_PARAM);
			STR_CASE(CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED);
		}

		return unknown;
	});
}

MsgDialogBase::~MsgDialogBase()
{
}

struct msg_info
{
	std::shared_ptr<MsgDialogBase> dlg;

	stx::init_mutex init;

	// Emulate fxm as if it's some sort of museum

	std::shared_ptr<MsgDialogBase> make() noexcept
	{
		const auto init_lock = init.init();

		if (!init_lock)
		{
			return nullptr;
		}

		dlg = Emu.GetCallbacks().get_msg_dialog();

		return dlg;
	}

	std::shared_ptr<MsgDialogBase> get() noexcept
	{
		const auto init_lock = init.access();

		if (!init_lock)
		{
			return nullptr;
		}

		return dlg;
	}

	void remove() noexcept
	{
		const auto init_lock = init.reset();

		if (!init_lock)
		{
			return;
		}

		dlg.reset();
	}
};

struct msg_dlg_thread_info
{
	atomic_t<u64> wait_until = 0;

	void operator()()
	{
		while (thread_ctrl::state() != thread_state::aborting)
		{
			const u64 new_value = wait_until.load();

			if (new_value == 0)
			{
				wait_until.wait(0);
				continue;
			}

			while (get_guest_system_time() < new_value)
			{
				if (wait_until.load() != new_value)
					break;

				std::this_thread::sleep_for(10ms);
			}

			if (auto manager = g_fxo->get<rsx::overlays::display_manager>())
			{
				if (auto dlg = manager->get<rsx::overlays::message_dialog>())
				{
					if (!wait_until.compare_and_swap_test(new_value, 0))
					{
						continue;
					}

					dlg->close(true, true);
				}
			}
			else if (const auto dlg = g_fxo->get<msg_info>()->get())
			{
				if (!wait_until.compare_and_swap_test(new_value, 0))
				{
					continue;
				}

				dlg->on_close(CELL_MSGDIALOG_BUTTON_NONE);
			}
		}
	}

	static constexpr auto thread_name = "MsgDialog Close Thread"sv;
};

using msg_dlg_thread = named_thread<msg_dlg_thread_info>;

// variable used to immediately get the response from auxiliary message dialogs (callbacks would be async)
atomic_t<s32> g_last_user_response = CELL_MSGDIALOG_BUTTON_NONE;

// forward declaration for open_msg_dialog
error_code cellMsgDialogOpen2(u32 type, vm::cptr<char> msgString, vm::ptr<CellMsgDialogCallback> callback, vm::ptr<void> userData, vm::ptr<void> extParam);

// wrapper to call for other hle dialogs
error_code open_msg_dialog(bool is_blocking, u32 type, vm::cptr<char> msgString, vm::ptr<CellMsgDialogCallback> callback, vm::ptr<void> userData, vm::ptr<void> extParam)
{
	cellSysutil.warning("open_msg_dialog(is_blocking=%d, type=0x%x, msgString=%s, callback=*0x%x, userData=*0x%x, extParam=*0x%x)", is_blocking, type, msgString, callback, userData, extParam);

	const MsgDialogType _type{ type };

	if (auto manager = g_fxo->get<rsx::overlays::display_manager>())
	{
		if (manager->get<rsx::overlays::message_dialog>())
		{
			return CELL_SYSUTIL_ERROR_BUSY;
		}

		g_last_user_response = CELL_MSGDIALOG_BUTTON_NONE;

		const auto res = manager->create<rsx::overlays::message_dialog>()->show(is_blocking, msgString.get_ptr(), _type, [callback, userData](s32 status)
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

		return res;
	}

	const auto dlg = g_fxo->get<msg_info>()->make();

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

			g_fxo->get<msg_dlg_thread>()->wait_until = 0;
			g_fxo->get<msg_info>()->remove();
		}

		pad::SetIntercepted(false);
	};

	pad::SetIntercepted(true);

	auto& ppu = *get_current_cpu_thread();
	lv2_obj::sleep(ppu);

	// Run asynchronously in GUI thread
	Emu.CallAfter([&]()
	{
		g_last_user_response = CELL_MSGDIALOG_BUTTON_NONE;
		dlg->Create(msgString.get_ptr());
		lv2_obj::awake(&ppu);
	});

	while (!ppu.state.test_and_reset(cpu_flag::signal))
	{
		if (ppu.is_stopped())
		{
			return 0;
		}

		thread_ctrl::wait();
	}

	if (is_blocking)
	{
		while (auto dlg = g_fxo->get<msg_info>()->get())
		{
			if (Emu.IsStopped() || dlg->state != MsgDialogState::Open)
			{
				break;
			}
			std::this_thread::yield();
		}
	}

	return CELL_OK;
}

void exit_game(s32/* buttonType*/, vm::ptr<void>/* userData*/)
{
	sysutil_send_system_cmd(CELL_SYSUTIL_REQUEST_EXITGAME, 0);
}

error_code open_exit_dialog(const std::string& message, bool is_exit_requested)
{
	cellSysutil.warning("open_exit_dialog(message=%s, is_exit_requested=%d)", message, is_exit_requested);

	vm::bptr<CellMsgDialogCallback> callback = vm::null;

	if (is_exit_requested)
	{
		callback.set(ppu_function_manager::addr + 8 * FIND_FUNC(exit_game));
	}

	const error_code res = open_msg_dialog
	(
		true,
		CELL_MSGDIALOG_TYPE_SE_TYPE_ERROR | CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK | CELL_MSGDIALOG_TYPE_DISABLE_CANCEL_ON,
		vm::make_str(message),
		callback
	);

	if (res != CELL_OK)
	{
		// Something went wrong, exit anyway
		if (is_exit_requested)
		{
			sysutil_send_system_cmd(CELL_SYSUTIL_REQUEST_EXITGAME, 0);
		}
	}

	return CELL_OK;
}

error_code cellMsgDialogOpen2(u32 type, vm::cptr<char> msgString, vm::ptr<CellMsgDialogCallback> callback, vm::ptr<void> userData, vm::ptr<void> extParam)
{
	cellSysutil.warning("cellMsgDialogOpen2(type=0x%x, msgString=%s, callback=*0x%x, userData=*0x%x, extParam=*0x%x)", type, msgString, callback, userData, extParam);

	if (!msgString || std::strlen(msgString.get_ptr()) >= CELL_MSGDIALOG_STRING_SIZE || type & -0x33f8)
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
		cellSysutil.warning("%s", msgString);
	}
	else
	{
		cellSysutil.error("%s", msgString);
	}

	return open_msg_dialog(false, type, msgString, callback, userData, extParam);
}

error_code cellMsgDialogOpen(u32 type, vm::cptr<char> msgString, vm::ptr<CellMsgDialogCallback> callback, vm::ptr<void> userData, vm::ptr<void> extParam)
{
	// Note: This function needs proper implementation, solve first argument "type" conflict with MsgDialogOpen2 in cellMsgDialog.h.
	cellSysutil.todo("cellMsgDialogOpen(type=0x%x, msgString=%s, callback=*0x%x, userData=*0x%x, extParam=*0x%x)", type, msgString, callback, userData, extParam);
	return cellMsgDialogOpen2(type, msgString, callback, userData, extParam);
}

error_code cellMsgDialogOpenErrorCode(u32 errorCode, vm::ptr<CellMsgDialogCallback> callback, vm::ptr<void> userData, vm::ptr<void> extParam)
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

error_code cellMsgDialogClose(f32 delay)
{
	cellSysutil.warning("cellMsgDialogClose(delay=%f)", delay);

	const u64 wait_until = get_guest_system_time() + static_cast<s64>(std::max<float>(delay, 0.0f) * 1000);

	if (auto manager = g_fxo->get<rsx::overlays::display_manager>())
	{
		if (auto dlg = manager->get<rsx::overlays::message_dialog>())
		{
			const auto thr = g_fxo->get<msg_dlg_thread>();
			thr->wait_until = wait_until;
			thr->wait_until.notify_one();
			return CELL_OK;
		}

		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	const auto dlg = g_fxo->get<msg_info>()->get();

	if (!dlg)
	{
		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	const auto thr = g_fxo->get<msg_dlg_thread>();
	thr->wait_until = wait_until;
	thr->wait_until.notify_one();
	return CELL_OK;
}

error_code cellMsgDialogAbort()
{
	cellSysutil.warning("cellMsgDialogAbort()");

	if (auto manager = g_fxo->get<rsx::overlays::display_manager>())
	{
		if (auto dlg = manager->get<rsx::overlays::message_dialog>())
		{
			g_fxo->get<msg_dlg_thread>()->wait_until = 0;
			dlg->close(false, true);
			return CELL_OK;
		}
	}

	const auto dlg = g_fxo->get<msg_info>()->get();

	if (!dlg)
	{
		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	if (!dlg->state.compare_and_swap_test(MsgDialogState::Open, MsgDialogState::Abort))
	{
		return CELL_SYSUTIL_ERROR_BUSY;
	}

	g_fxo->get<msg_dlg_thread>()->wait_until = 0;
	g_fxo->get<msg_info>()->remove(); // this shouldn't call on_close
	pad::SetIntercepted(false);       // so we need to reenable the pads here

	return CELL_OK;
}

error_code cellMsgDialogOpenSimulViewWarning(vm::ptr<CellMsgDialogCallback> callback, vm::ptr<void> userData, vm::ptr<void> extParam)
{
	cellSysutil.todo("cellMsgDialogOpenSimulViewWarning(callback=*0x%x, userData=*0x%x, extParam=*0x%x)", callback, userData, extParam);

	error_code ret = cellMsgDialogOpen2(CELL_MSGDIALOG_TYPE_SE_TYPE_NORMAL | CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK, vm::make_str("SimulView Warning"), callback, userData, extParam);

	// The dialog should ideally only be closeable by pressing ok after 3 seconds until it closes itself automatically after 5 seconds
	if (ret == CELL_OK)
		cellMsgDialogClose(5000.0f);

	return ret;
}

error_code cellMsgDialogProgressBarSetMsg(u32 progressBarIndex, vm::cptr<char> msgString)
{
	cellSysutil.warning("cellMsgDialogProgressBarSetMsg(progressBarIndex=%d, msgString=%s)", progressBarIndex, msgString);

	if (!msgString || std::strlen(msgString.get_ptr()) >= CELL_MSGDIALOG_PROGRESSBAR_STRING_SIZE)
	{
		return CELL_MSGDIALOG_ERROR_PARAM;
	}

	if (auto manager = g_fxo->get<rsx::overlays::display_manager>())
	{
		if (auto dlg = manager->get<rsx::overlays::message_dialog>())
		{
			return dlg->progress_bar_set_message(progressBarIndex, msgString.get_ptr());
		}
	}

	const auto dlg = g_fxo->get<msg_info>()->get();

	if (!dlg)
	{
		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	if (progressBarIndex >= dlg->type.progress_bar_count)
	{
		return CELL_MSGDIALOG_ERROR_PARAM;
	}

	Emu.CallAfter([=, msg = std::string{ msgString.get_ptr() }]
	{
		dlg->ProgressBarSetMsg(progressBarIndex, msg);
	});

	return CELL_OK;
}

error_code cellMsgDialogProgressBarReset(u32 progressBarIndex)
{
	cellSysutil.warning("cellMsgDialogProgressBarReset(progressBarIndex=%d)", progressBarIndex);

	if (auto manager = g_fxo->get<rsx::overlays::display_manager>())
	{
		if (auto dlg = manager->get<rsx::overlays::message_dialog>())
		{
			return dlg->progress_bar_reset(progressBarIndex);
		}
	}

	const auto dlg = g_fxo->get<msg_info>()->get();

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

error_code cellMsgDialogProgressBarInc(u32 progressBarIndex, u32 delta)
{
	cellSysutil.warning("cellMsgDialogProgressBarInc(progressBarIndex=%d, delta=%d)", progressBarIndex, delta);

	if (auto manager = g_fxo->get<rsx::overlays::display_manager>())
	{
		if (auto dlg = manager->get<rsx::overlays::message_dialog>())
		{
			return dlg->progress_bar_increment(progressBarIndex, static_cast<f32>(delta));
		}
	}

	const auto dlg = g_fxo->get<msg_info>()->get();

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

	// Helper Function
	REG_FUNC(cellSysutil, exit_game).flag(MFF_HIDDEN);
}
