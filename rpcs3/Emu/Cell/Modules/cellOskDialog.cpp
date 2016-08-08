#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

logs::channel cellOskDialog("cellOskDialog", logs::level::notice);

s32 cellOskDialogLoadAsync()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellOskDialogUnloadAsync()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellOskDialogGetSize()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellOskDialogAbort()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellOskDialogSetDeviceMask()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellOskDialogSetSeparateWindowOption()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellOskDialogSetInitialInputDevice()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellOskDialogSetInitialKeyLayout()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellOskDialogDisableDimmer()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellOskDialogSetKeyLayoutOption()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellOskDialogAddSupportLanguage()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellOskDialogSetLayoutMode()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellOskDialogGetInputText()
{
	fmt::throw_exception("Unimplemented" HERE);
}


s32 cellOskDialogExtInputDeviceUnlock()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellOskDialogExtRegisterKeyboardEventHookCallback()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellOskDialogExtAddJapaneseOptionDictionary()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellOskDialogExtEnableClipboard()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellOskDialogExtSendFinishMessage()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellOskDialogExtAddOptionDictionary()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellOskDialogExtSetInitialScale()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellOskDialogExtInputDeviceLock()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellOskDialogExtSetBaseColor()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellOskDialogExtRegisterConfirmWordFilterCallback()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellOskDialogExtUpdateInputText()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellOskDialogExtDisableHalfByteKana()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellOskDialogExtSetPointerEnable()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellOskDialogExtUpdatePointerDisplayPos()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellOskDialogExtEnableHalfByteKana()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellOskDialogExtRegisterForceFinishCallback()
{
	fmt::throw_exception("Unimplemented" HERE);
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
