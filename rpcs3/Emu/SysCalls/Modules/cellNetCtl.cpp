#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "cellSysutil.h"
#include "cellNetCtl.h"

Module *cellNetCtl;

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
	cellNetCtl->Warning("cellNetCtlInit()");

	if (cellNetCtlInstance.m_bInitialized)
		return CELL_NET_CTL_ERROR_NOT_TERMINATED;

	cellNetCtlInstance.m_bInitialized = true;

	return CELL_OK;
}

int cellNetCtlTerm()
{
	cellNetCtl->Warning("cellNetCtlTerm()");

	if (!cellNetCtlInstance.m_bInitialized)
		return CELL_NET_CTL_ERROR_NOT_INITIALIZED;

	cellNetCtlInstance.m_bInitialized = false;

	return CELL_OK;
}

int cellNetCtlGetState(vm::ptr<u32> state)
{
	cellNetCtl->Log("cellNetCtlGetState(state_addr=0x%x)", state.addr());

	*state = CELL_NET_CTL_STATE_Disconnected; // TODO: Allow other states

	return CELL_OK;
}

int cellNetCtlAddHandler(vm::ptr<cellNetCtlHandler> handler, vm::ptr<u32> arg, vm::ptr<s32> hid)
{
	cellNetCtl->Todo("cellNetCtlAddHandler(handler_addr=0x%x, arg_addr=0x%x, hid=0x%x)", handler.addr(), arg.addr(), hid.addr());

	return CELL_OK;
}

int cellNetCtlDelHandler(s32 hid)
{
	cellNetCtl->Todo("cellNetCtlDelHandler(hid=0x%x)", hid);

	return CELL_OK;
}

int cellNetCtlGetInfo(s32 code, vm::ptr<CellNetCtlInfo> info)
{
	cellNetCtl->Todo("cellNetCtlGetInfo(code=%x, info_addr=0x%x)", code, info.addr());

	return CELL_OK;
}

int cellNetCtlNetStartDialogLoadAsync(vm::ptr<CellNetCtlNetStartDialogParam> param)
{
	cellNetCtl->Warning("cellNetCtlNetStartDialogLoadAsync(param_addr=0x%x)", param.addr());

	// TODO: Actually sign into PSN
	sysutilSendSystemCommand(CELL_SYSUTIL_NET_CTL_NETSTART_FINISHED, 0);
	return CELL_OK;
}

int cellNetCtlNetStartDialogAbortAsync()
{
	cellNetCtl->Todo("cellNetCtlNetStartDialogAbortAsync()");

	return CELL_OK;
}

int cellNetCtlNetStartDialogUnloadAsync(vm::ptr<CellNetCtlNetStartDialogResult> result)
{
	cellNetCtl->Warning("cellNetCtlNetStartDialogUnloadAsync(result_addr=0x%x)", result.addr());

	sysutilSendSystemCommand(CELL_SYSUTIL_NET_CTL_NETSTART_UNLOADED, 0);
	return CELL_OK;
}

int cellNetCtlGetNatInfo(vm::ptr<CellNetCtlNatInfo> natInfo)
{
	cellNetCtl->Todo("cellNetCtlGetNatInfo(natInfo_addr=0x%x)", natInfo.addr());

	if (natInfo->size == 0)
	{
		cellNetCtl->Error("cellNetCtlGetNatInfo : CELL_NET_CTL_ERROR_INVALID_SIZE");
		return CELL_NET_CTL_ERROR_INVALID_SIZE;
	}
	
	return CELL_OK;
}

void cellNetCtl_unload()
{
	cellNetCtlInstance.m_bInitialized = false;
}

void cellNetCtl_init(Module *pxThis)
{
	cellNetCtl = pxThis;

	cellNetCtl->AddFunc(0xbd5a59fc, cellNetCtlInit);
	cellNetCtl->AddFunc(0x105ee2cb, cellNetCtlTerm);

	cellNetCtl->AddFunc(0x8b3eba69, cellNetCtlGetState);
	cellNetCtl->AddFunc(0x0ce13c6b, cellNetCtlAddHandler);
	cellNetCtl->AddFunc(0x901815c3, cellNetCtlDelHandler);

	cellNetCtl->AddFunc(0x1e585b5d, cellNetCtlGetInfo);

	cellNetCtl->AddFunc(0x04459230, cellNetCtlNetStartDialogLoadAsync);
	cellNetCtl->AddFunc(0x71d53210, cellNetCtlNetStartDialogAbortAsync);
	cellNetCtl->AddFunc(0x0f1f13d3, cellNetCtlNetStartDialogUnloadAsync);

	cellNetCtl->AddFunc(0x3a12865f, cellNetCtlGetNatInfo);
}
