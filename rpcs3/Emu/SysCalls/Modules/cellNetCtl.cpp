#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "rpcs3/Ini.h"
#include "cellSysutil.h"
#include "cellNetCtl.h"

extern Module cellNetCtl;

struct cellNetCtlInternal
{
	bool m_bInitialized;

	cellNetCtlInternal()
		: m_bInitialized(false)
	{
	}
};

cellNetCtlInternal cellNetCtlInstance;

int cellNetCtlInit()
{
	cellNetCtl.Warning("cellNetCtlInit()");

	if (cellNetCtlInstance.m_bInitialized)
		return CELL_NET_CTL_ERROR_NOT_TERMINATED;

	cellNetCtlInstance.m_bInitialized = true;

	return CELL_OK;
}

int cellNetCtlTerm()
{
	cellNetCtl.Warning("cellNetCtlTerm()");

	if (!cellNetCtlInstance.m_bInitialized)
		return CELL_NET_CTL_ERROR_NOT_INITIALIZED;

	cellNetCtlInstance.m_bInitialized = false;

	return CELL_OK;
}

int cellNetCtlGetState(vm::ptr<u32> state)
{
	cellNetCtl.Warning("cellNetCtlGetState(state_addr=0x%x)", state.addr());

	// Do we need to allow any other connection states?
	if (Ini.Connected.GetValue())
	{
		*state = CELL_NET_CTL_STATE_IPObtained;
	}
	else
	{
		*state = CELL_NET_CTL_STATE_Disconnected;
	}

	return CELL_OK;
}

int cellNetCtlAddHandler(vm::ptr<cellNetCtlHandler> handler, vm::ptr<void> arg, vm::ptr<s32> hid)
{
	cellNetCtl.Todo("cellNetCtlAddHandler(handler_addr=0x%x, arg_addr=0x%x, hid_addr=0x%x)", handler.addr(), arg.addr(), hid.addr());

	return CELL_OK;
}

int cellNetCtlDelHandler(s32 hid)
{
	cellNetCtl.Todo("cellNetCtlDelHandler(hid=0x%x)", hid);

	return CELL_OK;
}

int cellNetCtlGetInfo(s32 code, vm::ptr<CellNetCtlInfo> info)
{
	cellNetCtl.Todo("cellNetCtlGetInfo(code=0x%x, info_addr=0x%x)", code, info.addr());

	if (code == CELL_NET_CTL_INFO_IP_ADDRESS)
	{
		strcpy_trunc(info->ip_address, "192.168.1.1");
	}

	return CELL_OK;
}

int cellNetCtlNetStartDialogLoadAsync(vm::ptr<CellNetCtlNetStartDialogParam> param)
{
	cellNetCtl.Warning("cellNetCtlNetStartDialogLoadAsync(param_addr=0x%x)", param.addr());

	// TODO: Actually sign into PSN
	sysutilSendSystemCommand(CELL_SYSUTIL_NET_CTL_NETSTART_FINISHED, 0);

	return CELL_OK;
}

int cellNetCtlNetStartDialogAbortAsync()
{
	cellNetCtl.Todo("cellNetCtlNetStartDialogAbortAsync()");

	return CELL_OK;
}

int cellNetCtlNetStartDialogUnloadAsync(vm::ptr<CellNetCtlNetStartDialogResult> result)
{
	cellNetCtl.Warning("cellNetCtlNetStartDialogUnloadAsync(result_addr=0x%x)", result.addr());

	sysutilSendSystemCommand(CELL_SYSUTIL_NET_CTL_NETSTART_UNLOADED, 0);

	return CELL_OK;
}

int cellNetCtlGetNatInfo(vm::ptr<CellNetCtlNatInfo> natInfo)
{
	cellNetCtl.Todo("cellNetCtlGetNatInfo(natInfo_addr=0x%x)", natInfo.addr());

	if (natInfo->size == 0)
	{
		cellNetCtl.Error("cellNetCtlGetNatInfo : CELL_NET_CTL_ERROR_INVALID_SIZE");
		return CELL_NET_CTL_ERROR_INVALID_SIZE;
	}
	
	return CELL_OK;
}

Module cellNetCtl("cellNetCtl", []()
{
	cellNetCtlInstance.m_bInitialized = false;

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
});
