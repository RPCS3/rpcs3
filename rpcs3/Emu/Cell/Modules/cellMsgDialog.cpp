#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/lv2/sys_sync.h"
#include "Emu/Cell/timers.hpp"
#include "Emu/Io/interception.h"

#include "Emu/RSX/Overlays/overlay_manager.h"
#include "Emu/RSX/Overlays/overlay_message_dialog.h"

#include "cellSysutil.h"
#include "cellMsgDialog.h"

#include <thread>

#include "util/init_mutex.hpp"

LOG_CHANNEL(cellSysutil);

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

template<>
void fmt_class_string<msg_dialog_source>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto src)
	{
		switch (src)
		{
			case msg_dialog_source::_cellMsgDialog: return "cellMsgDialog";
			case msg_dialog_source::_cellSaveData: return "cellSaveData";
			case msg_dialog_source::_cellGame: return "cellGame";
			case msg_dialog_source::_cellCrossController: return "cellCrossController";
			case msg_dialog_source::_sceNp: return "sceNp";
			case msg_dialog_source::_sceNpTrophy: return "sceNpTrophy";
			case msg_dialog_source::sys_progress: return "sys_progress";
			case msg_dialog_source::shader_loading: return "shader_loading";
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
				thread_ctrl::wait_on(wait_until, 0);
				continue;
			}

			while (get_guest_system_time() < new_value)
			{
				if (wait_until.load() != new_value)
					break;

				thread_ctrl::wait_for(10'000);
			}

			if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>())
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
			else if (const auto dlg = g_fxo->get<msg_info>().get())
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

// forward declaration for open_msg_dialog
error_code cellMsgDialogOpen2(u32 type, vm::cptr<char> msgString, vm::ptr<CellMsgDialogCallback> callback, vm::ptr<void> userData, vm::ptr<void> extParam);

// wrapper to call for other hle dialogs
error_code open_msg_dialog(bool is_blocking, u32 type, vm::cptr<char> msgString, msg_dialog_source source, vm::ptr<CellMsgDialogCallback> callback, vm::ptr<void> userData, vm::ptr<void> extParam, s32* return_code)
{
	cellSysutil.notice("open_msg_dialog(is_blocking=%d, type=0x%x, msgString=%s, source=%s, callback=*0x%x, userData=*0x%x, extParam=*0x%x, return_code=*0x%x)", is_blocking, type, msgString, source, callback, userData, extParam, return_code);

	const MsgDialogType _type{ type };

	if (return_code)
	{
		*return_code = CELL_MSGDIALOG_BUTTON_NONE;
	}

	if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>())
	{
		if (manager->get<rsx::overlays::message_dialog>())
		{
			return CELL_SYSUTIL_ERROR_BUSY;
		}

		if (s32 ret = sysutil_send_system_cmd(CELL_SYSUTIL_DRAWING_BEGIN, 0); ret < 0)
		{
			return CellSysutilError{ret + 0u};
		}

		const auto notify = std::make_shared<atomic_t<u32>>(0);

		const auto res = manager->create<rsx::overlays::message_dialog>()->show(is_blocking, msgString.get_ptr(), _type, source, [callback, userData, &return_code, is_blocking, notify](s32 status)
		{
			if (is_blocking && return_code)
			{
				*return_code = status;
			}

			sysutil_send_system_cmd(CELL_SYSUTIL_DRAWING_END, 0);

			if (callback)
			{
				sysutil_register_cb([=](ppu_thread& ppu) -> s32
				{
					callback(ppu, status, userData);
					return CELL_OK;
				});
			}

			if (is_blocking && notify)
			{
				*notify = 1;
				notify->notify_one();
			}
		});

		// Wait for on_close
		while (is_blocking && !Emu.IsStopped() && !*notify)
		{
			notify->wait(false, atomic_wait_timeout{1'000'000});
		}

		return res;
	}

	const auto dlg = g_fxo->get<msg_info>().make();

	if (!dlg)
	{
		return CELL_SYSUTIL_ERROR_BUSY;
	}

	if (s32 ret = sysutil_send_system_cmd(CELL_SYSUTIL_DRAWING_BEGIN, 0); ret < 0)
	{
		return CellSysutilError{ret + 0u};
	}

	dlg->type = _type;
	dlg->source = source;

	dlg->on_close = [callback, userData, is_blocking, &return_code, wptr = std::weak_ptr<MsgDialogBase>(dlg)](s32 status)
	{
		if (is_blocking && return_code)
		{
			*return_code = status;
		}

		const auto dlg = wptr.lock();

		if (dlg && dlg->state.compare_and_swap_test(MsgDialogState::Open, MsgDialogState::Close))
		{
			sysutil_send_system_cmd(CELL_SYSUTIL_DRAWING_END, 0);

			if (callback)
			{
				sysutil_register_cb([=](ppu_thread& ppu) -> s32
				{
					callback(ppu, status, userData);
					return CELL_OK;
				});
			}

			g_fxo->get<msg_dlg_thread>().wait_until = 0;
			g_fxo->get<msg_info>().remove();
		}

		input::SetIntercepted(false);
	};

	input::SetIntercepted(true);

	auto& ppu = *get_current_cpu_thread();
	lv2_obj::sleep(ppu);

	// PS3 memory must not be accessed by Main thread
	std::string msg_string = msgString.get_ptr();

	// Run asynchronously in GUI thread
	Emu.CallFromMainThread([&, msg_string = std::move(msg_string)]()
	{
		dlg->Create(msg_string);
		lv2_obj::awake(&ppu);
	});

	while (auto state = ppu.state.fetch_sub(cpu_flag::signal))
	{
		if (is_stopped(state))
		{
			return {};
		}

		if (state & cpu_flag::signal)
		{
			break;
		}

		ppu.state.wait(state);
	}

	if (is_blocking)
	{
		while (auto dlg = g_fxo->get<msg_info>().get())
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

void close_msg_dialog()
{
	cellSysutil.notice("close_msg_dialog()");

	if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>())
	{
		if (auto dlg = manager->get<rsx::overlays::message_dialog>())
		{
			g_fxo->get<msg_dlg_thread>().wait_until = 0;
			dlg->close(false, true); // this doesn't call on_close
			sysutil_send_system_cmd(CELL_SYSUTIL_DRAWING_END, 0);
			return;
		}
	}

	if (const auto dlg = g_fxo->get<msg_info>().get())
	{
		dlg->state = MsgDialogState::Close;
		g_fxo->get<msg_dlg_thread>().wait_until = 0;
		g_fxo->get<msg_info>().remove(); // this shouldn't call on_close
		input::SetIntercepted(false);    // so we need to reenable the pads here
		sysutil_send_system_cmd(CELL_SYSUTIL_DRAWING_END, 0);
	}
}

void exit_game(s32/* buttonType*/, vm::ptr<void>/* userData*/)
{
	sysutil_send_system_cmd(CELL_SYSUTIL_REQUEST_EXITGAME, 0);
}

error_code open_exit_dialog(const std::string& message, bool is_exit_requested, msg_dialog_source source)
{
	cellSysutil.notice("open_exit_dialog(message=%s, is_exit_requested=%d, source=%s)", message, is_exit_requested, source);

	vm::bptr<CellMsgDialogCallback> callback = vm::null;

	if (is_exit_requested)
	{
		callback.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(exit_game)));
	}

	const error_code res = open_msg_dialog
	(
		true,
		CELL_MSGDIALOG_TYPE_SE_TYPE_ERROR | CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK | CELL_MSGDIALOG_TYPE_DISABLE_CANCEL_ON,
		vm::make_str(message),
		source,
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

	if (_type.se_normal)
	{
		cellSysutil.warning("Opening message dialog with message: %s", msgString);
	}
	else
	{
		cellSysutil.error("Opening error message dialog with message: %s", msgString);
	}

	return open_msg_dialog(false, type, msgString, msg_dialog_source::_cellMsgDialog, callback, userData, extParam);
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

	localized_string_id string_id = localized_string_id::INVALID;

	switch (errorCode)
	{
	case 0x80010001: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010001; break; // The resource is temporarily unavailable.
	case 0x80010002: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010002; break; // Invalid argument or flag.
	case 0x80010003: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010003; break; // The feature is not yet implemented.
	case 0x80010004: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010004; break; // Memory allocation failed.
	case 0x80010005: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010005; break; // The resource with the specified identifier does not exist.
	case 0x80010006: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010006; break; // The file does not exist.
	case 0x80010007: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010007; break; // The file is in unrecognized format / The file is not a valid ELF file.
	case 0x80010008: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010008; break; // Resource deadlock is avoided.
	case 0x80010009: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010009; break; // Operation not permitted.
	case 0x8001000A: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_8001000A; break; // The device or resource is busy.
	case 0x8001000B: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_8001000B; break; // The operation is timed out.
	case 0x8001000C: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_8001000C; break; // The operation is aborted.
	case 0x8001000D: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_8001000D; break; // Invalid memory access.
	case 0x8001000F: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_8001000F; break; // State of the target thread is invalid.
	case 0x80010010: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010010; break; // Alignment is invalid.
	case 0x80010011: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010011; break; // Shortage of the kernel resources.
	case 0x80010012: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010012; break; // The file is a directory.
	case 0x80010013: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010013; break; // Operation cancelled.
	case 0x80010014: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010014; break; // Entry already exists.
	case 0x80010015: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010015; break; // Port is already connected.
	case 0x80010016: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010016; break; // Port is not connected.
	case 0x80010017: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010017; break; // Failure in authorizing SELF. Program authentication fail.
	case 0x80010018: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010018; break; // The file is not MSELF.
	case 0x80010019: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010019; break; // System version error.
	case 0x8001001A: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_8001001A; break; // Fatal system error occurred while authorizing SELF. SELF auth failure.
	case 0x8001001B: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_8001001B; break; // Math domain violation.
	case 0x8001001C: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_8001001C; break; // Math range violation.
	case 0x8001001D: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_8001001D; break; // Illegal multi-byte sequence in input.
	case 0x8001001E: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_8001001E; break; // File position error.
	case 0x8001001F: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_8001001F; break; // Syscall was interrupted.
	case 0x80010020: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010020; break; // File too large.
	case 0x80010021: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010021; break; // Too many links.
	case 0x80010022: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010022; break; // File table overflow.
	case 0x80010023: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010023; break; // No space left on device.
	case 0x80010024: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010024; break; // Not a TTY.
	case 0x80010025: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010025; break; // Broken pipe.
	case 0x80010026: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010026; break; // Read-only filesystem.
	case 0x80010027: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010027; break; // Illegal seek.
	case 0x80010028: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010028; break; // Arg list too long.
	case 0x80010029: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010029; break; // Access violation.
	case 0x8001002A: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_8001002A; break; // Invalid file descriptor.
	case 0x8001002B: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_8001002B; break; // Filesystem mounting failed.
	case 0x8001002C: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_8001002C; break; // Too many files open.
	case 0x8001002D: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_8001002D; break; // No device.
	case 0x8001002E: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_8001002E; break; // Not a directory.
	case 0x8001002F: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_8001002F; break; // No such device or IO.
	case 0x80010030: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010030; break; // Cross-device link error.
	case 0x80010031: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010031; break; // Bad Message.
	case 0x80010032: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010032; break; // In progress.
	case 0x80010033: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010033; break; // Message size error.
	case 0x80010034: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010034; break; // Name too long.
	case 0x80010035: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010035; break; // No lock.
	case 0x80010036: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010036; break; // Not empty.
	case 0x80010037: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010037; break; // Not supported.
	case 0x80010038: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010038; break; // File-system specific error.
	case 0x80010039: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_80010039; break; // Overflow occured.
	case 0x8001003A: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_8001003A; break; // Filesystem not mounted.
	case 0x8001003B: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_8001003B; break; // Not SData.
	case 0x8001003C: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_8001003C; break; // Incorrect version in sys_load_param.
	case 0x8001003D: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_8001003D; break; // Pointer is null.
	case 0x8001003E: string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_8001003E; break; // Pointer is null.
	default:         string_id = localized_string_id::CELL_MSG_DIALOG_ERROR_DEFAULT;  break; // An error has occurred.
	}

	const std::string error = get_localized_string(string_id, fmt::format("%08x", errorCode).c_str());

	return cellMsgDialogOpen2(CELL_MSGDIALOG_TYPE_SE_TYPE_ERROR | CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK, vm::make_str(error), callback, userData, extParam);
}

