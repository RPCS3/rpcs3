#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellOskDialog;

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
	extern Module cellSysutil;

	// cellOskDialog functions:
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

Module cellOskDialog("cellOskDialog", []()
{
	// cellOskDialogExt functions:
	REG_FUNC(cellOskDialog, cellOskDialogExtInputDeviceUnlock);
	REG_FUNC(cellOskDialog, cellOskDialogExtRegisterKeyboardEventHookCallback);
	REG_FUNC(cellOskDialog, cellOskDialogExtAddJapaneseOptionDictionary);
	REG_FUNC(cellOskDialog, cellOskDialogExtEnableClipboard);
	REG_FUNC(cellOskDialog, cellOskDialogExtSendFinishMessage);
	REG_FUNC(cellOskDialog, cellOskDialogExtAddOptionDictionary);
	REG_FUNC(cellOskDialog, cellOskDialogExtSetInitialScale);
	REG_FUNC(cellOskDialog, cellOskDialogExtInputDeviceLock);
	REG_FUNC(cellOskDialog, cellOskDialogExtSetBaseColor);
	REG_FUNC(cellOskDialog, cellOskDialogExtRegisterConfirmWordFilterCallback);
	REG_FUNC(cellOskDialog, cellOskDialogExtUpdateInputText);
	REG_FUNC(cellOskDialog, cellOskDialogExtDisableHalfByteKana);
	REG_FUNC(cellOskDialog, cellOskDialogExtSetPointerEnable);
	REG_FUNC(cellOskDialog, cellOskDialogExtUpdatePointerDisplayPos);
	REG_FUNC(cellOskDialog, cellOskDialogExtEnableHalfByteKana);
	REG_FUNC(cellOskDialog, cellOskDialogExtRegisterForceFinishCallback);
});
