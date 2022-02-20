#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Io/interception.h"
#include "Emu/RSX/Overlays/overlay_osk.h"
#include "Emu/IdManager.h"

#include "cellSysutil.h"
#include "cellOskDialog.h"
#include "cellMsgDialog.h"

#include "util/init_mutex.hpp"

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

OskDialogBase::~OskDialogBase()
{
}

struct osk_info
{
	std::shared_ptr<OskDialogBase> dlg;

	std::array<char16_t, CELL_OSKDIALOG_STRING_SIZE> valid_text{};
	shared_mutex text_mtx;

	atomic_t<bool> use_separate_windows = false;

	atomic_t<bool> lock_ext_input = false;
	atomic_t<u32> device_mask = 0; // 0 means all devices can influence the OSK
	atomic_t<u32> key_layout = CELL_OSKDIALOG_10KEY_PANEL;
	atomic_t<CellOskDialogInitialKeyLayout> initial_key_layout = CELL_OSKDIALOG_INITIAL_PANEL_LAYOUT_SYSTEM;
	atomic_t<CellOskDialogInputDevice> initial_input_device = CELL_OSKDIALOG_INPUT_DEVICE_PAD;

	atomic_t<bool> clipboard_enabled = false; // For copy and paste
	atomic_t<bool> half_byte_kana_enabled = false;
	atomic_t<u32> supported_languages = 0; // Used to enable non-default languages in the OSK

	atomic_t<bool> dimmer_enabled = true;
	atomic_t<f32> base_color_red = 1.0f;
	atomic_t<f32> base_color_blue = 1.0f;
	atomic_t<f32> base_color_green = 1.0f;
	atomic_t<f32> base_color_alpha = 1.0f;

	atomic_t<bool> pointer_enabled = false;
	CellOskDialogPoint pointer_pos{0.0f, 0.0f};
	atomic_t<f32> initial_scale = 1.0f;

	atomic_t<u32> layout_mode = CELL_OSKDIALOG_LAYOUTMODE_X_ALIGN_LEFT | CELL_OSKDIALOG_LAYOUTMODE_Y_ALIGN_TOP;

	atomic_t<CellOskDialogContinuousMode> osk_continuous_mode = CELL_OSKDIALOG_CONTINUOUS_MODE_NONE;
	atomic_t<u32> last_dialog_state = CELL_SYSUTIL_OSKDIALOG_UNLOADED; // Used for continuous seperate window dialog

	atomic_t<vm::ptr<cellOskDialogConfirmWordFilterCallback>> osk_confirm_callback{};

	stx::init_mutex init;

	void reset()
	{
		std::lock_guard lock(text_mtx);

		dlg.reset();
		valid_text = {};
		use_separate_windows = false;
		lock_ext_input = false;
		device_mask = 0;
		key_layout = CELL_OSKDIALOG_10KEY_PANEL;
		initial_key_layout = CELL_OSKDIALOG_INITIAL_PANEL_LAYOUT_SYSTEM;
		initial_input_device = CELL_OSKDIALOG_INPUT_DEVICE_PAD;
		clipboard_enabled = false;
		half_byte_kana_enabled = false;
		supported_languages = 0;
		dimmer_enabled = true;
		base_color_red = 1.0f;
		base_color_blue = 1.0f;
		base_color_green = 1.0f;
		base_color_alpha = 1.0f;
		pointer_enabled = false;
		pointer_pos = {0.0f, 0.0f};
		initial_scale = 1.0f;
		layout_mode = CELL_OSKDIALOG_LAYOUTMODE_X_ALIGN_LEFT | CELL_OSKDIALOG_LAYOUTMODE_Y_ALIGN_TOP;
		osk_continuous_mode = CELL_OSKDIALOG_CONTINUOUS_MODE_NONE;
		last_dialog_state = CELL_SYSUTIL_OSKDIALOG_UNLOADED;
		osk_confirm_callback.store({});
	}
};

// TODO: don't use this function
std::shared_ptr<OskDialogBase> _get_osk_dialog(bool create = false)
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

