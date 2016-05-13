#include "stdafx.h"
#include "Utilities/Config.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

#include "cellSysutil.h"
#include "cellNetCtl.h"

#include "Utilities/StrUtil.h"

logs::channel cellNetCtl("cellNetCtl", logs::level::notice);

cfg::map_entry<s32> g_cfg_net_status(cfg::root.net, "Connection status",
{
	{ "Disconnected", CELL_NET_CTL_STATE_Disconnected },
	{ "Connecting", CELL_NET_CTL_STATE_Connecting },
	{ "Obtaining IP", CELL_NET_CTL_STATE_IPObtaining },
	{ "IP Obtained", CELL_NET_CTL_STATE_IPObtained },
});

cfg::string_entry g_cfg_net_ip_address(cfg::root.net, "IP address", "192.168.1.1");

s32 cellNetCtlInit()
{
	cellNetCtl.warning("cellNetCtlInit()");

	return CELL_OK;
}

s32 cellNetCtlTerm()
{
	cellNetCtl.warning("cellNetCtlTerm()");

	return CELL_OK;
}

s32 cellNetCtlGetState(vm::ptr<u32> state)
{
	cellNetCtl.trace("cellNetCtlGetState(state=*0x%x)", state);

	*state = g_cfg_net_status.get();
	return CELL_OK;
}

s32 cellNetCtlAddHandler(vm::ptr<cellNetCtlHandler> handler, vm::ptr<void> arg, vm::ptr<s32> hid)
{
	cellNetCtl.todo("cellNetCtlAddHandler(handler=*0x%x, arg=*0x%x, hid=*0x%x)", handler, arg, hid);

	return CELL_OK;
}

s32 cellNetCtlDelHandler(s32 hid)
{
	cellNetCtl.todo("cellNetCtlDelHandler(hid=0x%x)", hid);

	return CELL_OK;
}

s32 cellNetCtlGetInfo(s32 code, vm::ptr<CellNetCtlInfo> info)
{
	cellNetCtl.todo("cellNetCtlGetInfo(code=0x%x (%s), info=*0x%x)", code, InfoCodeToName(code), info);

	if (code == CELL_NET_CTL_INFO_MTU)
	{
		info->mtu = 1500;
	}
	else if (code == CELL_NET_CTL_INFO_LINK)
	{
		if (g_cfg_net_status.get() != CELL_NET_CTL_STATE_Disconnected)
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
		if (g_cfg_net_status.get() != CELL_NET_CTL_STATE_IPObtained)
		{
			// 0.0.0.0 seems to be the default address when no ethernet cables are connected to the PS3
			strcpy_trunc(info->ip_address, "0.0.0.0");
		}
		else
		{
			strcpy_trunc(info->ip_address, g_cfg_net_ip_address);
		}
	}
	else if (code == CELL_NET_CTL_INFO_NETMASK)
	{
		strcpy_trunc(info->netmask, "255.255.255.255");
	}

	return CELL_OK;
}

s32 cellNetCtlNetStartDialogLoadAsync(vm::ptr<CellNetCtlNetStartDialogParam> param)
{
	cellNetCtl.error("cellNetCtlNetStartDialogLoadAsync(param=*0x%x)", param);

	// TODO: Actually sign into PSN or an emulated network similar to PSN (ESN)
	// TODO: Properly open the dialog prompt for sign in
	sysutilSendSystemCommand(CELL_SYSUTIL_NET_CTL_NETSTART_LOADED, 0);
	sysutilSendSystemCommand(CELL_SYSUTIL_NET_CTL_NETSTART_FINISHED, 0);

	return CELL_OK;
}

s32 cellNetCtlNetStartDialogAbortAsync()
{
	cellNetCtl.error("cellNetCtlNetStartDialogAbortAsync()");

	return CELL_OK;
}

s32 cellNetCtlNetStartDialogUnloadAsync(vm::ptr<CellNetCtlNetStartDialogResult> result)
{
	cellNetCtl.warning("cellNetCtlNetStartDialogUnloadAsync(result=*0x%x)", result);

	result->result = CELL_NET_CTL_ERROR_DIALOG_CANCELED;
	sysutilSendSystemCommand(CELL_SYSUTIL_NET_CTL_NETSTART_UNLOADED, 0);

	return CELL_OK;
}

s32 cellNetCtlGetNatInfo(vm::ptr<CellNetCtlNatInfo> natInfo)
{
	cellNetCtl.todo("cellNetCtlGetNatInfo(natInfo=*0x%x)", natInfo);

	if (natInfo->size == 0)
	{
		cellNetCtl.error("cellNetCtlGetNatInfo : CELL_NET_CTL_ERROR_INVALID_SIZE");
		return CELL_NET_CTL_ERROR_INVALID_SIZE;
	}
	
	return CELL_OK;
}

s32 cellGameUpdateInit()
{
	throw EXCEPTION("");
}

s32 cellGameUpdateTerm()
{
	throw EXCEPTION("");
}


s32 cellGameUpdateCheckStartAsync()
{
	throw EXCEPTION("");
}

s32 cellGameUpdateCheckFinishAsync()
{
	throw EXCEPTION("");
}

s32 cellGameUpdateCheckStartWithoutDialogAsync()
{
	throw EXCEPTION("");
}

s32 cellGameUpdateCheckAbort()
{
	throw EXCEPTION("");
}

s32 cellGameUpdateCheckStartAsyncEx()
{
	throw EXCEPTION("");
}

s32 cellGameUpdateCheckFinishAsyncEx()
{
	throw EXCEPTION("");
}

s32 cellGameUpdateCheckStartWithoutDialogAsyncEx()
{
	throw EXCEPTION("");
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
