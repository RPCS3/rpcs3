#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "cellWebBrowser.h"
#include "Emu/IdManager.h"

#include "cellSysutil.h"

LOG_CHANNEL(cellSysutil);

struct browser_info
{
	vm::ptr<CellWebBrowserSystemCallback> system_cb{};
	vm::ptr<void> userData{};

	shared_mutex mutex;
};

error_code cellWebBrowserActivate()
{
	cellSysutil.todo("cellWebBrowserActivate()");
	return CELL_OK;
}

error_code cellWebBrowserConfig()
{
	cellSysutil.todo("cellWebBrowserConfig()");
	return CELL_OK;
}

error_code cellWebBrowserConfig2(vm::cptr<CellWebBrowserConfig2> config, u32 version)
{
	cellSysutil.todo("cellWebBrowserConfig2(config=*0x%x, version=%d)", config, version);
	return CELL_OK;
}

error_code cellWebBrowserConfigGetHeapSize()
{
	cellSysutil.todo("cellWebBrowserConfigGetHeapSize()");
	return CELL_OK;
}

error_code cellWebBrowserConfigGetHeapSize2()
{
	cellSysutil.todo("cellWebBrowserConfigGetHeapSize2()");
	return CELL_OK;
}

error_code cellWebBrowserConfigSetCustomExit()
{
	cellSysutil.todo("cellWebBrowserConfigSetCustomExit()");
	return CELL_OK;
}

error_code cellWebBrowserConfigSetDisableTabs()
{
	cellSysutil.todo("cellWebBrowserConfigSetDisableTabs()");
	return CELL_OK;
}

error_code cellWebBrowserConfigSetErrorHook2()
{
	cellSysutil.todo("cellWebBrowserConfigSetErrorHook2()");
	return CELL_OK;
}

error_code cellWebBrowserConfigSetFullScreen2(vm::cptr<CellWebBrowserConfig2> config, u32 full)
{
	cellSysutil.todo("cellWebBrowserConfigSetFullScreen2(config=*0x%x, full=%d)", config, full);
	return CELL_OK;
}

error_code cellWebBrowserConfigSetFullVersion2()
{
	cellSysutil.todo("cellWebBrowserConfigSetFullVersion2()");
	return CELL_OK;
}

error_code cellWebBrowserConfigSetFunction()
{
	cellSysutil.todo("cellWebBrowserConfigSetFunction()");
	return CELL_OK;
}

error_code cellWebBrowserConfigSetFunction2(vm::ptr<CellWebBrowserConfig2> config, u32 funcset)
{
	cellSysutil.todo("cellWebBrowserConfigSetFunction2(config=*0x%x, funcset=0x%x)", config, funcset);
	return CELL_OK;
}

error_code cellWebBrowserConfigSetHeapSize()
{
	cellSysutil.todo("cellWebBrowserConfigSetHeapSize()");
	return CELL_OK;
}

error_code cellWebBrowserConfigSetHeapSize2(vm::ptr<CellWebBrowserConfig2> config, u32 size)
{
	cellSysutil.todo("cellWebBrowserConfigSetHeapSize(config=*0x%x, size=0x%x)", config, size);
	return CELL_OK;
}

error_code cellWebBrowserConfigSetMimeSet()
{
	cellSysutil.todo("cellWebBrowserConfigSetMimeSet()");
	return CELL_OK;
}

error_code cellWebBrowserConfigSetNotifyHook2(vm::cptr<CellWebBrowserConfig2> config, vm::ptr<CellWebBrowserNotify> cb, vm::ptr<void> userdata)
{
	cellSysutil.todo("cellWebBrowserConfigSetNotifyHook2(config=*0x%x, cb=*0x%x, userdata=*0x%x)", config, cb, userdata);
	return CELL_OK;
}

error_code cellWebBrowserConfigSetRequestHook2()
{
	cellSysutil.todo("cellWebBrowserConfigSetRequestHook2()");
	return CELL_OK;
}