error_code cellOskDialogLoadAsync(u32 container, vm::ptr<CellOskDialogParam> dialogParam, vm::ptr<CellOskDialogInputFieldInfo> inputFieldInfo)
{
	cellOskDialog.warning("cellOskDialogLoadAsync(container=0x%x, dialogParam=*0x%x, inputFieldInfo=*0x%x)", container, dialogParam, inputFieldInfo);

	if (!dialogParam || !inputFieldInfo || !inputFieldInfo->message || !inputFieldInfo->init_text || inputFieldInfo->limit_length > CELL_OSKDIALOG_STRING_SIZE)
	{
		return CELL_OSKDIALOG_ERROR_PARAM;
	}

	auto osk = _get_osk_dialog(true);

	// Can't open another dialog if this one is already open.
	if (!osk || osk->state.load() != OskDialogState::Unloaded)
	{
		return CELL_SYSUTIL_ERROR_BUSY;
	}

	// Get the OSK options
	u32 maxLength = (inputFieldInfo->limit_length >= CELL_OSKDIALOG_STRING_SIZE) ? 511 : u32{inputFieldInfo->limit_length};
	const u32 prohibitFlgs = dialogParam->prohibitFlgs;
	const u32 allowOskPanelFlg = dialogParam->allowOskPanelFlg;
	const u32 firstViewPanel = dialogParam->firstViewPanel;

	// Get init text and prepare return value
	osk->osk_input_result = CELL_OSKDIALOG_INPUT_FIELD_RESULT_OK;
	std::memset(osk->osk_text, 0, sizeof(osk->osk_text));

	// Also clear the info text just to be sure (it should be zeroed at this point anyway)
	{
		auto& info = g_fxo->get<osk_info>();
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

	atomic_t<bool> result = false;

	osk->on_osk_close = [wptr = std::weak_ptr<OskDialogBase>(osk)](s32 status)
	{
		cellOskDialog.error("on_osk_close(status=%d)", status);

		const auto osk = wptr.lock();
		osk->state = OskDialogState::Closed;

		auto& info = g_fxo->get<osk_info>();

		const bool keep_seperate_window_open = info.use_separate_windows.load() && (info.osk_continuous_mode.load() != CELL_OSKDIALOG_CONTINUOUS_MODE_NONE);

		switch (status)
		{
		case CELL_OSKDIALOG_CLOSE_CONFIRM:
		{
			if (auto ccb = info.osk_confirm_callback.exchange({}))
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
				cellOskDialog.warning("cellOskDialogLoadAsync: input result is CELL_OSKDIALOG_INPUT_FIELD_RESULT_NO_INPUT_TEXT");
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
				sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_INPUT_ENTERED, 0);
				break;
			}
			case CELL_OSKDIALOG_CLOSE_CANCEL:
			{
				info.last_dialog_state = CELL_SYSUTIL_OSKDIALOG_INPUT_CANCELED;
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

			if (info.osk_continuous_mode.load() == CELL_OSKDIALOG_CONTINUOUS_MODE_HIDE)
			{
				sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_DISPLAY_CHANGED, CELL_OSKDIALOG_DISPLAY_STATUS_HIDE);
			}
		}
		else if (status != FAKE_CELL_OSKDIALOG_CLOSE_ABORT) // Handled in cellOskDialogAbort
		{
			info.last_dialog_state = CELL_SYSUTIL_OSKDIALOG_FINISHED;
			sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_FINISHED, 0);
		}

		input::SetIntercepted(false);
	};

	if (auto& info = g_fxo->get<osk_info>(); info.osk_continuous_mode == CELL_OSKDIALOG_CONTINUOUS_MODE_HIDE)
	{
		info.last_dialog_state = CELL_SYSUTIL_OSKDIALOG_LOADED;
		sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_LOADED, 0);
		return CELL_OK;
	}

	input::SetIntercepted(true);

	Emu.CallFromMainThread([=, &result]()
	{
		osk->Create(get_localized_string(localized_string_id::CELL_OSK_DIALOG_TITLE), message, osk->osk_text, maxLength, prohibitFlgs, allowOskPanelFlg, firstViewPanel);
		result = true;
		result.notify_one();

		if (g_fxo->get<osk_info>().osk_continuous_mode.load() == CELL_OSKDIALOG_CONTINUOUS_MODE_HIDE)
		{
			sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_DISPLAY_CHANGED, CELL_OSKDIALOG_DISPLAY_STATUS_SHOW);
		}
	});

	g_fxo->get<osk_info>().last_dialog_state = CELL_SYSUTIL_OSKDIALOG_LOADED;
	sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_LOADED, 0);

	while (!result && !Emu.IsStopped())
	{
		thread_ctrl::wait_on(result, false);
	}

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

	const auto osk = _get_osk_dialog();

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

		osk->state = OskDialogState::Unloaded;
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
			info.valid_text = {};
			break;
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

	const auto osk = _get_osk_dialog();

	if (!osk)
	{
		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	const s32 result = osk->state.atomic_op([](OskDialogState& state)
	{
		// Check for open dialog. In this case the dialog is "Open" if it was not unloaded before.
		if (state == OskDialogState::Unloaded)
		{
			return static_cast<s32>(CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED);
		}

		state = OskDialogState::Abort;

		return static_cast<s32>(CELL_OK);
	});

	if (result != CELL_OK)
	{
		return result;
	}

	osk->osk_input_result = CELL_OSKDIALOG_INPUT_FIELD_RESULT_ABORT;
	osk->Close(FAKE_CELL_OSKDIALOG_CLOSE_ABORT);

	g_fxo->get<osk_info>().last_dialog_state = CELL_SYSUTIL_OSKDIALOG_FINISHED;
	sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_FINISHED, 0);

	return CELL_OK;
}

