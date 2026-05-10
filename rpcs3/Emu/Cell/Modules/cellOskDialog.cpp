#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Io/interception.h"
#include "Emu/Io/Keyboard.h"
#include "Emu/IdManager.h"

#include "Emu/RSX/Overlays/overlay_manager.h"
#include "Emu/RSX/Overlays/overlay_osk.h"

#include "cellSysutil.h"
#include "cellOskDialog.h"
#include "cellMsgDialog.h"
#include "cellImeJp.h"

#include <thread>

LOG_CHANNEL(cellOskDialog);

template<>
void fmt_class_string<CellOskDialogError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_OSKDIALOG_ERROR_IME_ALREADY_IN_USE);
			STR_CASE(CELL_OSKDIALOG_ERROR_GET_SIZE_ERROR);
			STR_CASE(CELL_OSKDIALOG_ERROR_UNKNOWN);
			STR_CASE(CELL_OSKDIALOG_ERROR_PARAM);
		}

		return unknown;
	});
}

template<>
void fmt_class_string<CellOskDialogContinuousMode>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto mode)
	{
		switch (mode)
		{
			STR_CASE(CELL_OSKDIALOG_CONTINUOUS_MODE_NONE);
			STR_CASE(CELL_OSKDIALOG_CONTINUOUS_MODE_REMAIN_OPEN);
			STR_CASE(CELL_OSKDIALOG_CONTINUOUS_MODE_HIDE);
			STR_CASE(CELL_OSKDIALOG_CONTINUOUS_MODE_SHOW);
		}

		return unknown;
	});
}

void osk_info::reset()
{
	std::lock_guard lock(text_mtx);

	dlg.reset();
	valid_text = {};
	use_separate_windows = false;
	lock_ext_input_device = false;
	device_mask = 0;
	input_field_window_width = 0;
	input_field_background_transparency = 1.0f;
	input_field_layout_info = {};
	input_panel_layout_info = {};
	key_layout_options = CELL_OSKDIALOG_10KEY_PANEL;
	initial_key_layout = CELL_OSKDIALOG_INITIAL_PANEL_LAYOUT_SYSTEM;
	initial_input_device = CELL_OSKDIALOG_INPUT_DEVICE_PAD;
	clipboard_enabled = false;
	half_byte_kana_enabled = false;
	supported_languages = 0;
	dimmer_enabled = true;
	base_color = OskDialogBase::color{ 0.2f, 0.2f, 0.2f, 1.0f };
	pointer_enabled = false;
	pointer_x = 0.0f;
	pointer_y = 0.0f;
	initial_scale = 1.0f;
	layout = {};
	osk_continuous_mode = CELL_OSKDIALOG_CONTINUOUS_MODE_NONE;
	last_dialog_state = CELL_SYSUTIL_OSKDIALOG_UNLOADED;
	osk_confirm_callback.store({});
	osk_force_finish_callback.store({});
	osk_hardware_keyboard_event_hook_callback.store({});
	hook_event_mode.store(0);
}

// Align horizontally
u32 osk_info::get_aligned_x(u32 layout_mode)
{
	// Let's prefer a centered alignment.
	if (layout_mode & CELL_OSKDIALOG_LAYOUTMODE_X_ALIGN_CENTER)
	{
		return CELL_OSKDIALOG_LAYOUTMODE_X_ALIGN_CENTER;
	}

	if (layout_mode & CELL_OSKDIALOG_LAYOUTMODE_X_ALIGN_LEFT)
	{
		return CELL_OSKDIALOG_LAYOUTMODE_X_ALIGN_LEFT;
	}

	return CELL_OSKDIALOG_LAYOUTMODE_X_ALIGN_RIGHT;
}

// Align vertically
u32 osk_info::get_aligned_y(u32 layout_mode)
{
	// Let's prefer a centered alignment.
	if (layout_mode & CELL_OSKDIALOG_LAYOUTMODE_Y_ALIGN_CENTER)
	{
		return CELL_OSKDIALOG_LAYOUTMODE_Y_ALIGN_CENTER;
	}

	if (layout_mode & CELL_OSKDIALOG_LAYOUTMODE_Y_ALIGN_TOP)
	{
		return CELL_OSKDIALOG_LAYOUTMODE_Y_ALIGN_TOP;
	}

	return CELL_OSKDIALOG_LAYOUTMODE_Y_ALIGN_BOTTOM;
}

// TODO: don't use this function
std::shared_ptr<OskDialogBase> _get_osk_dialog(bool create)
{
	auto& osk = g_fxo->get<osk_info>();

	if (create)
	{
		const auto init = osk.init.init();

		if (!init)
		{
			return nullptr;
		}

		if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>())
		{
			std::shared_ptr<rsx::overlays::osk_dialog> dlg = std::make_shared<rsx::overlays::osk_dialog>();
			osk.dlg = manager->add(dlg);
		}
		else
		{
			osk.dlg = Emu.GetCallbacks().get_osk_dialog();
		}

		return osk.dlg;
	}

	const auto init = osk.init.access();

	if (!init)
	{
		return nullptr;
	}

	return osk.dlg;
}

extern bool close_osk_from_ps_button()
{
	const auto osk = _get_osk_dialog(false);
	if (!osk)
	{
		// The OSK is not open
		return true;
	}

	osk_info& info = g_fxo->get<osk_info>();

	// We can only close the osk in separate window mode when it is hidden (continuous_mode is set to CELL_OSKDIALOG_CONTINUOUS_MODE_HIDE)
	if (!info.use_separate_windows || osk->continuous_mode != CELL_OSKDIALOG_CONTINUOUS_MODE_HIDE)
	{
		cellOskDialog.warning("close_osk_from_ps_button: can't close OSK (use_separate_windows=%d, continuous_mode=%s)", info.use_separate_windows.load(), osk->continuous_mode.load());
		return false;
	}

	std::lock_guard lock(info.text_mtx);

	if (auto cb = info.osk_force_finish_callback.load())
	{
		bool done = false;
		bool close_osk = false;

		sysutil_register_cb([&](ppu_thread& cb_ppu) -> s32
		{
			cellOskDialog.notice("osk_force_finish_callback()");
			close_osk = cb(cb_ppu);
			cellOskDialog.notice("osk_force_finish_callback returned %d", close_osk);
			done = true;
			return 0;
		});

		// wait for check callback
		while (!done && !Emu.IsStopped())
		{
			std::this_thread::yield();
		}

		if (!close_osk)
		{
			// We are not allowed to close the OSK
			cellOskDialog.warning("close_osk_from_ps_button: can't close OSK (osk_force_finish_callback returned false)");
			return false;
		}
	}

	// Forcefully terminate the OSK
	cellOskDialog.warning("close_osk_from_ps_button: Terminating the OSK ...");

	osk->Close(FAKE_CELL_OSKDIALOG_CLOSE_TERMINATE);
	osk->state = OskDialogState::Closed;

	cellOskDialog.notice("close_osk_from_ps_button: sending CELL_SYSUTIL_OSKDIALOG_FINISHED");
	sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_FINISHED, 0);

	return true;
}


