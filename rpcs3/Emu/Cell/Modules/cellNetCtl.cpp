#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

#include "cellGame.h"
#include "cellSysutil.h"
#include "cellNetCtl.h"

#include "Utilities/StrUtil.h"

logs::channel cellNetCtl("cellNetCtl");

template <>
void fmt_class_string<CellNetCtlError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_NET_CTL_ERROR_NOT_INITIALIZED);
			STR_CASE(CELL_NET_CTL_ERROR_NOT_TERMINATED);
			STR_CASE(CELL_NET_CTL_ERROR_HANDLER_MAX);
			STR_CASE(CELL_NET_CTL_ERROR_ID_NOT_FOUND);
			STR_CASE(CELL_NET_CTL_ERROR_INVALID_ID);
			STR_CASE(CELL_NET_CTL_ERROR_INVALID_CODE);
			STR_CASE(CELL_NET_CTL_ERROR_INVALID_ADDR);
			STR_CASE(CELL_NET_CTL_ERROR_NOT_CONNECTED);
			STR_CASE(CELL_NET_CTL_ERROR_NOT_AVAIL);
			STR_CASE(CELL_NET_CTL_ERROR_INVALID_TYPE);
			STR_CASE(CELL_NET_CTL_ERROR_INVALID_SIZE);
			STR_CASE(CELL_NET_CTL_ERROR_NET_DISABLED);
			STR_CASE(CELL_NET_CTL_ERROR_NET_NOT_CONNECTED);
			STR_CASE(CELL_NET_CTL_ERROR_NP_NO_ACCOUNT);
			STR_CASE(CELL_NET_CTL_ERROR_NP_RESERVED1);
			STR_CASE(CELL_NET_CTL_ERROR_NP_RESERVED2);
			STR_CASE(CELL_NET_CTL_ERROR_NET_CABLE_NOT_CONNECTED);
			STR_CASE(CELL_NET_CTL_ERROR_DIALOG_CANCELED);
			STR_CASE(CELL_NET_CTL_ERROR_DIALOG_ABORTED);

			STR_CASE(CELL_NET_CTL_ERROR_WLAN_DEAUTHED);
			STR_CASE(CELL_NET_CTL_ERROR_WLAN_KEYINFO_EXCHNAGE_TIMEOUT);
			STR_CASE(CELL_NET_CTL_ERROR_WLAN_ASSOC_FAILED);
			STR_CASE(CELL_NET_CTL_ERROR_WLAN_AP_DISAPPEARED);
			STR_CASE(CELL_NET_CTL_ERROR_PPPOE_SESSION_INIT);
			STR_CASE(CELL_NET_CTL_ERROR_PPPOE_SESSION_NO_PADO);
			STR_CASE(CELL_NET_CTL_ERROR_PPPOE_SESSION_NO_PADS);
			STR_CASE(CELL_NET_CTL_ERROR_PPPOE_SESSION_GET_PADT);
			STR_CASE(CELL_NET_CTL_ERROR_PPPOE_SESSION_SERVICE_NAME);
			STR_CASE(CELL_NET_CTL_ERROR_PPPOE_SESSION_AC_SYSTEM);
			STR_CASE(CELL_NET_CTL_ERROR_PPPOE_SESSION_GENERIC);
			STR_CASE(CELL_NET_CTL_ERROR_PPPOE_STATUS_AUTH);
			STR_CASE(CELL_NET_CTL_ERROR_PPPOE_STATUS_NETWORK);
			STR_CASE(CELL_NET_CTL_ERROR_PPPOE_STATUS_TERMINATE);
			STR_CASE(CELL_NET_CTL_ERROR_DHCP_LEASE_TIME);

			STR_CASE(CELL_GAMEUPDATE_ERROR_NOT_INITIALIZED);
			STR_CASE(CELL_GAMEUPDATE_ERROR_ALREADY_INITIALIZED);
			STR_CASE(CELL_GAMEUPDATE_ERROR_INVALID_ADDR);
			STR_CASE(CELL_GAMEUPDATE_ERROR_INVALID_SIZE);
			STR_CASE(CELL_GAMEUPDATE_ERROR_INVALID_MEMORY_CONTAINER);
			STR_CASE(CELL_GAMEUPDATE_ERROR_INSUFFICIENT_MEMORY_CONTAINER);
			STR_CASE(CELL_GAMEUPDATE_ERROR_BUSY);
			STR_CASE(CELL_GAMEUPDATE_ERROR_NOT_START);
			STR_CASE(CELL_GAMEUPDATE_ERROR_LOAD_FAILED);
		}

		return unknown;
	});
}

