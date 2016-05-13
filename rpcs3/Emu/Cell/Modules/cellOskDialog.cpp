#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

logs::channel cellOskDialog("cellOskDialog", logs::level::notice);

s32 cellOskDialogLoadAsync()
{
	throw EXCEPTION("");
}

s32 cellOskDialogUnloadAsync()
{
	throw EXCEPTION("");
}

s32 cellOskDialogGetSize()
{
	throw EXCEPTION("");
}

s32 cellOskDialogAbort()
{
	throw EXCEPTION("");
}

s32 cellOskDialogSetDeviceMask()
{
	throw EXCEPTION("");
}

s32 cellOskDialogSetSeparateWindowOption()
{
	throw EXCEPTION("");
}

s32 cellOskDialogSetInitialInputDevice()
{
	throw EXCEPTION("");
}

s32 cellOskDialogSetInitialKeyLayout()
{
	throw EXCEPTION("");
}

s32 cellOskDialogDisableDimmer()
{
	throw EXCEPTION("");
}

s32 cellOskDialogSetKeyLayoutOption()
{
	throw EXCEPTION("");
}

s32 cellOskDialogAddSupportLanguage()
{
	throw EXCEPTION("");
}

s32 cellOskDialogSetLayoutMode()
{
	throw EXCEPTION("");
}

s32 cellOskDialogGetInputText()
{
	throw EXCEPTION("");
}


s32 cellOskDialogExtInputDeviceUnlock()
{
	throw EXCEPTION("");
}

s32 cellOskDialogExtRegisterKeyboardEventHookCallback()
{
	throw EXCEPTION("");
}

s32 cellOskDialogExtAddJapaneseOptionDictionary()
{
	throw EXCEPTION("");
}

s32 cellOskDialogExtEnableClipboard()
{
	throw EXCEPTION("");
}

s32 cellOskDialogExtSendFinishMessage()
{
	throw EXCEPTION("");
}

s32 cellOskDialogExtAddOptionDictionary()
{
	throw EXCEPTION("");
}

s32 cellOskDialogExtSetInitialScale()
{
	throw EXCEPTION("");
}

s32 cellOskDialogExtInputDeviceLock()
{
	throw EXCEPTION("");
}

s32 cellOskDialogExtSetBaseColor()
{
	throw EXCEPTION("");
}

s32 cellOskDialogExtRegisterConfirmWordFilterCallback()
{
	throw EXCEPTION("");
}

s32 cellOskDialogExtUpdateInputText()
{
	throw EXCEPTION("");
}

s32 cellOskDialogExtDisableHalfByteKana()
{
	throw EXCEPTION("");
}

s32 cellOskDialogExtSetPointerEnable()
{
	throw EXCEPTION("");
}

s32 cellOskDialogExtUpdatePointerDisplayPos()
{
	throw EXCEPTION("");
}

s32 cellOskDialogExtEnableHalfByteKana()
{
	throw EXCEPTION("");
}

s32 cellOskDialogExtRegisterForceFinishCallback()
{
	throw EXCEPTION("");
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