error_code cellOskDialogLoadAsync(u32 container, vm::ptr<CellOskDialogParam> dialogParam, vm::ptr<CellOskDialogInputFieldInfo> inputFieldInfo)
{
	cellOskDialog.warning("cellOskDialogLoadAsync(container=0x%x, dialogParam=*0x%x, inputFieldInfo=*0x%x)", container, dialogParam, inputFieldInfo);

	if (!dialogParam || !inputFieldInfo || !inputFieldInfo->message || !inputFieldInfo->init_text || inputFieldInfo->limit_length > CELL_OSKDIALOG_STRING_SIZE)
	{
		return CELL_OSKDIALOG_ERROR_PARAM;
	}

	cellOskDialog.notice("cellOskDialogLoadAsync: dialogParam={ allowOskPanelFlg=0x%x, prohibitFlgs=0x%x, firstViewPanel=%d, controlPoint=(%.2f,%.2f) }",
		dialogParam->allowOskPanelFlg, dialogParam->prohibitFlgs, dialogParam->firstViewPanel, dialogParam->controlPoint.x, dialogParam->controlPoint.y);

	auto osk = _get_osk_dialog(true);

	// Can't open another dialog if this one is already open.
	if (!osk || osk->state.load() != OskDialogState::Unloaded)
	{
		return CELL_SYSUTIL_ERROR_BUSY;
	}

	// Get the OSK options
	auto& info = g_fxo->get<osk_info>();
	u32 maxLength = (inputFieldInfo->limit_length >= CELL_OSKDIALOG_STRING_SIZE) ? 511 : u32{inputFieldInfo->limit_length};
	const u32 prohibitFlgs = dialogParam->prohibitFlgs;
	const u32 allowOskPanelFlg = dialogParam->allowOskPanelFlg;
	const u32 firstViewPanel = dialogParam->firstViewPanel;
	info.layout.x_offset = dialogParam->controlPoint.x;
	info.layout.y_offset = dialogParam->controlPoint.y;

	// Get init text and prepare return value
	osk->osk_input_result = CELL_OSKDIALOG_INPUT_FIELD_RESULT_OK;
	osk->osk_text = {};

	// Also clear the info text just to be sure (it should be zeroed at this point anyway)
	{
		std::lock_guard lock(info.text_mtx);
		info.valid_text = {};
	}

	if (inputFieldInfo->init_text)
	{
		for (u32 i = 0; (i < maxLength) && (inputFieldInfo->init_text[i] != 0); i++)
		{
			osk->osk_text[i] = inputFieldInfo->init_text[i];
		}
	}

	// Get message to display above the input field
	// Guarantees 0 terminated (+1). In praxis only 128 but for now lets display all of it
	char16_t message[CELL_OSKDIALOG_STRING_SIZE + 1]{};

	if (inputFieldInfo->message)
	{
		for (u32 i = 0; (i < CELL_OSKDIALOG_STRING_SIZE) && (inputFieldInfo->message[i] != 0); i++)
		{
			message[i] = inputFieldInfo->message[i];
		}
	}

	osk->on_osk_close = [](s32 status)
	{
		cellOskDialog.notice("on_osk_close(status=%d)", status);

		const auto osk = _get_osk_dialog(false);
		if (!osk)
		{
			return;
		}

		osk->state = OskDialogState::Closed;

		auto& info = g_fxo->get<osk_info>();

		const bool keep_seperate_window_open = info.use_separate_windows.load() && (info.osk_continuous_mode.load() != CELL_OSKDIALOG_CONTINUOUS_MODE_NONE);

		switch (status)
		{
		case CELL_OSKDIALOG_CLOSE_CONFIRM:
		{
			if (auto ccb = info.osk_confirm_callback.load())
			{
				std::vector<u16> string_to_send(CELL_OSKDIALOG_STRING_SIZE);
				atomic_t<bool> done = false;
				u32 i;

				for (i = 0; i < CELL_OSKDIALOG_STRING_SIZE - 1; i++)
				{
					string_to_send[i] = osk->osk_text[i];
					if (osk->osk_text[i] == 0) break;
				}

				sysutil_register_cb([&, length = i, string_to_send = std::move(string_to_send)](ppu_thread& cb_ppu) -> s32
				{
					vm::var<u16[], vm::page_allocator<>> string_var(CELL_OSKDIALOG_STRING_SIZE, string_to_send.data());

					const u32 return_value = ccb(cb_ppu, string_var.begin(), static_cast<s32>(length));
					cellOskDialog.warning("osk_confirm_callback return_value=%d", return_value);

					for (u32 i = 0; i < CELL_OSKDIALOG_STRING_SIZE - 1; i++)
					{
						osk->osk_text[i] = string_var.begin()[i];
					}

					done = true;
					return 0;
				});

				// wait for check callback
				while (!done && !Emu.IsStopped())
				{
					std::this_thread::yield();
				}
			}

			if (info.use_separate_windows.load() && osk->osk_text[0] == 0)
			{
				cellOskDialog.warning("on_osk_close: input result is CELL_OSKDIALOG_INPUT_FIELD_RESULT_NO_INPUT_TEXT");
				osk->osk_input_result = CELL_OSKDIALOG_INPUT_FIELD_RESULT_NO_INPUT_TEXT;
			}
			else
			{
				osk->osk_input_result = CELL_OSKDIALOG_INPUT_FIELD_RESULT_OK;
			}
			break;
		}
		case CELL_OSKDIALOG_CLOSE_CANCEL:
		{
			osk->osk_input_result = CELL_OSKDIALOG_INPUT_FIELD_RESULT_CANCELED;
			break;
		}
		case FAKE_CELL_OSKDIALOG_CLOSE_ABORT:
		{
			osk->osk_input_result = CELL_OSKDIALOG_INPUT_FIELD_RESULT_ABORT;
			break;
		}
		default:
		{
			cellOskDialog.fatal("on_osk_close: Unknown status (%d)", status);
			break;
		}
		}

		// Send OSK status
		if (keep_seperate_window_open)
		{
			switch (status)
			{
			case CELL_OSKDIALOG_CLOSE_CONFIRM:
			{
				info.last_dialog_state = CELL_SYSUTIL_OSKDIALOG_INPUT_ENTERED;

				cellOskDialog.notice("on_osk_close: sending CELL_SYSUTIL_OSKDIALOG_INPUT_ENTERED");
				sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_INPUT_ENTERED, 0);
				break;
			}
			case CELL_OSKDIALOG_CLOSE_CANCEL:
			{
				info.last_dialog_state = CELL_SYSUTIL_OSKDIALOG_INPUT_CANCELED;

				cellOskDialog.notice("on_osk_close: sending CELL_SYSUTIL_OSKDIALOG_INPUT_CANCELED");
				sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_INPUT_CANCELED, 0);
				break;
			}
			case FAKE_CELL_OSKDIALOG_CLOSE_ABORT:
			{
				// Handled in cellOskDialogAbort
				break;
			}
			default:
			{
				cellOskDialog.fatal("on_osk_close: Unknown status (%d)", status);
				break;
			}
			}
		}
		else if (status != FAKE_CELL_OSKDIALOG_CLOSE_ABORT) // Handled in cellOskDialogAbort
		{
			info.last_dialog_state = CELL_SYSUTIL_OSKDIALOG_FINISHED;

			cellOskDialog.notice("on_osk_close: sending CELL_SYSUTIL_OSKDIALOG_FINISHED");
			sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_FINISHED, 0);
		}

		// The interception status of the continuous separate window is handled differently
		if (!keep_seperate_window_open)
		{
			input::SetIntercepted(false);
		}
	};

	// Set key callback
	osk->on_osk_key_input_entered = [](CellOskDialogKeyMessage key_message)
	{
		auto& info = g_fxo->get<osk_info>();
		std::lock_guard lock(info.text_mtx);
		auto event_hook_callback = info.osk_hardware_keyboard_event_hook_callback.load();

		cellOskDialog.notice("on_osk_key_input_entered: led=%d, mkey=%d, keycode=%d, hook_event_mode=%d, event_hook_callback=*0x%x", key_message.led, key_message.mkey, key_message.keycode, info.hook_event_mode.load(), event_hook_callback);

		const auto osk = _get_osk_dialog(false);

		if (!osk || !event_hook_callback)
		{
			// Nothing to do here
			return;
		}

		bool is_hook_key = false;

		switch (key_message.keycode)
		{
		case CELL_KEYC_NO_EVENT:
		{
			// Any shift/alt/ctrl key
			is_hook_key = key_message.mkey > 0 && (info.hook_event_mode & CELL_OSKDIALOG_EVENT_HOOK_TYPE_ONLY_MODIFIER);
			break;
		}
		case CELL_KEYC_E_ROLLOVER:
		case CELL_KEYC_E_POSTFAIL:
		case CELL_KEYC_E_UNDEF:
		case CELL_KEYC_ESCAPE:
		case CELL_KEYC_106_KANJI:
		case CELL_KEYC_CAPS_LOCK:
		case CELL_KEYC_F1:
		case CELL_KEYC_F2:
		case CELL_KEYC_F3:
		case CELL_KEYC_F4:
		case CELL_KEYC_F5:
		case CELL_KEYC_F6:
		case CELL_KEYC_F7:
		case CELL_KEYC_F8:
		case CELL_KEYC_F9:
		case CELL_KEYC_F10:
		case CELL_KEYC_F11:
		case CELL_KEYC_F12:
		case CELL_KEYC_PRINTSCREEN:
		case CELL_KEYC_SCROLL_LOCK:
		case CELL_KEYC_PAUSE:
		case CELL_KEYC_INSERT:
		case CELL_KEYC_HOME:
		case CELL_KEYC_PAGE_UP:
		case CELL_KEYC_DELETE:
		case CELL_KEYC_END:
		case CELL_KEYC_PAGE_DOWN:
		case CELL_KEYC_RIGHT_ARROW:
		case CELL_KEYC_LEFT_ARROW:
		case CELL_KEYC_DOWN_ARROW:
		case CELL_KEYC_UP_ARROW:
		case CELL_KEYC_NUM_LOCK:
		case CELL_KEYC_APPLICATION:
		case CELL_KEYC_KANA:
		case CELL_KEYC_HENKAN:
		case CELL_KEYC_MUHENKAN:
		{
			// Any function key or other special key like Delete
			is_hook_key = (info.hook_event_mode & CELL_OSKDIALOG_EVENT_HOOK_TYPE_FUNCTION_KEY);
			break;
		}
		default:
		{
			// Any regular ascii key
			is_hook_key = (info.hook_event_mode & CELL_OSKDIALOG_EVENT_HOOK_TYPE_ASCII_KEY);
			break;
		}
		}

		if (!is_hook_key)
		{
			cellOskDialog.notice("on_osk_key_input_entered: not a hook key: led=%d, mkey=%d, keycode=%d, hook_event_mode=%d", key_message.led, key_message.mkey, key_message.keycode, info.hook_event_mode.load());
			return;
		}

		// The max size is 100 characters plus '\0'. Apparently this used to be 30 plus '\0' in older firmware.
		constexpr u32 max_size = 101;

		// TODO: Send unconfirmed string if there is one.
		// As far as I understand, this is for example supposed to be the IME preview.
		// So when you type in japanese, you get some word propositions which you can select and confirm.
		// The "confirmed" string is basically everything you already wrote, while the "unconfirmed"
		// string is the auto-completion part that you haven't accepted as proper word yet.
		// The game expects you to send this "preview", or an empty string if there is none.
		std::array<u16, max_size> string_to_send{};

		sysutil_register_cb([key_message, string_to_send, event_hook_callback](ppu_thread& cb_ppu) -> s32
		{
			// Prepare callback variables
			vm::var<CellOskDialogKeyMessage> keyMessage(key_message);
			vm::var<u32> action(CELL_OSKDIALOG_CHANGE_NO_EVENT);
			vm::var<u16[], vm::page_allocator<>> pActionInfo(::narrow<u32>(string_to_send.size()), string_to_send.data());

			// Create helpers for logging
			std::u16string utf16_string(reinterpret_cast<const char16_t*>(string_to_send.data()), string_to_send.size());
			std::string utf8_string = utf16_to_ascii8(utf16_string);

			cellOskDialog.notice("osk_hardware_keyboard_event_hook_callback(led=%d, mkey=%d, keycode=%d, action=%d, pActionInfo='%s')", keyMessage->led, keyMessage->mkey, keyMessage->keycode, *action, utf8_string);

			// Call the hook function. The game reads and writes pActionInfo. We need to react based on the returned action.
			const bool return_value = event_hook_callback(cb_ppu, keyMessage, action, pActionInfo);
			ensure(action);
			ensure(pActionInfo);

			// Parse returned text for logging
			utf16_string.clear();
			for (u32 i = 0; i < max_size; i++)
			{
				const u16 code = pActionInfo[i];
				if (!code) break;
				utf16_string.push_back(code);
			}
			utf8_string = utf16_to_ascii8(utf16_string);

			cellOskDialog.notice("osk_hardware_keyboard_event_hook_callback: return_value=%d, action=%d, pActionInfo='%s'", return_value, *action, utf8_string);

			// Check if the hook function was successful
			if (return_value)
			{
				const auto osk = _get_osk_dialog(false);
				if (!osk)
				{
					cellOskDialog.error("osk_hardware_keyboard_event_hook_callback: osk is null");
					return 0;
				}

				auto& info = g_fxo->get<osk_info>();
				std::lock_guard lock(info.text_mtx);

				switch (*action)
				{
				case CELL_OSKDIALOG_CHANGE_NO_EVENT:
				case CELL_OSKDIALOG_CHANGE_EVENT_CANCEL:
				{
					// Do nothing
					break;
				}
				case CELL_OSKDIALOG_CHANGE_WORDS_INPUT:
				{
					// TODO: Replace unconfirmed string.
					cellOskDialog.todo("osk_hardware_keyboard_event_hook_callback: replace unconfirmed string with '%s'", utf8_string);
					break;
				}
				case CELL_OSKDIALOG_CHANGE_WORDS_INSERT:
				{
					// TODO: Remove unconfirmed string
					cellOskDialog.todo("osk_hardware_keyboard_event_hook_callback: remove unconfirmed string");

					// Set confirmed string and reset unconfirmed string
					cellOskDialog.notice("osk_hardware_keyboard_event_hook_callback: inserting string '%s'", utf8_string);
					osk->Insert(utf16_string);
					break;
				}
				case CELL_OSKDIALOG_CHANGE_WORDS_REPLACE_ALL:
				{
					// Replace confirmed string and remove unconfirmed string.
					cellOskDialog.notice("osk_hardware_keyboard_event_hook_callback: replacing all strings with '%s'", utf8_string);
					osk->SetText(utf16_string);
					break;
				}
				default:
				{
					cellOskDialog.error("osk_hardware_keyboard_event_hook_callback returned invalid action (%d)", *action);
					break;
				}
				}
			}

			return 0;
		});
	};

	// Set device mask and event lock
	osk->ignore_device_events = info.lock_ext_input_device.load();
	osk->input_device = info.initial_input_device.load();
	osk->continuous_mode = info.osk_continuous_mode.load();

	if (info.use_separate_windows)
	{
		osk->pad_input_enabled = (info.device_mask != CELL_OSKDIALOG_DEVICE_MASK_PAD);
		osk->mouse_input_enabled = (info.device_mask != CELL_OSKDIALOG_DEVICE_MASK_PAD);
	}

	input::SetIntercepted(osk->pad_input_enabled, osk->keyboard_input_enabled, osk->mouse_input_enabled);

	cellOskDialog.notice("cellOskDialogLoadAsync: creating OSK dialog ...");

	Emu.BlockingCallFromMainThread([=, &info]()
	{
		osk->Create({
			.title = get_localized_string(localized_string_id::CELL_OSK_DIALOG_TITLE),
			.message = message,
			.init_text = osk->osk_text.data(),
			.charlimit = maxLength,
			.prohibit_flags = prohibitFlgs,
			.panel_flag = allowOskPanelFlg,
			.support_language = info.supported_languages,
			.first_view_panel = firstViewPanel,
			.layout = info.layout,
			.input_layout = info.input_field_layout_info,
			.panel_layout = info.input_panel_layout_info,
			.input_field_window_width = info.input_field_window_width,
			.input_field_background_transparency = info.input_field_background_transparency,
			.initial_scale = info.initial_scale,
			.base_color = info.base_color,
			.dimmer_enabled = info.dimmer_enabled,
			.use_separate_windows = info.use_separate_windows,
			.intercept_input = false // We handle the interception manually based on the device mask
		});
	});

	g_fxo->get<osk_info>().last_dialog_state = CELL_SYSUTIL_OSKDIALOG_LOADED;

	if (info.use_separate_windows)
	{
		const bool visible = osk->continuous_mode != CELL_OSKDIALOG_CONTINUOUS_MODE_HIDE;
		cellOskDialog.notice("cellOskDialogLoadAsync: sending CELL_SYSUTIL_OSKDIALOG_DISPLAY_CHANGED with %s", visible ? "CELL_OSKDIALOG_DISPLAY_STATUS_SHOW" : "CELL_OSKDIALOG_DISPLAY_STATUS_HIDE");
		sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_DISPLAY_CHANGED, visible ? CELL_OSKDIALOG_DISPLAY_STATUS_SHOW : CELL_OSKDIALOG_DISPLAY_STATUS_HIDE);
	}

	cellOskDialog.notice("cellOskDialogLoadAsync: sending CELL_SYSUTIL_OSKDIALOG_LOADED");
	sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_LOADED, 0);

	cellOskDialog.notice("cellOskDialogLoadAsync: created OSK dialog");
	return CELL_OK;
}