template <>
void fmt_class_string<CellNetCtlState>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](CellNetCtlState value)
	{
		switch (value)
		{
		case CELL_NET_CTL_STATE_Disconnected: return "Disconnected";
		case CELL_NET_CTL_STATE_Connecting: return "Connecting";
		case CELL_NET_CTL_STATE_IPObtaining: return "Obtaining IP";
		case CELL_NET_CTL_STATE_IPObtained: return "IP Obtained";
		}

		return unknown;
	});
}

error_code cellNetCtlInit()
{
	cellNetCtl.warning("cellNetCtlInit()");

	return CELL_OK;
}

error_code cellNetCtlTerm()
{
	cellNetCtl.warning("cellNetCtlTerm()");

	return CELL_OK;
}

error_code cellNetCtlGetState(vm::ptr<u32> state)
{
	cellNetCtl.trace("cellNetCtlGetState(state=*0x%x)", state);

	*state = g_cfg.net.net_status;
	return CELL_OK;
}

error_code cellNetCtlAddHandler(vm::ptr<cellNetCtlHandler> handler, vm::ptr<void> arg, vm::ptr<s32> hid)
{
	cellNetCtl.todo("cellNetCtlAddHandler(handler=*0x%x, arg=*0x%x, hid=*0x%x)", handler, arg, hid);

	return CELL_OK;
}

error_code cellNetCtlDelHandler(s32 hid)
{
	cellNetCtl.todo("cellNetCtlDelHandler(hid=0x%x)", hid);

	return CELL_OK;
}

error_code cellNetCtlGetInfo(s32 code, vm::ptr<CellNetCtlInfo> info)
{
	cellNetCtl.todo("cellNetCtlGetInfo(code=0x%x (%s), info=*0x%x)", code, InfoCodeToName(code), info);

	if (code == CELL_NET_CTL_INFO_MTU)
	{
		info->mtu = 1500;
	}
	else if (code == CELL_NET_CTL_INFO_LINK)
	{
		if (g_cfg.net.net_status != CELL_NET_CTL_STATE_Disconnected)
		{
			info->link = CELL_NET_CTL_LINK_CONNECTED;
		}
		else
		{
			info->link = CELL_NET_CTL_LINK_DISCONNECTED;
		}
	}
	else if (code == CELL_NET_CTL_INFO_IP_ADDRESS)
	{
		if (g_cfg.net.net_status != CELL_NET_CTL_STATE_IPObtained)
		{
			// 0.0.0.0 seems to be the default address when no ethernet cables are connected to the PS3
			strcpy_trunc(info->ip_address, "0.0.0.0");
		}
		else
		{
			strcpy_trunc(info->ip_address, g_cfg.net.ip_address);
		}
	}
	else if (code == CELL_NET_CTL_INFO_NETMASK)
	{
		strcpy_trunc(info->netmask, "255.255.255.255");
	}
	else if (code == CELL_NET_CTL_INFO_HTTP_PROXY_CONFIG)
	{
		info->http_proxy_config = 0;
	}

	return CELL_OK;
}

error_code cellNetCtlNetStartDialogLoadAsync(vm::ptr<CellNetCtlNetStartDialogParam> param)
{
	cellNetCtl.error("cellNetCtlNetStartDialogLoadAsync(param=*0x%x)", param);

	// TODO: Actually sign into PSN or an emulated network similar to PSN (ESN)
	// TODO: Properly open the dialog prompt for sign in
	sysutil_send_system_cmd(CELL_SYSUTIL_NET_CTL_NETSTART_LOADED, 0);
	sysutil_send_system_cmd(CELL_SYSUTIL_NET_CTL_NETSTART_FINISHED, 0);

	return CELL_OK;
}

