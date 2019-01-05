#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"
#include "pad_thread.h"

#include "cellSysutil.h"
#include "cellOskDialog.h"
#include "cellMsgDialog.h"

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

OskDialogBase::~OskDialogBase()
{
}

error_code cellOskDialogLoadAsync(u32 container, vm::ptr<CellOskDialogParam> dialogParam, vm::ptr<CellOskDialogInputFieldInfo> inputFieldInfo)
{
	cellOskDialog.warning("cellOskDialogLoadAsync(container=0x%x, dialogParam=*0x%x, inputFieldInfo=*0x%x)", container, dialogParam, inputFieldInfo);

	if (!dialogParam || !inputFieldInfo || !inputFieldInfo->message || !inputFieldInfo->init_text || inputFieldInfo->limit_length > CELL_OSKDIALOG_STRING_SIZE)
	{
		return CELL_OSKDIALOG_ERROR_PARAM;
	}

	auto osk = fxm::get<OskDialogBase>();
	if (!osk)
	{
		osk = fxm::import<OskDialogBase>(Emu.GetCallbacks().get_osk_dialog);
	}

	// Can't open another dialog if this one is already open.
	if (!osk || osk->state.load() == OskDialogState::Open)
	{
		return CELL_SYSUTIL_ERROR_BUSY;
	}

	// Get the OSK options
	u32 maxLength = (inputFieldInfo->limit_length >= CELL_OSKDIALOG_STRING_SIZE) ? 511 : (u32)inputFieldInfo->limit_length;
	u32 options = dialogParam->prohibitFlgs;
	bool use_seperate_windows = false; // TODO
	bool keep_dialog_open = false; // TODO maybe same as use_seperate_windows

	// Get init text and prepare return value
	osk->osk_input_result = CELL_OSKDIALOG_INPUT_FIELD_RESULT_OK;
	std::memset(osk->osk_text, 0, sizeof(osk->osk_text));
	std::memset(osk->osk_text_old, 0, sizeof(osk->osk_text_old));

	if (inputFieldInfo->init_text.addr() != 0)
	{
		for (u32 i = 0; (i < maxLength) && (inputFieldInfo->init_text[i] != 0); i++)
		{
			osk->osk_text[i] = inputFieldInfo->init_text[i];
			osk->osk_text_old[i] = inputFieldInfo->init_text[i];
		}
	}

	// Get message to display above the input field
	// Guarantees 0 terminated (+1). In praxis only 128 but for now lets display all of it
	char16_t message[CELL_OSKDIALOG_STRING_SIZE + 1];
	std::memset(message, 0, sizeof(message));

	if (inputFieldInfo->message.addr() != 0)
	{
		for (u32 i = 0; (i < CELL_OSKDIALOG_STRING_SIZE) && (inputFieldInfo->message[i] != 0); i++)
		{
			message[i] = inputFieldInfo->message[i];
		}
	}

	bool result = false;

	osk->on_close = [maxLength, use_seperate_windows, keep_dialog_open, wptr = std::weak_ptr<OskDialogBase>(osk)](s32 status)
	{
		const auto osk = wptr.lock();
		osk->state = OskDialogState::Close;

		const bool accepted = status == CELL_MSGDIALOG_BUTTON_OK;

		if (accepted)
		{
			if (osk->osk_confirm_callback)
			{
				vm::ptr<u16> string_to_send = vm::cast(vm::alloc(CELL_OSKDIALOG_STRING_SIZE * 2, vm::main));
				atomic_t<bool> done = false;
				u32 return_value;
				u32 i = 0;

				for (i; i < CELL_OSKDIALOG_STRING_SIZE - 1; i++)
				{
					string_to_send[i] = osk->osk_text[i];
					if (osk->osk_text[i] == 0) break;
				}

				sysutil_register_cb([&, length = i](ppu_thread& cb_ppu) -> s32
				{
					return_value = osk->osk_confirm_callback(cb_ppu, string_to_send, (s32)length);
					cellOskDialog.warning("osk_confirm_callback return_value=%d", return_value);

					for (u32 i = 0; i < CELL_OSKDIALOG_STRING_SIZE - 1; i++)
					{
						osk->osk_text[i] = string_to_send[i];
					}

					done = true;
					return 0;
				});

				// wait for check callback
				while (done == false)
				{
					std::this_thread::yield();
				}

				// reset this here (just to make sure it's null)
				osk->osk_confirm_callback = vm::null;
			}

			if (use_seperate_windows)
			{
				if (osk->osk_text[0] == 0)
				{
					cellOskDialog.warning("cellOskDialogLoadAsync: input result is CELL_OSKDIALOG_INPUT_FIELD_RESULT_NO_INPUT_TEXT");
					osk->osk_input_result = CELL_OSKDIALOG_INPUT_FIELD_RESULT_NO_INPUT_TEXT;
				}
				else
				{
					osk->osk_input_result = CELL_OSKDIALOG_INPUT_FIELD_RESULT_OK;
				}
			}
			else
			{
				osk->osk_input_result = CELL_OSKDIALOG_INPUT_FIELD_RESULT_OK;
			}
		}
		else
		{
			osk->osk_input_result = CELL_OSKDIALOG_INPUT_FIELD_RESULT_CANCELED;
		}

		// Send OSK status
		if (use_seperate_windows)
		{
			if (accepted)
			{
				if (keep_dialog_open)
				{
					sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_INPUT_ENTERED, 0);
				}
				else
				{
					sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_FINISHED, 0);
				}
			}
			else
			{
				sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_INPUT_CANCELED, 0);
			}
		}
		else
		{
			sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_FINISHED, 0);
		}

		pad::SetIntercepted(false);
	};

	osk->on_osk_input_entered = [=]()
	{
		if (use_seperate_windows)
		{
			sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_INPUT_ENTERED, 0);
		}
	};

	pad::SetIntercepted(true);

	Emu.CallAfter([=, &result]()
	{
		osk->Create("On Screen Keyboard", message, osk->osk_text, maxLength, options);
		result = true;
	});

	sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_LOADED, 0);

	while (!result)
	{
		thread_ctrl::wait_for(1000);
	}

	return CELL_OK;
}