error_code cellOskDialogLoadAsyncExt()
{
	cellOskDialog.todo("cellOskDialogLoadAsyncExt()");
	return CELL_OK;
}

error_code getText(vm::ptr<CellOskDialogCallbackReturnParam> OutputInfo, bool is_unload)
{
	if (!OutputInfo || OutputInfo->numCharsResultString < 0)
	{
		return CELL_OSKDIALOG_ERROR_PARAM;
	}

	const auto osk = _get_osk_dialog(false);

	if (!osk)
	{
		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	auto& info = g_fxo->get<osk_info>();

	const bool keep_seperate_window_open = info.use_separate_windows.load() && (info.osk_continuous_mode.load() != CELL_OSKDIALOG_CONTINUOUS_MODE_NONE);

	info.text_mtx.lock();

	// Update text buffer if called from cellOskDialogUnloadAsync or if the user accepted the dialog during continuous seperate window mode.
	if (is_unload || (keep_seperate_window_open && info.last_dialog_state == CELL_SYSUTIL_OSKDIALOG_INPUT_ENTERED))
	{
		for (s32 i = 0; i < CELL_OSKDIALOG_STRING_SIZE - 1; i++)
		{
			info.valid_text[i] = osk->osk_text[i];
		}
	}

	if (is_unload)
	{
		OutputInfo->result = osk->osk_input_result.load();
	}
	else
	{
		if (info.valid_text[0] == 0)
		{
			OutputInfo->result = CELL_OSKDIALOG_INPUT_FIELD_RESULT_NO_INPUT_TEXT;
		}
		else
		{
			OutputInfo->result = CELL_OSKDIALOG_INPUT_FIELD_RESULT_OK;
		}
	}

	const bool do_copy = OutputInfo->pResultString && (OutputInfo->result == CELL_OSKDIALOG_INPUT_FIELD_RESULT_OK || (is_unload && OutputInfo->result == CELL_OSKDIALOG_INPUT_FIELD_RESULT_NO_INPUT_TEXT));

	for (s32 i = 0; do_copy && i < CELL_OSKDIALOG_STRING_SIZE - 1; i++)
	{
		if (i < OutputInfo->numCharsResultString)
		{
			OutputInfo->pResultString[i] = info.valid_text[i];

			if (info.valid_text[i] == 0)
			{
				break;
			}
		}
		else
		{
			OutputInfo->pResultString[i] = 0;
			break;
		}
	}

	info.text_mtx.unlock();

	if (is_unload)
	{
		// Unload should be called last, so remove the dialog here
		if (const auto reset_lock = info.init.reset())
		{
			info.reset();
		}

		if (keep_seperate_window_open)
		{
			cellOskDialog.notice("cellOskDialogUnloadAsync: terminating continuous overlay");
			osk->Close(FAKE_CELL_OSKDIALOG_CLOSE_TERMINATE);
		}

		osk->state = OskDialogState::Unloaded;

		cellOskDialog.notice("cellOskDialogUnloadAsync: sending CELL_SYSUTIL_OSKDIALOG_UNLOADED");
		sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_UNLOADED, 0);
	}
	else if (keep_seperate_window_open)
	{
		// Clear text buffer unless the dialog is still open during continuous seperate window mode.
		switch (info.last_dialog_state)
		{
		case CELL_SYSUTIL_OSKDIALOG_FINISHED:
		case CELL_SYSUTIL_OSKDIALOG_UNLOADED:
		case CELL_SYSUTIL_OSKDIALOG_INPUT_CANCELED:
		case CELL_SYSUTIL_OSKDIALOG_INPUT_ENTERED:
		{
			auto& info = g_fxo->get<osk_info>();
			std::lock_guard lock(info.text_mtx);

			info.valid_text = {};
			osk->Clear(true);
			break;
		}
		default:
			break;
		}
	}

	return CELL_OK;
}

