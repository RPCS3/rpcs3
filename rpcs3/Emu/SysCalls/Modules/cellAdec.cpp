#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "cellAdec.h"

void cellAdec_init();
Module cellAdec(0x0006, cellAdec_init);

int cellAdecQueryAttr(mem_ptr_t<CellAdecType> type, mem_ptr_t<CellAdecAttr> attr)
{
	cellAdec.Error("cellAdecQueryAttr(type_addr=0x%x, attr_addr=0x%x)", type.GetAddr(), attr.GetAddr());
	return CELL_OK;
}

int cellAdecOpen(mem_ptr_t<CellAdecType> type, mem_ptr_t<CellAdecResource> res, mem_ptr_t<CellAdecCb> cb, mem32_t handle)
{
	cellAdec.Error("cellAdecOpen(type_addr=0x%x, res_addr=0x%x, cb_addr=0x%x, handle_addr=0x%x)", 
		type.GetAddr(), res.GetAddr(), cb.GetAddr(), handle.GetAddr());
	return CELL_OK;
}

int cellAdecOpenEx(mem_ptr_t<CellAdecType> type, mem_ptr_t<CellAdecResourceEx> res, mem_ptr_t<CellAdecCb> cb, mem32_t handle)
{
	cellAdec.Error("cellAdecOpenEx(type_addr=0x%x, res_addr=0x%x, cb_addr=0x%x, handle_addr=0x%x)", 
		type.GetAddr(), res.GetAddr(), cb.GetAddr(), handle.GetAddr());
	return CELL_OK;
}

int cellAdecClose(u32 handle)
{
	cellAdec.Error("cellAdecClose(handle=0x%x)", handle);
	return CELL_OK;
}

int cellAdecStartSeq(u32 handle, u32 param_addr)
{
	cellAdec.Error("cellAdecStartSeq(handle=0x%x, param_addr=0x%x)", handle, param_addr);
	return CELL_OK;
}

int cellAdecEndSeq(u32 handle)
{
	cellAdec.Error("cellAdecEndSeq(handle=0x%x)", handle);
	return CELL_OK;
}

int cellAdecDecodeAu(u32 handle, mem_ptr_t<CellAdecAuInfo> auInfo)
{
	cellAdec.Error("cellAdecDecodeAu(handle=0x%x, auInfo_addr=0x%x)", handle, auInfo.GetAddr());
	return CELL_OK;
}

int cellAdecGetPcm(u32 handle, u32 outBuffer_addr)
{
	cellAdec.Error("cellAdecGetPcm(handle=0x%x, outBuffer_addr=0x%x)", handle, outBuffer_addr);
	return CELL_OK;
}

int cellAdecGetPcmItem(u32 handle, u32 pcmItem_ptr_addr)
{
	cellAdec.Error("cellAdecGetPcmItem(handle=0x%x, pcmItem_ptr_addr=0x%x)", handle, pcmItem_ptr_addr);
	return CELL_OK;
}

void cellAdec_init()
{
	cellAdec.AddFunc(0x7e4a4a49, cellAdecQueryAttr);
	cellAdec.AddFunc(0xd00a6988, cellAdecOpen);
	cellAdec.AddFunc(0x8b5551a4, cellAdecOpenEx);
	cellAdec.AddFunc(0x847d2380, cellAdecClose);
	cellAdec.AddFunc(0x487b613e, cellAdecStartSeq);
	cellAdec.AddFunc(0xe2ea549b, cellAdecEndSeq);
	cellAdec.AddFunc(0x1529e506, cellAdecDecodeAu);
	cellAdec.AddFunc(0x97ff2af1, cellAdecGetPcm);
	cellAdec.AddFunc(0xbd75f78b, cellAdecGetPcmItem);
}