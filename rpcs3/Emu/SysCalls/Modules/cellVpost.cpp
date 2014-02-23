#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "cellVpost.h"

void cellVpost_init();
Module cellVpost(0x0008, cellVpost_init);

int cellVpostQueryAttr(const mem_ptr_t<CellVpostCfgParam> cfgParam, mem_ptr_t<CellVpostAttr> attr)
{
	cellVpost.Error("cellVpostQueryAttr(cfgParam_addr=0x%x, attr_addr=0x%x)", cfgParam.GetAddr(), attr.GetAddr());
	return CELL_OK;
}

int cellVpostOpen(const mem_ptr_t<CellVpostCfgParam> cfgParam, const mem_ptr_t<CellVpostResource> resource, mem32_t handle)
{
	cellVpost.Error("cellVpostOpen(cfgParam_addr=0x%x, resource_addr=0x%x, handle_addr=0x%x)",
		cfgParam.GetAddr(), resource.GetAddr(), handle.GetAddr());
	return CELL_OK;
}

int cellVpostOpenEx(const mem_ptr_t<CellVpostCfgParam> cfgParam, const mem_ptr_t<CellVpostResourceEx> resource, mem32_t handle)
{
	cellVpost.Error("cellVpostOpenEx(cfgParam_addr=0x%x, resource_addr=0x%x, handle_addr=0x%x)",
		cfgParam.GetAddr(), resource.GetAddr(), handle.GetAddr());
	return CELL_OK;
}

int cellVpostClose(u32 handle)
{
	cellVpost.Error("cellVpostClose(handle=0x%x)", handle);
	return CELL_OK;
}

int cellVpostExec(u32 handle, const u32 inPicBuff_addr, const mem_ptr_t<CellVpostCtrlParam> ctrlParam,
				  u32 outPicBuff_addr, mem_ptr_t<CellVpostPictureInfo> picInfo)
{
	cellVpost.Error("cellVpostExec(handle=0x%x, inPicBuff_addr=0x%x, ctrlParam_addr=0x%x, outPicBuff_addr=0x%x, picInfo_addr=0x%x)",
		handle, inPicBuff_addr, ctrlParam.GetAddr(), outPicBuff_addr, picInfo.GetAddr());
	return CELL_OK;
}

void cellVpost_init()
{
	cellVpost.AddFunc(0x95e788c3, cellVpostQueryAttr);
	cellVpost.AddFunc(0xcd33f3e2, cellVpostOpen);
	cellVpost.AddFunc(0x40524325, cellVpostOpenEx);
	cellVpost.AddFunc(0x10ef39f6, cellVpostClose);
	cellVpost.AddFunc(0xabb8cc3d, cellVpostExec);
}