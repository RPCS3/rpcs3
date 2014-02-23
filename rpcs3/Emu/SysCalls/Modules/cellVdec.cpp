#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "cellVdec.h"

void cellVdec_init();
Module cellVdec(0x0005, cellVdec_init);

int cellVdecQueryAttr(const mem_ptr_t<CellVdecType> type, mem_ptr_t<CellVdecAttr> attr)
{
	cellVdec.Error("cellVdecQueryAttr(type_addr=0x%x, attr_addr=0x%x)", type.GetAddr(), attr.GetAddr());
	return CELL_OK;
}

int cellVdecQueryAttrEx(const mem_ptr_t<CellVdecTypeEx> type, mem_ptr_t<CellVdecAttr> attr)
{
	cellVdec.Error("cellVdecQueryAttrEx(type_addr=0x%x, attr_addr=0x%x)", type.GetAddr(), attr.GetAddr());
	return CELL_OK;
}

int cellVdecOpen(const mem_ptr_t<CellVdecType> type, const mem_ptr_t<CellVdecResource> res, const mem_ptr_t<CellVdecCb> cb, mem32_t handle)
{
	cellVdec.Error("cellVdecOpen(type_addr=0x%x, res_addr=0x%x, cb_addr=0x%x, handle_addr=0x%x)",
		type.GetAddr(), res.GetAddr(), cb.GetAddr(), handle.GetAddr());
	return CELL_OK;
}

int cellVdecOpenEx(const mem_ptr_t<CellVdecTypeEx> type, const mem_ptr_t<CellVdecResourceEx> res, const mem_ptr_t<CellVdecCb> cb, mem32_t handle)
{
	cellVdec.Error("cellVdecOpenEx(type_addr=0x%x, res_addr=0x%x, cb_addr=0x%x, handle_addr=0x%x)",
		type.GetAddr(), res.GetAddr(), cb.GetAddr(), handle.GetAddr());
	return CELL_OK;
}

int cellVdecClose(u32 handle)
{
	cellVdec.Error("cellVdecClose(handle=0x%x)", handle);
	return CELL_OK;
}

int cellVdecStartSeq(u32 handle)
{
	cellVdec.Error("cellVdecStartSeq(handle=0x%x)", handle);
	return CELL_OK;
}

int cellVdecEndSeq(u32 handle)
{
	cellVdec.Error("cellVdecEndSeq(handle=0x%x)", handle);
	return CELL_OK;
}

int cellVdecDecodeAu(u32 handle, CellVdecDecodeMode mode, const mem_ptr_t<CellVdecAuInfo> auInfo)
{
	cellVdec.Error("cellVdecDecodeAu(handle=0x%x, mode=0x%x, auInfo_addr=0x%x)", handle, mode, auInfo.GetAddr());
	return CELL_OK;
}

int cellVdecGetPicture(u32 handle, const mem_ptr_t<CellVdecPicFormat> format, u32 out_addr)
{
	cellVdec.Error("cellVdecGetPicture(handle=0x%x, format_addr=0x%x, out_addr=0x%x)", handle, format.GetAddr(), out_addr);
	return CELL_OK;
}

int cellVdecGetPicItem(u32 handle, const u32 picItem_ptr_addr)
{
	cellVdec.Error("cellVdecGetPicItem(handle=0x%x, picItem_ptr_addr=0x%x)", handle, picItem_ptr_addr);
	return CELL_OK;
}

int cellVdecSetFrameRate(u32 handle, CellVdecFrameRate frc)
{
	cellVdec.Error("cellVdecSetFrameRate(handle=0x%x, frc=0x%x)", handle, frc);
	return CELL_OK;
}

void cellVdec_init()
{
	cellVdec.AddFunc(0xff6f6ebe, cellVdecQueryAttr);
	cellVdec.AddFunc(0xc982a84a, cellVdecQueryAttrEx);
	cellVdec.AddFunc(0xb6bbcd5d, cellVdecOpen);
	cellVdec.AddFunc(0x0053e2d8, cellVdecOpenEx);
	cellVdec.AddFunc(0x16698e83, cellVdecClose);
	cellVdec.AddFunc(0xc757c2aa, cellVdecStartSeq);
	cellVdec.AddFunc(0x824433f0, cellVdecEndSeq);
	cellVdec.AddFunc(0x2bf4ddd2, cellVdecDecodeAu);
	cellVdec.AddFunc(0x807c861a, cellVdecGetPicture);
	cellVdec.AddFunc(0x17c702b9, cellVdecGetPicItem);
	cellVdec.AddFunc(0xe13ef6fc, cellVdecSetFrameRate);
}