error_code cellOskDialogUnloadAsync(vm::ptr<CellOskDialogCallbackReturnParam> OutputInfo)
{
	cellOskDialog.warning("cellOskDialogUnloadAsync(OutputInfo=*0x%x)", OutputInfo);
	return getText(OutputInfo, true);
}

error_code cellOskDialogGetSize(vm::ptr<u16> width, vm::ptr<u16> height, u32 /*CellOskDialogType*/ dialogType)
{
	cellOskDialog.warning("cellOskDialogGetSize(width=*0x%x, height=*0x%x, dialogType=%d)", width, height, dialogType);

	if (!width || !height)
	{
		return CELL_OSKDIALOG_ERROR_PARAM;
	}

	if (dialogType >= CELL_OSKDIALOG_TYPE_SEPARATE_SINGLELINE_TEXT_WINDOW)
	{
		*width = 0;
	}
	else
	{
		*width = 1;
	}

	*height = 1;

	return CELL_OK;
}

error_code cellOskDialogAbort()
{
	cellOskDialog.warning("cellOskDialogAbort()");

	const auto osk = _get_osk_dialog(false);

	if (!osk)
	{
		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	const error_code result = osk->state.atomic_op([](OskDialogState& state) -> error_code
	{
		// Check for open dialog. In this case the dialog is "Open" if it was not unloaded before.
		if (state == OskDialogState::Unloaded)
		{
			return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
		}

		state = OskDialogState::Abort;

		return CELL_OK;
	});

	if (result == CELL_OK)
	{
		osk->osk_input_result = CELL_OSKDIALOG_INPUT_FIELD_RESULT_ABORT;
		osk->Close(FAKE_CELL_OSKDIALOG_CLOSE_ABORT);
	}

	g_fxo->get<osk_info>().last_dialog_state = CELL_SYSUTIL_OSKDIALOG_FINISHED;

	cellOskDialog.notice("cellOskDialogAbort: sending CELL_SYSUTIL_OSKDIALOG_FINISHED");
	sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_FINISHED, 0);

	return CELL_OK;
}

