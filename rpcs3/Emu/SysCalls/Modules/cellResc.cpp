#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "Emu/GS/GCM.h"

void cellRecs_init();
Module cellRecs(0x001f, cellRecs_init);

int cellRescSetConvertAndFlip(s32 indx)
{
	cellRecs.Log("cellRescSetConvertAndFlip(indx=0x%x)", indx);

	return CELL_OK;
}

int cellRescSetWaitFlip()
{
	return CELL_OK;
}

int cellRescSetFlipHandler(u32 handler_addr)
{
	cellRecs.Warning("cellRescSetFlipHandler(handler_addr=%d)", handler_addr);
	if(!Memory.IsGoodAddr(handler_addr) && handler_addr != 0)
	{
		return CELL_EFAULT;
	}

	Emu.GetGSManager().GetRender().m_flip_handler.SetAddr(handler_addr);
	return 0;
}

void cellRecs_init()
{
	cellRecs.AddFunc(0x25c107e6, cellRescSetConvertAndFlip);
	cellRecs.AddFunc(0x0d3c22ce, cellRescSetWaitFlip);
	cellRecs.AddFunc(0x2ea94661, cellRescSetFlipHandler);
}