error_code cellOskDialogSetDeviceMask(u32 deviceMask)
{
	cellOskDialog.todo("cellOskDialogSetDeviceMask(deviceMask=0x%x)", deviceMask);

	// TODO: error checks. It probably checks for use_separate_windows

	g_fxo->get<osk_info>().device_mask = deviceMask;

	// TODO: change osk device input

	return CELL_OK;
}

error_code cellOskDialogSetSeparateWindowOption(vm::ptr<CellOskDialogSeparateWindowOption> windowOption)
{
	cellOskDialog.todo("cellOskDialogSetSeparateWindowOption(windowOption=*0x%x)", windowOption);

	if (!windowOption)
	{
		return CELL_OSKDIALOG_ERROR_PARAM;
	}

	auto& osk = g_fxo->get<osk_info>();
	osk.use_separate_windows = true;
	osk.osk_continuous_mode  = static_cast<CellOskDialogContinuousMode>(+windowOption->continuousMode);
	// TODO: handle rest of windowOption

	cellOskDialog.warning("cellOskDialogSetSeparateWindowOption: continuousMode=%s)", osk.osk_continuous_mode.load());

	return CELL_OK;
}

error_code cellOskDialogSetInitialInputDevice(u32 inputDevice)
{
	cellOskDialog.todo("cellOskDialogSetInitialInputDevice(inputDevice=%d)", inputDevice);

	// TODO: error checks

	g_fxo->get<osk_info>().initial_input_device = static_cast<CellOskDialogInputDevice>(inputDevice);

	// TODO: use value

	return CELL_OK;
}

error_code cellOskDialogSetInitialKeyLayout(u32 initialKeyLayout)
{
	cellOskDialog.todo("cellOskDialogSetInitialKeyLayout(initialKeyLayout=%d)", initialKeyLayout);

	// TODO: error checks

	g_fxo->get<osk_info>().initial_key_layout = static_cast<CellOskDialogInitialKeyLayout>(initialKeyLayout);

	// TODO: use value

	return CELL_OK;
}

error_code cellOskDialogDisableDimmer()
{
	cellOskDialog.todo("cellOskDialogDisableDimmer()");

	// TODO: error checks

	g_fxo->get<osk_info>().dimmer_enabled = false;

	// TODO: use value

	return CELL_OK;
}

error_code cellOskDialogSetKeyLayoutOption(u32 option)
{
	cellOskDialog.todo("cellOskDialogSetKeyLayoutOption(option=0x%x)", option);

	if (option == 0 || option > 3) // CELL_OSKDIALOG_10KEY_PANEL OR CELL_OSKDIALOG_FULLKEY_PANEL
	{
		return CELL_OSKDIALOG_ERROR_PARAM;
	}

	g_fxo->get<osk_info>().key_layout = option;

	// TODO: use value

	return CELL_OK;
}