error_code cellOskDialogSetDeviceMask(u32 deviceMask)
{
	cellOskDialog.warning("cellOskDialogSetDeviceMask(deviceMask=0x%x)", deviceMask);

	// TODO: It might also return an error if use_separate_windows is not enabled
	if (deviceMask > CELL_OSKDIALOG_DEVICE_MASK_PAD)
	{
		return CELL_OSKDIALOG_ERROR_PARAM;
	}

	auto& info = g_fxo->get<osk_info>();
	info.device_mask = deviceMask;

	if (info.use_separate_windows)
	{
		if (const auto osk = _get_osk_dialog(false))
		{
			osk->pad_input_enabled = (deviceMask != CELL_OSKDIALOG_DEVICE_MASK_PAD);
			osk->mouse_input_enabled = (deviceMask != CELL_OSKDIALOG_DEVICE_MASK_PAD);

			input::SetIntercepted(osk->pad_input_enabled, osk->keyboard_input_enabled, osk->mouse_input_enabled);
		}
	}

	return CELL_OK;
}

error_code cellOskDialogSetSeparateWindowOption(vm::ptr<CellOskDialogSeparateWindowOption> windowOption)
{
	cellOskDialog.warning("cellOskDialogSetSeparateWindowOption(windowOption=*0x%x)", windowOption);

	if (!windowOption ||
		!windowOption->inputFieldLayoutInfo ||
		!!windowOption->reserved ||
		windowOption->continuousMode > CELL_OSKDIALOG_CONTINUOUS_MODE_SHOW ||
		windowOption->deviceMask > CELL_OSKDIALOG_DEVICE_MASK_PAD)
	{
		return CELL_OSKDIALOG_ERROR_PARAM;
	}

	auto& osk = g_fxo->get<osk_info>();
	osk.use_separate_windows = true;
	osk.osk_continuous_mode  = static_cast<CellOskDialogContinuousMode>(+windowOption->continuousMode);
	osk.device_mask = windowOption->deviceMask;
	osk.input_field_window_width = windowOption->inputFieldWindowWidth;
	osk.input_field_background_transparency = std::clamp<f32>(windowOption->inputFieldBackgroundTrans, 0.0f, 1.0f);

	// Choose proper alignments, since the devs didn't make them exclusive for some reason.
	const auto aligned_layout = [](const CellOskDialogLayoutInfo& info) -> osk_window_layout
	{
		osk_window_layout res{};
		res.layout_mode = info.layoutMode;
		res.x_align = osk_info::get_aligned_x(res.layout_mode);
		res.y_align = osk_info::get_aligned_y(res.layout_mode);
		res.x_offset = info.position.x;
		res.y_offset = info.position.y;
		return res;
	};

	osk.input_field_layout_info = aligned_layout(*windowOption->inputFieldLayoutInfo);

	// Panel layout is optional
	if (windowOption->inputPanelLayoutInfo)
	{
		osk.input_panel_layout_info = aligned_layout(*windowOption->inputPanelLayoutInfo);
	}
	else
	{
		// Align to input field
		osk.input_panel_layout_info = osk.input_field_layout_info;
	}

	cellOskDialog.warning("cellOskDialogSetSeparateWindowOption: use_separate_windows=true, continuous_mode=%s, device_mask=0x%x, input_field_window_width=%d, input_field_background_transparency=%.2f, input_field_layout_info=%s, input_panel_layout_info=%s)",
		osk.osk_continuous_mode.load(), osk.device_mask.load(), osk.input_field_window_width.load(), osk.input_field_background_transparency.load(), osk.input_field_layout_info, osk.input_panel_layout_info);

	return CELL_OK;
}