error_code cellNetCtlNetStartDialogAbortAsync()
{
	cellNetCtl.error("cellNetCtlNetStartDialogAbortAsync()");

	return CELL_OK;
}

error_code cellNetCtlNetStartDialogUnloadAsync(vm::ptr<CellNetCtlNetStartDialogResult> result)
{
	cellNetCtl.warning("cellNetCtlNetStartDialogUnloadAsync(result=*0x%x)", result);

	result->result = CELL_NET_CTL_ERROR_DIALOG_CANCELED;
	sysutil_send_system_cmd(CELL_SYSUTIL_NET_CTL_NETSTART_UNLOADED, 0);

	return CELL_OK;
}

error_code cellNetCtlGetNatInfo(vm::ptr<CellNetCtlNatInfo> natInfo)
{
	cellNetCtl.todo("cellNetCtlGetNatInfo(natInfo=*0x%x)", natInfo);

	if (natInfo->size == 0)
	{
		cellNetCtl.error("cellNetCtlGetNatInfo : CELL_NET_CTL_ERROR_INVALID_SIZE");
		return CELL_NET_CTL_ERROR_INVALID_SIZE;
	}

	return CELL_OK;
}

error_code cellNetCtlAddHandlerGameInt()
{
	cellNetCtl.todo("cellNetCtlAddHandlerGameInt()");
	return CELL_OK;
}

error_code cellNetCtlConnectGameInt()
{
	cellNetCtl.todo("cellNetCtlConnectGameInt()");
	return CELL_OK;
}

error_code cellNetCtlDelHandlerGameInt()
{
	cellNetCtl.todo("cellNetCtlDelHandlerGameInt()");
	return CELL_OK;
}

error_code cellNetCtlDisconnectGameInt()
{
	cellNetCtl.todo("cellNetCtlDisconnectGameInt()");
	return CELL_OK;
}

error_code cellNetCtlGetInfoGameInt()
{
	cellNetCtl.todo("cellNetCtlGetInfoGameInt()");
	return CELL_OK;
}

error_code cellNetCtlGetScanInfoGameInt()
{
	cellNetCtl.todo("cellNetCtlGetScanInfoGameInt()");
	return CELL_OK;
}

error_code cellNetCtlGetStateGameInt()
{
	cellNetCtl.todo("cellNetCtlGetStateGameInt()");
	return CELL_OK;
}

error_code cellNetCtlScanGameInt()
{
	cellNetCtl.todo("cellNetCtlScanGameInt()");
	return CELL_OK;
}

error_code cellGameUpdateInit()
{
	cellNetCtl.todo("cellGameUpdateInit()");
	return CELL_OK;
}

error_code cellGameUpdateTerm()
{
	cellNetCtl.todo("cellGameUpdateTerm()");
	return CELL_OK;
}

error_code cellGameUpdateCheckStartAsync(ppu_thread& ppu, vm::cptr<CellGameUpdateParam> param, vm::ptr<CellGameUpdateCallback> cb_func, vm::ptr<void> userdata)
{
	cellNetCtl.todo("cellGameUpdateCheckStartAsync(param=*0x%x, cb_func=*0x%x, userdata=*0x%x)", param, cb_func, userdata);
	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		cb_func(ppu, CELL_OK, CELL_OK, userdata);
		return CELL_OK;
	});
	return CELL_OK;
}

error_code cellGameUpdateCheckFinishAsync(ppu_thread& ppu, vm::ptr<CellGameUpdateCallback> cb_func, vm::ptr<void> userdata)
{
	cellNetCtl.todo("cellGameUpdateCheckFinishAsync(cb_func=*0x%x, userdata=*0x%x)", cb_func, userdata);
	const u32 PROCESSING_COMPLETE = 5;
	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		cb_func(ppu, PROCESSING_COMPLETE, CELL_OK, userdata);
		return CELL_OK;
	});
	return CELL_OK;
}