error_code cellMsgDialogClose(f32 delay)
{
	cellSysutil.warning("cellMsgDialogClose(delay=%f)", delay);

	const u64 wait_until = get_guest_system_time() + static_cast<s64>(std::max<float>(delay, 0.0f) * 1000);

	if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>())
	{
		if (auto dlg = manager->get<rsx::overlays::message_dialog>();
			dlg && dlg->source() == msg_dialog_source::_cellMsgDialog)
		{
			auto& thr = g_fxo->get<msg_dlg_thread>();
			thr.wait_until = wait_until;
			thr.wait_until.notify_one();
			return CELL_OK;
		}

		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	const auto dlg = g_fxo->get<msg_info>().get();

	if (!dlg || dlg->source != msg_dialog_source::_cellMsgDialog)
	{
		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	auto& thr = g_fxo->get<msg_dlg_thread>();
	thr.wait_until = wait_until;
	thr.wait_until.notify_one();
	return CELL_OK;
}

error_code cellMsgDialogAbort()
{
	cellSysutil.warning("cellMsgDialogAbort()");

	if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>())
	{
		if (auto dlg = manager->get<rsx::overlays::message_dialog>();
			dlg && dlg->source() == msg_dialog_source::_cellMsgDialog)
		{
			g_fxo->get<msg_dlg_thread>().wait_until = 0;
			dlg->close(false, true); // this doesn't call on_close
			sysutil_send_system_cmd(CELL_SYSUTIL_DRAWING_END, 0);
			return CELL_OK;
		}

		return CELL_OK; // Not CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED, tested on HW.
	}

	const auto dlg = g_fxo->get<msg_info>().get();

	if (!dlg || dlg->source != msg_dialog_source::_cellMsgDialog)
	{
		return CELL_OK; // Not CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED, tested on HW.
	}

	if (!dlg->state.compare_and_swap_test(MsgDialogState::Open, MsgDialogState::Abort))
	{
		return CELL_SYSUTIL_ERROR_BUSY;
	}

	g_fxo->get<msg_dlg_thread>().wait_until = 0;
	g_fxo->get<msg_info>().remove(); // this shouldn't call on_close
	input::SetIntercepted(false);     // so we need to reenable the pads here
	sysutil_send_system_cmd(CELL_SYSUTIL_DRAWING_END, 0);

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

	if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>())
	{
		if (auto dlg = manager->get<rsx::overlays::message_dialog>())
		{
			return dlg->progress_bar_set_message(progressBarIndex, msgString.get_ptr());
		}
	}

	const auto dlg = g_fxo->get<msg_info>().get();

	if (!dlg)
	{
		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	if (progressBarIndex >= dlg->type.progress_bar_count)
	{
		return CELL_MSGDIALOG_ERROR_PARAM;
	}

	Emu.CallFromMainThread([=, msg = std::string{ msgString.get_ptr() }]
	{
		dlg->ProgressBarSetMsg(progressBarIndex, msg);
	});

	return CELL_OK;
}