error_code cellOskDialogSetInitialInputDevice(u32 inputDevice)
{
	cellOskDialog.warning("cellOskDialogSetInitialInputDevice(inputDevice=%d)", inputDevice);

	if (inputDevice > CELL_OSKDIALOG_INPUT_DEVICE_KEYBOARD)
	{
		return CELL_OSKDIALOG_ERROR_PARAM;
	}

	g_fxo->get<osk_info>().initial_input_device = static_cast<CellOskDialogInputDevice>(inputDevice);

	return CELL_OK;
}

error_code cellOskDialogSetInitialKeyLayout(u32 initialKeyLayout)
{
	cellOskDialog.warning("cellOskDialogSetInitialKeyLayout(initialKeyLayout=%d)", initialKeyLayout);

	if (initialKeyLayout > CELL_OSKDIALOG_INITIAL_PANEL_LAYOUT_FULLKEY)
	{
		return CELL_OSKDIALOG_ERROR_PARAM;
	}

	auto& osk = g_fxo->get<osk_info>();

	if (osk.key_layout_options & initialKeyLayout)
	{
		osk.initial_key_layout = static_cast<CellOskDialogInitialKeyLayout>(initialKeyLayout);
	}

	return CELL_OK;
}

error_code cellOskDialogDisableDimmer()
{
	cellOskDialog.warning("cellOskDialogDisableDimmer()");

	g_fxo->get<osk_info>().dimmer_enabled = false;

	return CELL_OK;
}

error_code cellOskDialogSetKeyLayoutOption(u32 option)
{
	cellOskDialog.warning("cellOskDialogSetKeyLayoutOption(option=0x%x)", option);

	if (option == 0 || option > 3) // CELL_OSKDIALOG_10KEY_PANEL OR CELL_OSKDIALOG_FULLKEY_PANEL
	{
		return CELL_OSKDIALOG_ERROR_PARAM;
	}

	g_fxo->get<osk_info>().key_layout_options = option;

	return CELL_OK;
}

error_code cellOskDialogAddSupportLanguage(u32 supportLanguage)
{
	cellOskDialog.warning("cellOskDialogAddSupportLanguage(supportLanguage=0x%x)", supportLanguage);

	g_fxo->get<osk_info>().supported_languages = supportLanguage;

	return CELL_OK;
}

error_code cellOskDialogSetLayoutMode(s32 layoutMode)
{
	cellOskDialog.warning("cellOskDialogSetLayoutMode(layoutMode=0x%x)", layoutMode);

	auto& osk = g_fxo->get<osk_info>();
	osk.layout.layout_mode = layoutMode;

	// Choose proper alignments, since the devs didn't make them exclusive for some reason.
	osk.layout.x_align = osk_info::get_aligned_x(layoutMode);
	osk.layout.y_align = osk_info::get_aligned_y(layoutMode);

	return CELL_OK;
}

error_code cellOskDialogGetInputText(vm::ptr<CellOskDialogCallbackReturnParam> OutputInfo)
{
	cellOskDialog.warning("cellOskDialogGetInputText(OutputInfo=*0x%x)", OutputInfo);
	return getText(OutputInfo, false);
}

error_code register_keyboard_event_hook_callback(u16 hookEventMode, vm::ptr<cellOskDialogHardwareKeyboardEventHookCallback> pCallback)
{
	cellOskDialog.warning("register_keyboard_event_hook_callback(hookEventMode=%u, pCallback=*0x%x)", hookEventMode, pCallback);

	if (!pCallback)
	{
		return CELL_OSKDIALOG_ERROR_PARAM;
	}

	g_fxo->get<osk_info>().osk_hardware_keyboard_event_hook_callback = pCallback;
	g_fxo->get<osk_info>().hook_event_mode = hookEventMode;

	return CELL_OK;
}

error_code cellOskDialogExtRegisterKeyboardEventHookCallback(u16 hookEventMode, vm::ptr<cellOskDialogHardwareKeyboardEventHookCallback> pCallback)
{
	cellOskDialog.warning("cellOskDialogExtRegisterKeyboardEventHookCallback(hookEventMode=%u, pCallback=*0x%x)", hookEventMode, pCallback);

	if (hookEventMode == 0 || hookEventMode > (CELL_OSKDIALOG_EVENT_HOOK_TYPE_FUNCTION_KEY | CELL_OSKDIALOG_EVENT_HOOK_TYPE_ASCII_KEY))
	{
		return CELL_OSKDIALOG_ERROR_PARAM;
	}

	return register_keyboard_event_hook_callback(hookEventMode, pCallback);
}