error_code cellGameUpdateCheckStartWithoutDialogAsync(vm::ptr<CellGameUpdateCallback> cb_func, vm::ptr<void> userdata)
{
	cellNetCtl.todo("cellGameUpdateCheckStartWithoutDialogAsync(cb_func=*0x%x, userdata=*0x%x)", cb_func, userdata);
	return CELL_OK;
}

error_code cellGameUpdateCheckAbort()
{
	cellNetCtl.todo("cellGameUpdateCheckAbort()");
	return CELL_OK;
}

error_code cellGameUpdateCheckStartAsyncEx(vm::cptr<CellGameUpdateParam> param, vm::ptr<CellGameUpdateCallbackEx> cb_func, vm::ptr<void> userdata)
{
	cellNetCtl.todo("cellGameUpdateCheckStartAsyncEx(param=*0x%x, cb_func=*0x%x, userdata=*0x%x)", param, cb_func, userdata);
	return CELL_OK;
}

error_code cellGameUpdateCheckFinishAsyncEx(vm::ptr<CellGameUpdateCallbackEx> cb_func, vm::ptr<void> userdata)
{
	cellNetCtl.todo("cellGameUpdateCheckFinishAsyncEx(cb_func=*0x%x, userdata=*0x%x)", cb_func, userdata);
	return CELL_OK;
}

error_code cellGameUpdateCheckStartWithoutDialogAsyncEx(vm::ptr<CellGameUpdateCallbackEx> cb_func, vm::ptr<void> userdata)
{
	cellNetCtl.todo("cellGameUpdateCheckStartWithoutDialogAsyncEx(cb_func=*0x%x, userdata=*0x%x)", cb_func, userdata);
	return CELL_OK;
}


DECLARE(ppu_module_manager::cellNetCtl)("cellNetCtl", []()
{
	REG_FUNC(cellNetCtl, cellNetCtlInit);
	REG_FUNC(cellNetCtl, cellNetCtlTerm);

	REG_FUNC(cellNetCtl, cellNetCtlGetState);
	REG_FUNC(cellNetCtl, cellNetCtlAddHandler);
	REG_FUNC(cellNetCtl, cellNetCtlDelHandler);

	REG_FUNC(cellNetCtl, cellNetCtlGetInfo);

	REG_FUNC(cellNetCtl, cellNetCtlNetStartDialogLoadAsync);
	REG_FUNC(cellNetCtl, cellNetCtlNetStartDialogAbortAsync);
	REG_FUNC(cellNetCtl, cellNetCtlNetStartDialogUnloadAsync);

	REG_FUNC(cellNetCtl, cellNetCtlGetNatInfo);

	REG_FUNC(cellNetCtl, cellNetCtlAddHandlerGameInt);
	REG_FUNC(cellNetCtl, cellNetCtlConnectGameInt);
	REG_FUNC(cellNetCtl, cellNetCtlDelHandlerGameInt);
	REG_FUNC(cellNetCtl, cellNetCtlDisconnectGameInt);
	REG_FUNC(cellNetCtl, cellNetCtlGetInfoGameInt);
	REG_FUNC(cellNetCtl, cellNetCtlGetScanInfoGameInt);
	REG_FUNC(cellNetCtl, cellNetCtlGetStateGameInt);
	REG_FUNC(cellNetCtl, cellNetCtlScanGameInt);

	REG_FUNC(cellNetCtl, cellGameUpdateInit);
	REG_FUNC(cellNetCtl, cellGameUpdateTerm);

	REG_FUNC(cellNetCtl, cellGameUpdateCheckStartAsync);
	REG_FUNC(cellNetCtl, cellGameUpdateCheckFinishAsync);
	REG_FUNC(cellNetCtl, cellGameUpdateCheckStartWithoutDialogAsync);
	REG_FUNC(cellNetCtl, cellGameUpdateCheckAbort);
	REG_FUNC(cellNetCtl, cellGameUpdateCheckStartAsyncEx);
	REG_FUNC(cellNetCtl, cellGameUpdateCheckFinishAsyncEx);
	REG_FUNC(cellNetCtl, cellGameUpdateCheckStartWithoutDialogAsyncEx);
});