error_code getText(vm::ptr<CellOskDialogCallbackReturnParam> OutputInfo, bool is_unload)
{
	if (!OutputInfo || OutputInfo->numCharsResultString < 0)
	{
		return CELL_OSKDIALOG_ERROR_PARAM;
	}

	const auto osk = fxm::get<OskDialogBase>();

	if (!osk)
	{
		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	if (is_unload)
	{
		OutputInfo->result = osk->osk_input_result;
	}
	else
	{
		if (memcmp(osk->osk_text_old, osk->osk_text, sizeof(osk->osk_text)) == 0)
		{
			OutputInfo->result = CELL_OSKDIALOG_INPUT_FIELD_RESULT_NO_INPUT_TEXT;
		}
		else
		{
			OutputInfo->result = CELL_OSKDIALOG_INPUT_FIELD_RESULT_OK;
		}
	}

	const bool is_valid = OutputInfo->pResultString && (OutputInfo->result == CELL_OSKDIALOG_INPUT_FIELD_RESULT_OK || (is_unload && OutputInfo->result == CELL_OSKDIALOG_INPUT_FIELD_RESULT_NO_INPUT_TEXT));

	for (u32 i = 0; i < CELL_OSKDIALOG_STRING_SIZE - 1; i++)
	{
		osk->osk_text_old[i] = osk->osk_text[i];

		if (is_valid && i < OutputInfo->numCharsResultString)
		{
			OutputInfo->pResultString[i] = osk->osk_text[i];
		}
	}

	if (is_unload)
	{
		// Unload should be called last, so remove the dialog here
		fxm::remove<OskDialogBase>();
		sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_UNLOADED, 0);
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

	const auto osk = fxm::get<OskDialogBase>();

	// Check for open dialog. In this case the dialog is only "Open" if it was not aborted before.
	if (!osk || osk->state.load() == OskDialogState::Abort)
	{
		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	// If the dialog has the Open state then it is in use. Only dialogs with the Close state can be aborted.
	if (!osk->state.compare_and_swap_test(OskDialogState::Open, OskDialogState::Abort))
	{
		return CELL_SYSUTIL_ERROR_BUSY;
	}

	osk->osk_input_result = CELL_OSKDIALOG_INPUT_FIELD_RESULT_ABORT;
	osk->Close(false);

	return CELL_OK;
}

error_code cellOskDialogSetDeviceMask(u32 deviceMask)
{
	cellOskDialog.todo("cellOskDialogSetDeviceMask(deviceMask=0x%x)", deviceMask);
	return CELL_OK;
}

error_code cellOskDialogSetSeparateWindowOption(vm::ptr<CellOskDialogSeparateWindowOption> windowOption)
{
	cellOskDialog.todo("cellOskDialogSetSeparateWindowOption(windowOption=*0x%x)", windowOption);

	// TODO: when games set the option to use seperate windows we will have to handle input signals and cancel signals

	return CELL_OK;
}

error_code cellOskDialogSetInitialInputDevice(vm::ptr<CellOskDialogInputDevice> inputDevice)
{
	cellOskDialog.todo("cellOskDialogSetInitialInputDevice(inputDevice=*0x%x)", inputDevice);
	return CELL_OK;
}

error_code cellOskDialogSetInitialKeyLayout(vm::ptr<CellOskDialogInitialKeyLayout> initialKeyLayout)
{
	cellOskDialog.todo("cellOskDialogSetInitialKeyLayout(initialKeyLayout=*0x%x)", initialKeyLayout);
	return CELL_OK;
}

error_code cellOskDialogDisableDimmer()
{
	cellOskDialog.todo("cellOskDialogDisableDimmer()");
	return CELL_OK;
}

error_code cellOskDialogSetKeyLayoutOption(u32 option)
{
	cellOskDialog.todo("cellOskDialogSetKeyLayoutOption(option=0x%x)", option);
	return CELL_OK;
}

error_code cellOskDialogAddSupportLanguage(u32 supportLanguage)
{
	cellOskDialog.todo("cellOskDialogAddSupportLanguage(supportLanguage=0x%x)", supportLanguage);
	return CELL_OK;
}

error_code cellOskDialogSetLayoutMode(s32 layoutMode)
{
	cellOskDialog.todo("cellOskDialogSetLayoutMode(layoutMode=%d)", layoutMode);
	return CELL_OK;
}

error_code cellOskDialogGetInputText(vm::ptr<CellOskDialogCallbackReturnParam> OutputInfo)
{
	cellOskDialog.warning("cellOskDialogGetInputText(OutputInfo=*0x%x)", OutputInfo);
	return getText(OutputInfo, false);
}

error_code cellOskDialogExtInputDeviceUnlock()
{
	cellOskDialog.todo("cellOskDialogExtInputDeviceUnlock()");
	return CELL_OK;
}

error_code cellOskDialogExtRegisterKeyboardEventHookCallback(u16 hookEventMode, vm::ptr<cellOskDialogHardwareKeyboardEventHookCallback> pCallback)
{
	cellOskDialog.todo("cellOskDialogExtRegisterKeyboardEventHookCallback(hookEventMode=%u, pCallback=*0x%x)", hookEventMode, pCallback);
	return CELL_OK;
}

error_code cellOskDialogExtRegisterKeyboardEventHookCallbackEx(u16 hookEventMode, vm::ptr<cellOskDialogHardwareKeyboardEventHookCallback> pCallback)
{
	cellOskDialog.todo("cellOskDialogExtRegisterKeyboardEventHookCallbackEx(hookEventMode=%u, pCallback=*0x%x)", hookEventMode, pCallback);
	return CELL_OK;
}

error_code cellOskDialogExtAddJapaneseOptionDictionary(vm::cpptr<char> filePath)
{
	cellOskDialog.todo("cellOskDialogExtAddJapaneseOptionDictionary(filePath=**0x%0x)", filePath);
	return CELL_OK;
}

error_code cellOskDialogExtEnableClipboard()
{
	cellOskDialog.todo("cellOskDialogExtEnableClipboard()");
	return CELL_OK;
}

error_code cellOskDialogExtSendFinishMessage(u32 /*CellOskDialogFinishReason*/ finishReason)
{
	cellOskDialog.warning("cellOskDialogExtSendFinishMessage(finishReason=%d)", finishReason);

	const auto osk = fxm::get<OskDialogBase>();

	// Check for open dialog.
	if (!osk || osk->state.load() != OskDialogState::Open)
	{
		return CELL_MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
	}

	osk->Close(finishReason == CELL_OSKDIALOG_CLOSE_CONFIRM);

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
	return CELL_OK;
}

error_code cellOskDialogExtInputDeviceLock()
{
	cellOskDialog.todo("cellOskDialogExtInputDeviceLock()");
	return CELL_OK;
}

error_code cellOskDialogExtSetBaseColor(f32 red, f32 blue, f32 green, f32 alpha)
{
	cellOskDialog.todo("cellOskDialogExtSetBaseColor(red=%f, blue=%f, green=%f, alpha=%f)", red, blue, green, alpha);
	return CELL_OK;
}

error_code cellOskDialogExtRegisterConfirmWordFilterCallback(vm::ptr<cellOskDialogConfirmWordFilterCallback> pCallback)
{
	cellOskDialog.warning("cellOskDialogExtRegisterConfirmWordFilterCallback(pCallback=*0x%x)", pCallback);

	auto osk = fxm::get<OskDialogBase>();
	if (!osk)
	{
		osk = fxm::import<OskDialogBase>(Emu.GetCallbacks().get_osk_dialog);
	}
	osk->osk_confirm_callback = pCallback;

	return CELL_OK;
}

error_code cellOskDialogExtUpdateInputText()
{
	cellOskDialog.todo("cellOskDialogExtUpdateInputText()");
	return CELL_OK;
}

error_code cellOskDialogExtDisableHalfByteKana()
{
	cellOskDialog.todo("cellOskDialogExtDisableHalfByteKana()");
	return CELL_OK;
}

error_code cellOskDialogExtSetPointerEnable(b8 enable)
{
	cellOskDialog.todo("cellOskDialogExtSetPointerEnable(enable=%d)", enable);
	return CELL_OK;
}

error_code cellOskDialogExtUpdatePointerDisplayPos(/*const CellOskDialogPoint pos*/)
{
	cellOskDialog.todo("cellOskDialogExtUpdatePointerDisplayPos(posX=%f, posY=%f)"/*, pos.x, pos.y*/);
	return CELL_OK;
}

error_code cellOskDialogExtEnableHalfByteKana()
{
	cellOskDialog.todo("cellOskDialogExtEnableHalfByteKana()");
	return CELL_OK;
}

error_code cellOskDialogExtRegisterForceFinishCallback(vm::ptr<cellOskDialogForceFinishCallback> pCallback)
{
	cellOskDialog.todo("cellOskDialogExtRegisterForceFinishCallback(pCallback=*0x%x)", pCallback);
	return CELL_OK;
}


void cellSysutil_OskDialog_init()
{
	REG_FUNC(cellSysutil, cellOskDialogLoadAsync);
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
