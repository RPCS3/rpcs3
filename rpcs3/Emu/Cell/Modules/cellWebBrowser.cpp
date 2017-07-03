#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "cellWebBrowser.h"

extern logs::channel cellSysutil;

s32 cellWebBrowserActivate()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserConfig()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserConfig2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserConfigGetHeapSize()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserConfigGetHeapSize2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserConfigSetCustomExit()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserConfigSetDisableTabs()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserConfigSetErrorHook2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserConfigSetFullScreen2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserConfigSetFullVersion2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserConfigSetFunction()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserConfigSetFunction2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserConfigSetHeapSize()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserConfigSetHeapSize2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserConfigSetMimeSet()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserConfigSetNotifyHook2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserConfigSetRequestHook2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserConfigSetStatusHook2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserConfigSetTabCount2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserConfigSetUnknownMIMETypeHook2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserConfigSetVersion()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserConfigSetViewCondition2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserConfigSetViewRect2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserConfigWithVer()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserCreate()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserCreate2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserCreateRender2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserCreateRenderWithRect2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserCreateWithConfig()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserCreateWithConfigFull()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserCreateWithRect2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserDeactivate()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserDestroy()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserDestroy2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserEstimate()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserEstimate2(vm::cptr<CellWebBrowserConfig2> config, vm::ptr<u32> memSize)
{
	cellSysutil.warning("cellWebBrowserEstimate2(config=*0x%x, memSize=*0x%x)", config, memSize);

	// TODO: When cellWebBrowser stuff is implemented, change this to some real
	// needed memory buffer size.
	*memSize = 1 * 1024 * 1024; // 1 MB
	return CELL_OK;
}

s32 cellWebBrowserGetUsrdataOnGameExit(vm::ptr<CellWebBrowserUsrdata> ptr)
{
	cellSysutil.todo("cellWebBrowserGetUsrdataOnGameExit(ptr=*0x%x", ptr);
	return CELL_OK;
}

s32 cellWebBrowserInitialize()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserNavigate2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserSetLocalContentsAdditionalTitleID()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserSetSystemCallbackUsrdata()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserShutdown()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserUpdatePointerDisplayPos2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserWakeupWithGameExit()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebComponentCreate()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebComponentCreateAsync()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebComponentDestroy()
{
	fmt::throw_exception("Unimplemented" HERE);
}


void cellSysutil_WebBrowser_init()
{
	REG_FUNC(cellSysutil, cellWebBrowserActivate);
	REG_FUNC(cellSysutil, cellWebBrowserConfig);
	REG_FUNC(cellSysutil, cellWebBrowserConfig2);
	REG_FUNC(cellSysutil, cellWebBrowserConfigGetHeapSize);
	REG_FUNC(cellSysutil, cellWebBrowserConfigGetHeapSize2);
	REG_FUNC(cellSysutil, cellWebBrowserConfigSetCustomExit);
	REG_FUNC(cellSysutil, cellWebBrowserConfigSetDisableTabs);
	REG_FUNC(cellSysutil, cellWebBrowserConfigSetErrorHook2);
	REG_FUNC(cellSysutil, cellWebBrowserConfigSetFullScreen2);
	REG_FUNC(cellSysutil, cellWebBrowserConfigSetFullVersion2);
	REG_FUNC(cellSysutil, cellWebBrowserConfigSetFunction);
	REG_FUNC(cellSysutil, cellWebBrowserConfigSetFunction2);
	REG_FUNC(cellSysutil, cellWebBrowserConfigSetHeapSize);
	REG_FUNC(cellSysutil, cellWebBrowserConfigSetHeapSize2);
	REG_FUNC(cellSysutil, cellWebBrowserConfigSetMimeSet);
	REG_FUNC(cellSysutil, cellWebBrowserConfigSetNotifyHook2);
	REG_FUNC(cellSysutil, cellWebBrowserConfigSetRequestHook2);
	REG_FUNC(cellSysutil, cellWebBrowserConfigSetStatusHook2);
	REG_FUNC(cellSysutil, cellWebBrowserConfigSetTabCount2);
	REG_FUNC(cellSysutil, cellWebBrowserConfigSetUnknownMIMETypeHook2);
	REG_FUNC(cellSysutil, cellWebBrowserConfigSetVersion);
	REG_FUNC(cellSysutil, cellWebBrowserConfigSetViewCondition2);
	REG_FUNC(cellSysutil, cellWebBrowserConfigSetViewRect2);
	REG_FUNC(cellSysutil, cellWebBrowserConfigWithVer);
	REG_FUNC(cellSysutil, cellWebBrowserCreate);
	REG_FUNC(cellSysutil, cellWebBrowserCreate2);
	REG_FUNC(cellSysutil, cellWebBrowserCreateRender2);
	REG_FUNC(cellSysutil, cellWebBrowserCreateRenderWithRect2);
	REG_FUNC(cellSysutil, cellWebBrowserCreateWithConfig);
	REG_FUNC(cellSysutil, cellWebBrowserCreateWithConfigFull);
	REG_FUNC(cellSysutil, cellWebBrowserCreateWithRect2);
	REG_FUNC(cellSysutil, cellWebBrowserDeactivate);
	REG_FUNC(cellSysutil, cellWebBrowserDestroy);
	REG_FUNC(cellSysutil, cellWebBrowserDestroy2);
	REG_FUNC(cellSysutil, cellWebBrowserEstimate);
	REG_FUNC(cellSysutil, cellWebBrowserEstimate2);
	REG_FUNC(cellSysutil, cellWebBrowserGetUsrdataOnGameExit);
	REG_FUNC(cellSysutil, cellWebBrowserInitialize);
	REG_FUNC(cellSysutil, cellWebBrowserNavigate2);
	REG_FUNC(cellSysutil, cellWebBrowserSetLocalContentsAdditionalTitleID);
	REG_FUNC(cellSysutil, cellWebBrowserSetSystemCallbackUsrdata);
	REG_FUNC(cellSysutil, cellWebBrowserShutdown);
	REG_FUNC(cellSysutil, cellWebBrowserUpdatePointerDisplayPos2);
	REG_FUNC(cellSysutil, cellWebBrowserWakeupWithGameExit);
	REG_FUNC(cellSysutil, cellWebComponentCreate);
	REG_FUNC(cellSysutil, cellWebComponentCreateAsync);
	REG_FUNC(cellSysutil, cellWebComponentDestroy);
}