error_code cellWebBrowserConfigSetStatusHook2()
{
	cellSysutil.todo("cellWebBrowserConfigSetStatusHook2()");
	return CELL_OK;
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

error_code cellWebBrowserConfigSetVersion()
{
	cellSysutil.todo("cellWebBrowserConfigSetVersion()");
	return CELL_OK;
}

error_code cellWebBrowserConfigSetViewCondition2(vm::ptr<CellWebBrowserConfig2> config, u32 cond)
{
	cellSysutil.todo("cellWebBrowserConfigSetViewCondition2(config=*0x%x, cond=0x%x)", config, cond);
	return CELL_OK;
}

error_code cellWebBrowserConfigSetViewRect2()
{
	cellSysutil.todo("cellWebBrowserConfigSetViewRect2()");
	return CELL_OK;
}

error_code cellWebBrowserConfigWithVer()
{
	cellSysutil.todo("cellWebBrowserConfigWithVer()");
	return CELL_OK;
}

error_code cellWebBrowserCreate()
{
	cellSysutil.todo("cellWebBrowserCreate()");
	return CELL_OK;
}

error_code cellWebBrowserCreate2()
{
	cellSysutil.todo("cellWebBrowserCreate2()");
	return CELL_OK;
}

error_code cellWebBrowserCreateRender2()
{
	cellSysutil.todo("cellWebBrowserCreateRender2()");
	return CELL_OK;
}

error_code cellWebBrowserCreateRenderWithRect2()
{
	cellSysutil.todo("cellWebBrowserCreateRenderWithRect2()");
	return CELL_OK;
}

error_code cellWebBrowserCreateWithConfig()
{
	cellSysutil.todo("cellWebBrowserCreateWithConfig()");
	return CELL_OK;
}

error_code cellWebBrowserCreateWithConfigFull()
{
	cellSysutil.todo("cellWebBrowserCreateWithConfigFull()");
	return CELL_OK;
}

error_code cellWebBrowserCreateWithRect2()
{
	cellSysutil.todo("cellWebBrowserCreateWithRect2()");
	return CELL_OK;
}

error_code cellWebBrowserDeactivate()
{
	cellSysutil.todo("cellWebBrowserDeactivate()");
	return CELL_OK;
}

error_code cellWebBrowserDestroy()
{
	cellSysutil.todo("cellWebBrowserDestroy()");
	return CELL_OK;
}

error_code cellWebBrowserDestroy2()
{
	cellSysutil.todo("cellWebBrowserDestroy2()");
	return CELL_OK;
}

error_code cellWebBrowserEstimate()
{
	cellSysutil.todo("cellWebBrowserEstimate()");
	return CELL_OK;
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

	auto& browser = g_fxo->get<browser_info>();
	browser.system_cb = system_cb;

	sysutil_register_cb([=, &browser](ppu_thread& ppu) -> s32
	{
		system_cb(ppu, CELL_SYSUTIL_WEBBROWSER_INITIALIZING_FINISHED, browser.userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellWebBrowserNavigate2()
{
	cellSysutil.todo("cellWebBrowserNavigate2()");
	return CELL_OK;
}

error_code cellWebBrowserSetLocalContentsAdditionalTitleID()
{
	cellSysutil.todo("cellWebBrowserSetLocalContentsAdditionalTitleID()");
	return CELL_OK;
}

error_code cellWebBrowserSetSystemCallbackUsrdata()
{
	cellSysutil.todo("cellWebBrowserSetSystemCallbackUsrdata()");
	return CELL_OK;
}

void cellWebBrowserShutdown()
{
	cellSysutil.todo("cellWebBrowserShutdown()");

	auto& browser = g_fxo->get<browser_info>();

	sysutil_register_cb([=, &browser](ppu_thread& ppu) -> s32
	{
		browser.system_cb(ppu, CELL_SYSUTIL_WEBBROWSER_SHUTDOWN_FINISHED, browser.userData);
		return CELL_OK;
	});
}

error_code cellWebBrowserUpdatePointerDisplayPos2()
{
	cellSysutil.todo("cellWebBrowserUpdatePointerDisplayPos2()");
	return CELL_OK;
}

error_code cellWebBrowserWakeupWithGameExit()
{
	cellSysutil.todo("cellWebBrowserWakeupWithGameExit()");
	return CELL_OK;
}

error_code cellWebComponentCreate()
{
	cellSysutil.todo("cellWebComponentCreate()");
	return CELL_OK;
}

error_code cellWebComponentCreateAsync()
{
	cellSysutil.todo("cellWebComponentCreateAsync()");
	return CELL_OK;
}

error_code cellWebComponentDestroy()
{
	cellSysutil.todo("cellWebComponentDestroy()");
	return CELL_OK;
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
