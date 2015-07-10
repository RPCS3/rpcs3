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

s32 cellNetCtlInit()
{
	cellNetCtl.Log("cellNetCtlInit()");

	if (cellNetCtlInstance.m_bInitialized)
		return CELL_NET_CTL_ERROR_NOT_TERMINATED;

	cellNetCtlInstance.m_bInitialized = true;

	return CELL_OK;
}

s32 cellNetCtlTerm()
{
	cellNetCtl.Log("cellNetCtlTerm()");

	if (!cellNetCtlInstance.m_bInitialized)
		return CELL_NET_CTL_ERROR_NOT_INITIALIZED;

	cellNetCtlInstance.m_bInitialized = false;

	return CELL_OK;
}

s32 cellNetCtlGetState(vm::ptr<u32> state)
{
	cellNetCtl.Log("cellNetCtlGetState(state=*0x%x)", state);

	if (Ini.NETStatus.GetValue() == 0)
	{
		*state = CELL_NET_CTL_STATE_IPObtained;
	}
	else if (Ini.NETStatus.GetValue() == 1)
	{
		*state = CELL_NET_CTL_STATE_IPObtaining;
	}
	else if (Ini.NETStatus.GetValue() == 2)
	{
		*state = CELL_NET_CTL_STATE_Connecting;
	}
	else
	{
		*state = CELL_NET_CTL_STATE_Disconnected;
	}

	return CELL_OK;
}

s32 cellNetCtlAddHandler(vm::ptr<cellNetCtlHandler> handler, vm::ptr<void> arg, vm::ptr<s32> hid)
{
	cellNetCtl.Todo("cellNetCtlAddHandler(handler=*0x%x, arg=*0x%x, hid=*0x%x)", handler, arg, hid);

	return CELL_OK;
}

s32 cellNetCtlDelHandler(s32 hid)
{
	cellNetCtl.Todo("cellNetCtlDelHandler(hid=0x%x)", hid);

	return CELL_OK;
}

s32 cellNetCtlGetInfo(s32 code, vm::ptr<CellNetCtlInfo> info)
{
	cellNetCtl.Todo("cellNetCtlGetInfo(code=0x%x, info=*0x%x)", code, info);

	if (code == CELL_NET_CTL_INFO_IP_ADDRESS)
	{
		strcpy_trunc(info->ip_address, "192.168.1.1");
	}

	return CELL_OK;
}

s32 cellNetCtlNetStartDialogLoadAsync(vm::ptr<CellNetCtlNetStartDialogParam> param)
{
	cellNetCtl.Warning("cellNetCtlNetStartDialogLoadAsync(param=*0x%x)", param);

	// TODO: Actually sign into PSN or an emulated network similar to PSN
	sysutilSendSystemCommand(CELL_SYSUTIL_NET_CTL_NETSTART_FINISHED, 0);

	return CELL_OK;
}

s32 cellNetCtlNetStartDialogAbortAsync()
{
	cellNetCtl.Todo("cellNetCtlNetStartDialogAbortAsync()");

	return CELL_OK;
}

s32 cellNetCtlNetStartDialogUnloadAsync(vm::ptr<CellNetCtlNetStartDialogResult> result)
{
	cellNetCtl.Warning("cellNetCtlNetStartDialogUnloadAsync(result=*0x%x)", result);

	sysutilSendSystemCommand(CELL_SYSUTIL_NET_CTL_NETSTART_UNLOADED, 0);

	return CELL_OK;
}

s32 cellNetCtlGetNatInfo(vm::ptr<CellNetCtlNatInfo> natInfo)
{
	cellNetCtl.Todo("cellNetCtlGetNatInfo(natInfo=*0x%x)", natInfo);

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