error_code cellOskDialogAddSupportLanguage(u32 supportLanguage)
{
	cellOskDialog.todo("cellOskDialogAddSupportLanguage(supportLanguage=0x%x)", supportLanguage);

	// TODO: error checks

	g_fxo->get<osk_info>().supported_languages = supportLanguage;

	// TODO: disable extra languages unless they were enabled here
	// Extra languages are:
	// CELL_OSKDIALOG_PANELMODE_POLISH
	// CELL_OSKDIALOG_PANELMODE_KOREAN
	// CELL_OSKDIALOG_PANELMODE_TURKEY
	// CELL_OSKDIALOG_PANELMODE_TRADITIONAL_CHINESE
	// CELL_OSKDIALOG_PANELMODE_SIMPLIFIED_CHINESE
	// CELL_OSKDIALOG_PANELMODE_PORTUGUESE_BRAZIL
	// CELL_OSKDIALOG_PANELMODE_DANISH
	// CELL_OSKDIALOG_PANELMODE_SWEDISH
	// CELL_OSKDIALOG_PANELMODE_NORWEGIAN
	// CELL_OSKDIALOG_PANELMODE_FINNISH

	return CELL_OK;
}

error_code cellOskDialogSetLayoutMode(s32 layoutMode)
{
	cellOskDialog.todo("cellOskDialogSetLayoutMode(layoutMode=%d)", layoutMode);

	// TODO: error checks

	g_fxo->get<osk_info>().layout_mode = layoutMode;

	// TODO: use layout mode

	return CELL_OK;
}

error_code cellOskDialogGetInputText(vm::ptr<CellOskDialogCallbackReturnParam> OutputInfo)
{
	cellOskDialog.warning("cellOskDialogGetInputText(OutputInfo=*0x%x)", OutputInfo);
	return getText(OutputInfo, false);
}

error_code register_keyboard_event_hook_callback(u16 hookEventMode, vm::ptr<cellOskDialogHardwareKeyboardEventHookCallback> pCallback)
{
	cellOskDialog.todo("register_keyboard_event_hook_callback(hookEventMode=%u, pCallback=*0x%x)", hookEventMode, pCallback);

	if (!pCallback)
	{
		return CELL_OSKDIALOG_ERROR_PARAM;
	}

	// TODO: register callback and and use it

	return CELL_OK;
}

error_code cellOskDialogExtRegisterKeyboardEventHookCallback(u16 hookEventMode, vm::ptr<cellOskDialogHardwareKeyboardEventHookCallback> pCallback)
{
	cellOskDialog.todo("cellOskDialogExtRegisterKeyboardEventHookCallback(hookEventMode=%u, pCallback=*0x%x)", hookEventMode, pCallback);

	if (hookEventMode == 0 || hookEventMode > 3) // CELL_OSKDIALOG_EVENT_HOOK_TYPE_FUNCTION_KEY OR CELL_OSKDIALOG_EVENT_HOOK_TYPE_ASCII_KEY
	{
		return CELL_OSKDIALOG_ERROR_PARAM;
	}

	return register_keyboard_event_hook_callback(hookEventMode, pCallback);
}

error_code cellOskDialogExtRegisterKeyboardEventHookCallbackEx(u16 hookEventMode, vm::ptr<cellOskDialogHardwareKeyboardEventHookCallback> pCallback)
{
	cellOskDialog.todo("cellOskDialogExtRegisterKeyboardEventHookCallbackEx(hookEventMode=%u, pCallback=*0x%x)", hookEventMode, pCallback);

	if (hookEventMode == 0 || hookEventMode > 7) // CELL_OSKDIALOG_EVENT_HOOK_TYPE_FUNCTION_KEY OR CELL_OSKDIALOG_EVENT_HOOK_TYPE_ASCII_KEY OR CELL_OSKDIALOG_EVENT_HOOK_TYPE_ONLY_MODIFIER
	{
		return CELL_OSKDIALOG_ERROR_PARAM;
	}

	return register_keyboard_event_hook_callback(hookEventMode, pCallback);
}

error_code cellOskDialogExtAddJapaneseOptionDictionary(vm::cpptr<char> filePath)
{
	cellOskDialog.todo("cellOskDialogExtAddJapaneseOptionDictionary(filePath=**0x%0x)", filePath);
	return CELL_OK;
}

error_code cellOskDialogExtEnableClipboard()
{
	cellOskDialog.todo("cellOskDialogExtEnableClipboard()");

	// TODO: error checks

	g_fxo->get<osk_info>().clipboard_enabled = true;

	// TODO: implement copy paste

	return CELL_OK;
}

