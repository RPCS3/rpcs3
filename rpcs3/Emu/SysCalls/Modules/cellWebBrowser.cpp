#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "cellWebBrowser.h"

extern Module cellSysutil;

s32 cellWebBrowserActivate()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserConfig()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserConfig2()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserConfigGetHeapSize()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserConfigGetHeapSize2()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserConfigSetCustomExit()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserConfigSetDisableTabs()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserConfigSetErrorHook2()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserConfigSetFullScreen2()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserConfigSetFullVersion2()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserConfigSetFunction()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserConfigSetFunction2()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserConfigSetHeapSize()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserConfigSetHeapSize2()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserConfigSetMimeSet()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserConfigSetNotifyHook2()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserConfigSetRequestHook2()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserConfigSetStatusHook2()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserConfigSetTabCount2()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserConfigSetUnknownMIMETypeHook2()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserConfigSetVersion()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserConfigSetViewCondition2()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserConfigSetViewRect2()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserConfigWithVer()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserCreate()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserCreate2()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserCreateRender2()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserCreateRenderWithRect2()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserCreateWithConfig()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserCreateWithConfigFull()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserCreateWithRect2()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserDeactivate()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserDestroy()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserDestroy2()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserEstimate()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserEstimate2(vm::cptr<CellWebBrowserConfig2> config, vm::ptr<u32> memSize)
{
	cellSysutil.Warning("cellWebBrowserEstimate2(config=*0x%x, memSize=*0x%x)", config, memSize);

	// TODO: When cellWebBrowser stuff is implemented, change this to some real
	// needed memory buffer size.
	*memSize = 1 * 1024 * 1024; // 1 MB
	return CELL_OK;
}

s32 cellWebBrowserGetUsrdataOnGameExit()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserInitialize()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserNavigate2()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserSetLocalContentsAdditionalTitleID()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserSetSystemCallbackUsrdata()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserShutdown()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserUpdatePointerDisplayPos2()
{
	throw EXCEPTION("");
}

s32 cellWebBrowserWakeupWithGameExit()
{
	throw EXCEPTION("");
}

s32 cellWebComponentCreate()
{
	throw EXCEPTION("");
}

s32 cellWebComponentCreateAsync()
{
	throw EXCEPTION("");
}

s32 cellWebComponentDestroy()
{
	throw EXCEPTION("");
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