error_code cellMsgDialogProgressBarReset(u32 progressBarIndex)
{
	cellSysutil.warning("cellMsgDialogProgressBarReset(progressBarIndex=%d)", progressBarIndex);

	if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>())
	{
		if (auto dlg = manager->get<rsx::overlays::message_dialog>())
		{
			return dlg->progress_bar_reset(progressBarIndex);
		}
	}

	const auto dlg = g_fxo->get<msg_info>().get();

	if (!dlg)
	{
		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	if (progressBarIndex >= dlg->type.progress_bar_count)
	{
		return CELL_MSGDIALOG_ERROR_PARAM;
	}

	Emu.CallFromMainThread([=]
	{
		dlg->ProgressBarReset(progressBarIndex);
	});

	return CELL_OK;
}

error_code cellMsgDialogProgressBarInc(u32 progressBarIndex, u32 delta)
{
	cellSysutil.warning("cellMsgDialogProgressBarInc(progressBarIndex=%d, delta=%d)", progressBarIndex, delta);

	if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>())
	{
		if (auto dlg = manager->get<rsx::overlays::message_dialog>())
		{
			return dlg->progress_bar_increment(progressBarIndex, static_cast<f32>(delta));
		}
	}

	const auto dlg = g_fxo->get<msg_info>().get();

	if (!dlg)
	{
		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	if (progressBarIndex >= dlg->type.progress_bar_count)
	{
		return CELL_MSGDIALOG_ERROR_PARAM;
	}

	Emu.CallFromMainThread([=]
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
	REG_HIDDEN_FUNC(exit_game);
}