error_code cellOskDialogExtRegisterKeyboardEventHookCallbackEx(u16 hookEventMode, vm::ptr<cellOskDialogHardwareKeyboardEventHookCallback> pCallback)
{
	cellOskDialog.warning("cellOskDialogExtRegisterKeyboardEventHookCallbackEx(hookEventMode=%u, pCallback=*0x%x)", hookEventMode, pCallback);

	if (hookEventMode == 0 || hookEventMode > (CELL_OSKDIALOG_EVENT_HOOK_TYPE_FUNCTION_KEY | CELL_OSKDIALOG_EVENT_HOOK_TYPE_ASCII_KEY | CELL_OSKDIALOG_EVENT_HOOK_TYPE_ONLY_MODIFIER))
	{
		return CELL_OSKDIALOG_ERROR_PARAM;
	}

	return register_keyboard_event_hook_callback(hookEventMode, pCallback);
}

error_code cellOskDialogExtAddJapaneseOptionDictionary(vm::cpptr<char> filePath)
{
	cellOskDialog.todo("cellOskDialogExtAddJapaneseOptionDictionary(filePath=**0x%0x)", filePath);

	std::vector<std::string> paths;

	if (filePath)
	{
		for (u32 i = 0; i < 4; i++)
		{
			if (!filePath[i])
			{
				break;
			}

			std::array<char, CELL_IMEJP_DIC_PATH_MAXLENGTH + 1> path{};
			std::memcpy(path.data(), filePath[i].get_ptr(), CELL_IMEJP_DIC_PATH_MAXLENGTH);
			paths.push_back(path.data());
		}
	}

	cellOskDialog.todo("cellOskDialogExtAddJapaneseOptionDictionary: got %d dictionaries:\n%s", paths.size(), fmt::merge(paths, "\n"));

	return CELL_OK;
}

error_code cellOskDialogExtEnableClipboard()
{
	cellOskDialog.todo("cellOskDialogExtEnableClipboard()");

	g_fxo->get<osk_info>().clipboard_enabled = true;

	// TODO: implement copy paste

	return CELL_OK;
}

