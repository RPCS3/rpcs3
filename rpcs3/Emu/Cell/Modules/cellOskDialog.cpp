#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/System.h"

#include "cellSysutil.h"
#include "cellOskDialog.h"
#include "cellMsgDialog.h"

logs::channel cellOskDialog("cellOskDialog");

static char16_t s_osk_text[CELL_OSKDIALOG_STRING_SIZE];

s32 cellOskDialogLoadAsync(u32 container, vm::ptr<CellOskDialogParam> dialogParam, vm::ptr<CellOskDialogInputFieldInfo> inputFieldInfo)
{
	cellOskDialog.warning("cellOskDialogLoadAsync(container=0x%x, dialogParam=*0x%x, inputFieldInfo=*0x%x)", container, dialogParam, inputFieldInfo);

	u32 maxLength = (inputFieldInfo->limit_length >= 512) ? 511 : (u32)inputFieldInfo->limit_length;

	std::memset(s_osk_text, 0, sizeof(s_osk_text));

	if (inputFieldInfo->init_text.addr() != 0)
		for (u32 i = 0; (i < maxLength) && (inputFieldInfo->init_text[i] != 0); i++)
			s_osk_text[i] = inputFieldInfo->init_text[i];

	const auto osk = Emu.GetCallbacks().get_msg_dialog();
	bool result = false;

	osk->on_close = [&](s32 status)
	{
		if (status != CELL_MSGDIALOG_BUTTON_OK) sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_INPUT_CANCELED, 0);
		sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_FINISHED, 0);
		result = true;
	};

	osk->on_osk_input_entered = [&]()
	{
		sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_INPUT_ENTERED, 0);
	};

	Emu.CallAfter([&]()
	{
		osk->CreateOsk("On Screen Keyboard", s_osk_text, maxLength);
	});

	sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_LOADED, 0);

	while (!result)
	{
		thread_ctrl::wait_for(1000);
	}

	return CELL_OK;
}

s32 cellOskDialogUnloadAsync(vm::ptr<CellOskDialogCallbackReturnParam> OutputInfo)
{
	cellOskDialog.warning("cellOskDialogUnloadAsync(OutputInfo=*0x%x)", OutputInfo);
	OutputInfo->result = CELL_OSKDIALOG_INPUT_FIELD_RESULT_OK;

	for (int i = 0; i < OutputInfo->numCharsResultString; i++)
	{
		OutputInfo->pResultString[i] = s_osk_text[i];
	}

	sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_UNLOADED, 0);

	return CELL_OK;
}

s32 cellOskDialogGetSize(vm::ptr<u16> width, vm::ptr<u16> height, vm::ptr<CellOskDialogType> dialogType)
{
	cellOskDialog.warning("cellOskDialogGetSize(width=*0x%x, height=*0x%x, dialogType=*0x%x)", width, height, dialogType);
	*width = 1;
	*height = 1;
	return CELL_OK;
}

s32 cellOskDialogAbort()
{
	cellOskDialog.warning("cellOskDialogAbort()");
	sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_FINISHED, 0);
	return CELL_OK;
}

s32 cellOskDialogSetDeviceMask(u32 deviceMask)
{
	cellOskDialog.todo("cellOskDialogSetDeviceMask(deviceMask=0x%x)", deviceMask);
	return CELL_OK;
}

s32 cellOskDialogSetSeparateWindowOption(vm::ptr<CellOskDialogSeparateWindowOption> windowOption)
{
	cellOskDialog.todo("cellOskDialogSetSeparateWindowOption(windowOption=*0x%x)", windowOption);
	return CELL_OK;
}

s32 cellOskDialogSetInitialInputDevice(vm::ptr<CellOskDialogInputDevice> inputDevice)
{
	cellOskDialog.todo("cellOskDialogSetInitialInputDevice(inputDevice=*0x%x)", inputDevice);
	return CELL_OK;
}

s32 cellOskDialogSetInitialKeyLayout(vm::ptr<CellOskDialogInitialKeyLayout> initialKeyLayout)
{
	cellOskDialog.todo("cellOskDialogSetInitialKeyLayout(initialKeyLayout=*0x%x)", initialKeyLayout);
	return CELL_OK;
}

s32 cellOskDialogDisableDimmer()
{
	cellOskDialog.todo("cellOskDialogDisableDimmer()");
	return CELL_OK;
}

s32 cellOskDialogSetKeyLayoutOption(u32 option)
{
	cellOskDialog.todo("cellOskDialogSetKeyLayoutOption(option=0x%x)", option);
	return CELL_OK;
}

s32 cellOskDialogAddSupportLanguage(u32 supportLanguage)
{
	cellOskDialog.todo("cellOskDialogAddSupportLanguage(supportLanguage=0x%x)", supportLanguage);
	return CELL_OK;
}