error_code cellOskDialogExtSendFinishMessage(u32 /*CellOskDialogFinishReason*/ finishReason)
{
	cellOskDialog.warning("cellOskDialogExtSendFinishMessage(finishReason=%d)", finishReason);

	const auto osk = _get_osk_dialog();

	// Check for "Open" dialog.
	if (!osk || osk->state.load() == OskDialogState::Unloaded)
	{
		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	osk->Close(finishReason);

	return CELL_OK;
}

error_code cellOskDialogExtAddOptionDictionary(vm::cptr<CellOskDialogImeDictionaryInfo> dictionaryInfo)
{
	cellOskDialog.todo("cellOskDialogExtAddOptionDictionary(dictionaryInfo=*0x%x)", dictionaryInfo);
	return CELL_OK;
}

error_code cellOskDialogExtSetInitialScale(f32 initialScale)
{
	cellOskDialog.todo("cellOskDialogExtSetInitialScale(initialScale=%f)", initialScale);

	// TODO: error checks (CELL_OSKDIALOG_SCALE_MIN, CELL_OSKDIALOG_SCALE_MAX)

	g_fxo->get<osk_info>().initial_scale = initialScale;

	// TODO: implement overlay scaling

	return CELL_OK;
}

error_code cellOskDialogExtInputDeviceLock()
{
	cellOskDialog.todo("cellOskDialogExtInputDeviceLock()");

	// TODO: error checks

	g_fxo->get<osk_info>().lock_ext_input = true;

	// TODO: change osk device input

	return CELL_OK;
}

error_code cellOskDialogExtInputDeviceUnlock()
{
	cellOskDialog.todo("cellOskDialogExtInputDeviceUnlock()");

	// TODO: error checks

	g_fxo->get<osk_info>().lock_ext_input = false;

	// TODO: change osk device input

	return CELL_OK;
}

error_code cellOskDialogExtSetBaseColor(f32 red, f32 blue, f32 green, f32 alpha)
{
	cellOskDialog.todo("cellOskDialogExtSetBaseColor(red=%f, blue=%f, green=%f, alpha=%f)", red, blue, green, alpha);

	// TODO: error checks

	auto& osk = g_fxo->get<osk_info>();
	osk.base_color_red = red;
	osk.base_color_blue = blue;
	osk.base_color_green = green;
	osk.base_color_alpha = alpha;

	// TODO: use osk base color

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

	const auto osk = _get_osk_dialog();

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
	cellOskDialog.todo("cellOskDialogExtSetPointerEnable(enable=%d)", enable);

	// TODO: error checks

	g_fxo->get<osk_info>().pointer_enabled = enable;

	// TODO: use new value in osk

	return CELL_OK;
}

error_code cellOskDialogExtUpdatePointerDisplayPos(vm::cptr<CellOskDialogPoint> pos)
{
	cellOskDialog.todo("cellOskDialogExtUpdatePointerDisplayPos(pos=0x%x, posX=%f, posY=%f)", pos->x, pos->y);

	// TODO: error checks

	if (pos)
	{
		g_fxo->get<osk_info>().pointer_pos = *pos;
	}

	// TODO: use new value in osk

	return CELL_OK;
}

error_code cellOskDialogExtEnableHalfByteKana()
{
	cellOskDialog.todo("cellOskDialogExtEnableHalfByteKana()");

	// TODO: error checks

	g_fxo->get<osk_info>().half_byte_kana_enabled = true;

	// TODO: use new value in osk

	return CELL_OK;
}

error_code cellOskDialogExtDisableHalfByteKana()
{
	cellOskDialog.todo("cellOskDialogExtDisableHalfByteKana()");

	// TODO: error checks

	g_fxo->get<osk_info>().half_byte_kana_enabled = false;

	// TODO: use new value in osk

	return CELL_OK;
}

error_code cellOskDialogExtRegisterForceFinishCallback(vm::ptr<cellOskDialogForceFinishCallback> pCallback)
{
	cellOskDialog.todo("cellOskDialogExtRegisterForceFinishCallback(pCallback=*0x%x)", pCallback);

	if (!pCallback)
	{
		return CELL_OSKDIALOG_ERROR_PARAM;
	}

	// TODO: register and use force finish callback (PS button during continuous mode)

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