error_code cellOskDialogExtSendFinishMessage(u32 /*CellOskDialogFinishReason*/ finishReason)
{
	cellOskDialog.warning("cellOskDialogExtSendFinishMessage(finishReason=%d)", finishReason);

	const auto osk = _get_osk_dialog(false);

	// Check for "Open" dialog.
	if (!osk || osk->state.load() == OskDialogState::Unloaded)
	{
		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	osk->Close(finishReason);

	return CELL_OK;
}

error_code cellOskDialogExtAddOptionDictionary(vm::cpptr<CellOskDialogImeDictionaryInfo> dictionaryInfo)
{
	cellOskDialog.todo("cellOskDialogExtAddOptionDictionary(dictionaryInfo=*0x%x)", dictionaryInfo);

	if (!dictionaryInfo)
	{
		return CELL_OSKDIALOG_ERROR_PARAM;
	}

	std::vector<std::pair<u32, std::string>> paths; // language and path

	for (u32 i = 0; i < 10; i++)
	{
		if (!dictionaryInfo[i] || !dictionaryInfo[i]->dictionaryPath)
		{
			break;
		}

		std::array<char, CELL_IMEJP_DIC_PATH_MAXLENGTH + 1> path{};
		std::memcpy(path.data(), dictionaryInfo[i]->dictionaryPath.get_ptr(), CELL_IMEJP_DIC_PATH_MAXLENGTH);
		paths.push_back({ dictionaryInfo[i]->targetLanguage, path.data() });
	}

	std::vector<std::string> msgs;
	for (const auto& entry : paths)
	{
		msgs.push_back(fmt::format("languages=0x%x, path='%s'", entry.first, entry.second));
	}

	cellOskDialog.todo("cellOskDialogExtAddOptionDictionary: got %d dictionaries:\n%s", msgs.size(), fmt::merge(msgs, "\n"));

	return CELL_OK;
}

error_code cellOskDialogExtSetInitialScale(f32 initialScale)
{
	cellOskDialog.warning("cellOskDialogExtSetInitialScale(initialScale=%f)", initialScale);

	if (initialScale < CELL_OSKDIALOG_SCALE_MIN || initialScale > CELL_OSKDIALOG_SCALE_MAX)
	{
		return CELL_OSKDIALOG_ERROR_PARAM;
	}

	g_fxo->get<osk_info>().initial_scale = initialScale;

	return CELL_OK;
}

error_code cellOskDialogExtInputDeviceLock()
{
	cellOskDialog.warning("cellOskDialogExtInputDeviceLock()");

	g_fxo->get<osk_info>().lock_ext_input_device = true;

	if (const auto osk = _get_osk_dialog(false))
	{
		osk->ignore_device_events = true;
	}

	return CELL_OK;
}

error_code cellOskDialogExtInputDeviceUnlock()
{
	cellOskDialog.warning("cellOskDialogExtInputDeviceUnlock()");

	g_fxo->get<osk_info>().lock_ext_input_device = false;

	if (const auto osk = _get_osk_dialog(false))
	{
		osk->ignore_device_events = false;
	}

	return CELL_OK;
}

error_code cellOskDialogExtSetBaseColor(f32 red, f32 green, f32 blue, f32 alpha)
{
	cellOskDialog.warning("cellOskDialogExtSetBaseColor(red=%f, blue=%f, green=%f, alpha=%f)", red, blue, green, alpha);

	if (red < 0.0f || red > 1.0f || green < 0.0f || green > 1.0f || blue < 0.0f || blue > 1.0f || alpha < 0.0f || alpha > 1.0f)
	{
		return CELL_OSKDIALOG_ERROR_PARAM;
	}

	auto& osk = g_fxo->get<osk_info>();
	osk.base_color = OskDialogBase::color{ red, green, blue, alpha };

	return CELL_OK;
}

error_code cellOskDialogExtRegisterConfirmWordFilterCallback(vm::ptr<cellOskDialogConfirmWordFilterCallback> pCallback)
{
	cellOskDialog.warning("cellOskDialogExtRegisterConfirmWordFilterCallback(pCallback=*0x%x)", pCallback);

	if (!pCallback)
	{
		return CELL_OSKDIALOG_ERROR_PARAM;
	}

	g_fxo->get<osk_info>().osk_confirm_callback = pCallback;

	return CELL_OK;
}

error_code cellOskDialogExtUpdateInputText()
{
	cellOskDialog.todo("cellOskDialogExtUpdateInputText()");

	// Usually, user input is only available when the dialog was accepted.
	// This function seems to be called in order to copy the current text to an internal buffer.
	// Afterwards, cellOskDialogGetInputText can be called to fetch the current text regardless of
	// user confirmation, even if the dialog is still in use.

	// TODO: error checks

	const auto osk = _get_osk_dialog(false);

	if (osk)
	{
		auto& info = g_fxo->get<osk_info>();
		std::lock_guard lock(info.text_mtx);

		for (s32 i = 0; i < CELL_OSKDIALOG_STRING_SIZE - 1; i++)
		{
			info.valid_text[i] = osk->osk_text[i];
		}
	}

	return CELL_OK;
}

error_code cellOskDialogExtSetPointerEnable(b8 enable)
{
	cellOskDialog.warning("cellOskDialogExtSetPointerEnable(enable=%d)", enable);

	// TODO: While the pointer is already displayed in the osk overlay, it is not really useful right now.
	//       On real hardware, this may be used for actual PS Move or mouse input.

	g_fxo->get<osk_info>().pointer_enabled = enable;

	return CELL_OK;
}

error_code cellOskDialogExtUpdatePointerDisplayPos(vm::cptr<CellOskDialogPoint> pos)
{
	cellOskDialog.warning("cellOskDialogExtUpdatePointerDisplayPos(pos=0x%x, posX=%f, posY=%f)", pos, pos->x, pos->y);

	if (pos)
	{
		osk_info& osk = g_fxo->get<osk_info>();
		osk.pointer_x = pos->x;
		osk.pointer_y = pos->y;
	}

	return CELL_OK;
}

error_code cellOskDialogExtEnableHalfByteKana()
{
	cellOskDialog.todo("cellOskDialogExtEnableHalfByteKana()");

	g_fxo->get<osk_info>().half_byte_kana_enabled = true;

	// TODO: use new value in osk

	return CELL_OK;
}

error_code cellOskDialogExtDisableHalfByteKana()
{
	cellOskDialog.todo("cellOskDialogExtDisableHalfByteKana()");

	g_fxo->get<osk_info>().half_byte_kana_enabled = false;

	// TODO: use new value in osk

	return CELL_OK;
}

error_code cellOskDialogExtRegisterForceFinishCallback(vm::ptr<cellOskDialogForceFinishCallback> pCallback)
{
	cellOskDialog.warning("cellOskDialogExtRegisterForceFinishCallback(pCallback=*0x%x)", pCallback);

	if (!pCallback)
	{
		return CELL_OSKDIALOG_ERROR_PARAM;
	}

	osk_info& info = g_fxo->get<osk_info>();
	std::lock_guard lock(info.text_mtx);
	info.osk_force_finish_callback = pCallback;

	// We use the force finish callback when the PS-Button is pressed and a System dialog shall be spawned while the OSK is loaded
	// 1. Check if we are in any continuous mode and the dialog is hidden
	// 2. If the above is true, call osk_force_finish_callback, deny the PS-Button press otherwise
	// 3. Check the return value of osk_force_finish_callback.
	//    if false, ignore the PS-Button press,
	//    else close the dialog etc., send CELL_SYSUTIL_OSKDIALOG_FINISHED

	return CELL_OK;
}


void cellSysutil_OskDialog_init()
{
	REG_FUNC(cellSysutil, cellOskDialogLoadAsync);
	REG_FUNC(cellSysutil, cellOskDialogLoadAsyncExt);
	REG_FUNC(cellSysutil, cellOskDialogUnloadAsync);
	REG_FUNC(cellSysutil, cellOskDialogGetSize);
	REG_FUNC(cellSysutil, cellOskDialogAbort);
	REG_FUNC(cellSysutil, cellOskDialogSetDeviceMask);
	REG_FUNC(cellSysutil, cellOskDialogSetSeparateWindowOption);
	REG_FUNC(cellSysutil, cellOskDialogSetInitialInputDevice);
	REG_FUNC(cellSysutil, cellOskDialogSetInitialKeyLayout);
	REG_FUNC(cellSysutil, cellOskDialogDisableDimmer);
	REG_FUNC(cellSysutil, cellOskDialogSetKeyLayoutOption);
	REG_FUNC(cellSysutil, cellOskDialogAddSupportLanguage);
	REG_FUNC(cellSysutil, cellOskDialogSetLayoutMode);
	REG_FUNC(cellSysutil, cellOskDialogGetInputText);
}

DECLARE(ppu_module_manager::cellOskDialog)("cellOskExtUtility", []()
{
	REG_FUNC(cellOskExtUtility, cellOskDialogExtInputDeviceUnlock);
	REG_FUNC(cellOskExtUtility, cellOskDialogExtRegisterKeyboardEventHookCallback);
	REG_FUNC(cellOskExtUtility, cellOskDialogExtRegisterKeyboardEventHookCallbackEx);
	REG_FUNC(cellOskExtUtility, cellOskDialogExtAddJapaneseOptionDictionary);
	REG_FUNC(cellOskExtUtility, cellOskDialogExtEnableClipboard);
	REG_FUNC(cellOskExtUtility, cellOskDialogExtSendFinishMessage);
	REG_FUNC(cellOskExtUtility, cellOskDialogExtAddOptionDictionary);
	REG_FUNC(cellOskExtUtility, cellOskDialogExtSetInitialScale);
	REG_FUNC(cellOskExtUtility, cellOskDialogExtInputDeviceLock);
	REG_FUNC(cellOskExtUtility, cellOskDialogExtSetBaseColor);
	REG_FUNC(cellOskExtUtility, cellOskDialogExtRegisterConfirmWordFilterCallback);
	REG_FUNC(cellOskExtUtility, cellOskDialogExtUpdateInputText);
	REG_FUNC(cellOskExtUtility, cellOskDialogExtDisableHalfByteKana);
	REG_FUNC(cellOskExtUtility, cellOskDialogExtSetPointerEnable);
	REG_FUNC(cellOskExtUtility, cellOskDialogExtUpdatePointerDisplayPos);
	REG_FUNC(cellOskExtUtility, cellOskDialogExtEnableHalfByteKana);
	REG_FUNC(cellOskExtUtility, cellOskDialogExtRegisterForceFinishCallback);
});