s32 cellOskDialogSetLayoutMode(s32 layoutMode)
{
	cellOskDialog.todo("cellOskDialogSetLayoutMode(layoutMode=%d)", layoutMode);
	return CELL_OK;
}

s32 cellOskDialogGetInputText(vm::ptr<CellOskDialogCallbackReturnParam> OutputInfo)
{
	cellOskDialog.warning("cellOskDialogGetInputText(OutputInfo=*0x%x)", OutputInfo);
	return cellOskDialogUnloadAsync(OutputInfo); //Same but for use with cellOskDialogSetSeparateWindowOption(). TODO. 
}

s32 cellOskDialogExtInputDeviceUnlock()
{
	cellOskDialog.todo("cellOskDialogExtInputDeviceUnlock()");
	return CELL_OK;
}

s32 cellOskDialogExtRegisterKeyboardEventHookCallback(u16 hookEventMode, vm::ptr<cellOskDialogHardwareKeyboardEventHookCallback> pCallback)
{
	cellOskDialog.todo("cellOskDialogExtRegisterKeyboardEventHookCallback(hookEventMode=%u, pCallback=*0x%x)", hookEventMode, pCallback);
	return CELL_OK;
}

s32 cellOskDialogExtRegisterKeyboardEventHookCallbackEx(u16 hookEventMode, vm::ptr<cellOskDialogHardwareKeyboardEventHookCallback> pCallback)
{
	cellOskDialog.todo("cellOskDialogExtRegisterKeyboardEventHookCallbackEx(hookEventMode=%u, pCallback=*0x%x)", hookEventMode, pCallback);
	return CELL_OK;
}

s32 cellOskDialogExtAddJapaneseOptionDictionary(vm::cpptr<char> filePath)
{
	cellOskDialog.todo("cellOskDialogExtAddJapaneseOptionDictionary(filePath=**0x%0x)", filePath);
	return CELL_OK;
}

s32 cellOskDialogExtEnableClipboard()
{
	cellOskDialog.todo("cellOskDialogExtEnableClipboard()");
	return CELL_OK;
}

s32 cellOskDialogExtSendFinishMessage(s32 /*CellOskDialogFinishReason*/ finishReason)
{
	cellOskDialog.todo("cellOskDialogExtSendFinishMessage(finishReason=%d)", finishReason);
	return CELL_OK;
}

s32 cellOskDialogExtAddOptionDictionary(vm::cptr<CellOskDialogImeDictionaryInfo> dictionaryInfo)
{
	cellOskDialog.todo("cellOskDialogExtAddOptionDictionary(dictionaryInfo=*0x%x)", dictionaryInfo);
	return CELL_OK;
}

s32 cellOskDialogExtSetInitialScale(f32 initialScale)
{
	cellOskDialog.todo("cellOskDialogExtSetInitialScale(initialScale=%f)", initialScale);
	return CELL_OK;
}

s32 cellOskDialogExtInputDeviceLock()
{
	cellOskDialog.todo("cellOskDialogExtInputDeviceLock()");
	return CELL_OK;
}

s32 cellOskDialogExtSetBaseColor(f32 red, f32 blue, f32 green, f32 alpha)
{
	cellOskDialog.warning("cellOskDialogExtSetBaseColor(red=%f, blue=%f, green=%f, alpha=%f)", red, blue, green, alpha);
	return CELL_OK;
}

s32 cellOskDialogExtRegisterConfirmWordFilterCallback(vm::ptr<cellOskDialogConfirmWordFilterCallback> pCallback)
{
	cellOskDialog.todo("cellOskDialogExtRegisterConfirmWordFilterCallback(pCallback=*0x%x)", pCallback);
	return CELL_OK;
}

s32 cellOskDialogExtUpdateInputText()
{
	cellOskDialog.todo("cellOskDialogExtUpdateInputText()");
	return CELL_OK;
}

s32 cellOskDialogExtDisableHalfByteKana()
{
	cellOskDialog.todo("cellOskDialogExtDisableHalfByteKana()");
	return CELL_OK;
}

s32 cellOskDialogExtSetPointerEnable(b8 enable)
{
	cellOskDialog.todo("cellOskDialogExtSetPointerEnable(enable=%d)", enable);
	return CELL_OK;
}

s32 cellOskDialogExtUpdatePointerDisplayPos()
{
	cellOskDialog.todo("cellOskDialogExtUpdatePointerDisplayPos"); // Missing arguments
	return CELL_OK;
}

s32 cellOskDialogExtEnableHalfByteKana()
{
	cellOskDialog.todo("cellOskDialogExtEnableHalfByteKana()");
	return CELL_OK;
}

s32 cellOskDialogExtRegisterForceFinishCallback(vm::ptr<cellOskDialogForceFinishCallback> pCallback)
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
