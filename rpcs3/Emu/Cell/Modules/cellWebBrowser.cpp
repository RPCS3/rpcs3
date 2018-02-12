#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "cellWebBrowser.h"
#include "Emu/IdManager.h"

extern logs::channel cellSysutil;

struct browser_t
{
	vm::ptr<CellWebBrowserSystemCallback> system_cb;
	vm::ptr<void> userData;
};

s32 cellWebBrowserActivate()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserConfig()
{
	fmt::throw_exception("Unimplemented" HERE);
}

error_code cellWebBrowserConfig2(vm::cptr<CellWebBrowserConfig2> config, u32 version)
{
	cellSysutil.todo("cellWebBrowserConfig2(config=*0x%x, version=%d)", config, version);
	return CELL_OK;
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

error_code cellWebBrowserConfigSetFullScreen2(vm::cptr<CellWebBrowserConfig2> config, u32 full)
{
	cellSysutil.todo("cellWebBrowserConfigSetFullScreen2(config=*0x%x, full=%d)", config, full);
	return CELL_OK;
}

s32 cellWebBrowserConfigSetFullVersion2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserConfigSetFunction()
{
	fmt::throw_exception("Unimplemented" HERE);
}

error_code cellWebBrowserConfigSetFunction2(vm::ptr<CellWebBrowserConfig2> config, u32 funcset)
{
	cellSysutil.todo("cellWebBrowserConfigSetFunction2(config=*0x%x, funcset=0x%x)", config, funcset);
	return CELL_OK;
}

s32 cellWebBrowserConfigSetHeapSize()
{
	fmt::throw_exception("Unimplemented" HERE);
}

error_code cellWebBrowserConfigSetHeapSize2(vm::ptr<CellWebBrowserConfig2> config, u32 size)
{
	cellSysutil.todo("cellWebBrowserConfigSetHeapSize(config=*0x%x, size=0x%x)", config, size);
	return CELL_OK;
}

s32 cellWebBrowserConfigSetMimeSet()
{
	fmt::throw_exception("Unimplemented" HERE);
}

error_code cellWebBrowserConfigSetNotifyHook2(vm::cptr<CellWebBrowserConfig2> config, vm::ptr<CellWebBrowserNotify> cb, vm::ptr<void> userdata)
{
	cellSysutil.todo("cellWebBrowserConfigSetNotifyHook2(config=*0x%x, cb=*0x%x, userdata=*0x%x)", config, cb, userdata);
	return CELL_OK;
}

s32 cellWebBrowserConfigSetRequestHook2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellWebBrowserConfigSetStatusHook2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

error_code cellWebBrowserConfigSetTabCount2(vm::cptr<CellWebBrowserConfig2> config, u32 tab_count)
{
	cellSysutil.todo("cellWebBrowserConfigSetTabCount2(config=*0x%x, tab_count=%d)", config, tab_count);
	return CELL_OK;
}

error_code cellWebBrowserConfigSetUnknownMIMETypeHook2(vm::cptr<CellWebBrowserConfig2> config, vm::ptr<CellWebBrowserMIMETypeCallback> cb, vm::ptr<void> userdata)
{
	cellSysutil.todo("cellWebBrowserConfigSetUnknownMIMETypeHook2(config=*0x%x, cb=*0x%x, userdata=*0x%x)", config, cb, userdata);
	return CELL_OK;
}

s32 cellWebBrowserConfigSetVersion()
{
	fmt::throw_exception("Unimplemented" HERE);
}

error_code cellWebBrowserConfigSetViewCondition2(vm::ptr<CellWebBrowserConfig2> config, u32 cond)
{
	cellSysutil.todo("cellWebBrowserConfigSetViewCondition2(config=*0x%x, cond=0x%x)", config, cond);
	return CELL_OK;
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

error_code cellWebBrowserEstimate2(vm::cptr<CellWebBrowserConfig2> config, vm::ptr<u32> memSize)
{
	cellSysutil.warning("cellWebBrowserEstimate2(config=*0x%x, memSize=*0x%x)", config, memSize);

	// TODO: When cellWebBrowser stuff is implemented, change this to some real
	// needed memory buffer size.
	*memSize = 1 * 1024 * 1024; // 1 MB
	return CELL_OK;
}

error_code cellWebBrowserGetUsrdataOnGameExit(vm::ptr<CellWebBrowserUsrdata> ptr)
{
	cellSysutil.todo("cellWebBrowserGetUsrdataOnGameExit(ptr=*0x%x)", ptr);
	return CELL_OK;
}

error_code cellWebBrowserInitialize(vm::ptr<CellWebBrowserSystemCallback> system_cb, u32 container)
{
	cellSysutil.todo("cellWebBrowserInitialize(system_cb=*0x%x, container=0x%x)", system_cb, container);

	const auto browser = fxm::make_always<browser_t>();
	browser->system_cb = system_cb;

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		system_cb(ppu, CELL_SYSUTIL_WEBBROWSER_INITIALIZING_FINISHED, browser->userData);
		return CELL_OK;
	});

	return CELL_OK;
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

void cellWebBrowserShutdown()
{
	cellSysutil.todo("cellWebBrowserShutdown()");

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		const auto browser = fxm::get_always<browser_t>();

		browser->system_cb(ppu, CELL_SYSUTIL_WEBBROWSER_SHUTDOWN_FINISHED, browser->userData);
		return CELL_OK;
	});
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
