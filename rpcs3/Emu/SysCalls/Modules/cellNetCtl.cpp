#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/System.h"

#include "cellNetCtl.h"

//void cellNetCtl_init();
//Module cellNetCtl(0x0014, cellNetCtl_init);
Module *cellNetCtl;

int cellNetCtlInit()
{
	UNIMPLEMENTED_FUNC(cellNetCtl);
	return CELL_OK;
}

int cellNetCtlTerm()
{
	UNIMPLEMENTED_FUNC(cellNetCtl);
	return CELL_OK;
}

int cellNetCtlGetState(mem32_t state)
{
	cellNetCtl->Log("cellNetCtlGetState(state_addr=0x%x)", state.GetAddr());

	state = CELL_NET_CTL_STATE_Disconnected; // TODO: Allow other states

	return CELL_OK;
}

int cellNetCtlAddHandler(mem_func_ptr_t<cellNetCtlHandler> handler, mem32_t arg, s32 hid)
{
	cellNetCtl->Todo("cellNetCtlAddHandler(handler_addr=0x%x, arg_addr=0x%x, hid=%x)", handler.GetAddr(), arg.GetAddr(), hid);

	return CELL_OK;
}

int cellNetCtlDelHandler(s32 hid)
{
	cellNetCtl->Todo("cellNetCtlDelHandler(hid=%x)", hid);

	return CELL_OK;
}

int cellNetCtlGetInfo(s32 code, mem_ptr_t<CellNetCtlInfo> info)
{
	cellNetCtl->Todo("cellNetCtlGetInfo(code=%x, info_addr=0x%x)", code, info.GetAddr());

	return CELL_OK;
}

int cellNetCtlNetStartDialogLoadAsync(mem_ptr_t<CellNetCtlNetStartDialogParam> param)
{
	cellNetCtl->Warning("cellNetCtlNetStartDialogLoadAsync(param_addr=0x%x)", param.GetAddr());

	// TODO: Actually sign into PSN
	Emu.GetCallbackManager().m_exit_callback.Handle(CELL_SYSUTIL_NET_CTL_NETSTART_FINISHED, 0);

	return CELL_OK;
}

int cellNetCtlNetStartDialogAbortAsync()
{
	cellNetCtl->Todo("cellNetCtlNetStartDialogAbortAsync()");

	return CELL_OK;
}

int cellNetCtlNetStartDialogUnloadAsync(mem_ptr_t<CellNetCtlNetStartDialogResult> result)
{
	cellNetCtl->Warning("cellNetCtlNetStartDialogUnloadAsync(result_addr=0x%x)", result.GetAddr());

	Emu.GetCallbackManager().m_exit_callback.Handle(CELL_SYSUTIL_NET_CTL_NETSTART_UNLOADED, 0);

	return CELL_OK;
}

int cellNetCtlGetNatInfo(mem_ptr_t<CellNetCtlNatInfo> natInfo)
{
	cellNetCtl->Todo("cellNetCtlGetNatInfo(natInfo_addr=0x%x)", natInfo.GetAddr());

	if (natInfo->size == 0)
	{
		cellNetCtl->Error("cellNetCtlGetNatInfo : CELL_NET_CTL_ERROR_INVALID_SIZE");
		return CELL_NET_CTL_ERROR_INVALID_SIZE;
	}
	
	return CELL_OK;
}

void cellNetCtl_init()
{
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
