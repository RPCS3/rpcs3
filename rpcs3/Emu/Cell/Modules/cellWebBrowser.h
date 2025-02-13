#pragma once

#include "Emu/Memory/vm_ptr.h"

//events
enum CellWebBrowserEvent : s32
{
	CELL_SYSUTIL_WEBBROWSER_INITIALIZING_FINISHED	= 1,
	CELL_SYSUTIL_WEBBROWSER_SHUTDOWN_FINISHED		= 4,
	CELL_SYSUTIL_WEBBROWSER_LOADING_FINISHED		= 5,
	CELL_SYSUTIL_WEBBROWSER_UNLOADING_FINISHED		= 7,
	CELL_SYSUTIL_WEBBROWSER_RELEASED				= 9,
	CELL_SYSUTIL_WEBBROWSER_GRABBED					= 11,
};

using CellWebBrowserCallback = void(s32 cb_type, vm::ptr<void> client_session, vm::ptr<void> usrdata);
using CellWebComponentCallback = void(s32 web_browser_id, s32 cb_type, vm::ptr<void> client_session, vm::ptr<void> usrdata);
using CellWebBrowserSystemCallback = void(s32 cb_type, vm::ptr<void> usrdata);

using CellWebBrowserMIMETypeCallback = void(vm::cptr<char> mimetype, vm::cptr<char> url, vm::ptr<void> usrdata);
using CellWebBrowserErrorCallback = void(s32 err_type, vm::ptr<void> usrdata);
using CellWebBrowserStatusCallback = void(s32 err_type, vm::ptr<void> usrdata);
using CellWebBrowserNotify = void(vm::cptr<char> message, vm::ptr<void> usrdata);
using CellWebBrowserUsrdata = void(vm::ptr<void> usrdata);

struct CellWebBrowserMimeSet
{
	vm::bcptr<char> type;
	vm::bcptr<char> directory;
};

struct CellWebBrowserPos
{
	be_t<s32> x;
	be_t<s32> y;
};

struct CellWebBrowserSize
{
	be_t<s32> width;
	be_t<s32> height;
};

struct CellWebBrowserRect
{
	CellWebBrowserPos pos;
	CellWebBrowserSize size;
};

struct CellWebBrowserConfig
{
	be_t<s32> version;
	be_t<s32> heap_size;
	vm::bcptr<CellWebBrowserMimeSet> mimesets;
	be_t<s32> mimeset_num;
	be_t<s32> functions;
	be_t<s32> tab_count;
	vm::bptr<CellWebBrowserCallback> exit_cb;
	vm::bptr<CellWebBrowserCallback> download_cb;
	vm::bptr<CellWebBrowserCallback> navigated_cb;
};

struct CellWebBrowserConfig2
{
	be_t<s32> version;
	be_t<s32> heap_size;
	be_t<s32> functions;
	be_t<s32> tab_count;
	be_t<s32> size_mode;
	be_t<s32> view_restriction;
	vm::bptr<CellWebBrowserMIMETypeCallback> unknown_mimetype_cb;
	vm::bptr<CellWebBrowserErrorCallback> error_cb;
	vm::bptr<CellWebBrowserStatusCallback> status_error_cb;
	vm::bptr<CellWebBrowserNotify> notify_cb;
	vm::bptr<CellWebBrowserCallback> request_cb;
	CellWebBrowserRect rect;
	be_t<f32> resolution_factor;
	be_t<s32> magic_number_;
};
