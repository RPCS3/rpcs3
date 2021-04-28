#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "cellCelpEnc.h"

LOG_CHANNEL(cellCelpEnc);

template <>
void fmt_class_string<CellCelpEncError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](CellCelpEncError value)
	{
		switch (value)
		{
		STR_CASE(CELL_CELPENC_ERROR_FAILED);
		STR_CASE(CELL_CELPENC_ERROR_SEQ);
		STR_CASE(CELL_CELPENC_ERROR_ARG);
		STR_CASE(CELL_CELPENC_ERROR_CORE_FAILED);
		STR_CASE(CELL_CELPENC_ERROR_CORE_SEQ);
		STR_CASE(CELL_CELPENC_ERROR_CORE_ARG);
		}

		return unknown;
	});
}

error_code cellCelpEncQueryAttr(vm::ptr<CellCelpEncAttr> attr)
{
	cellCelpEnc.todo("cellCelpEncQueryAttr(attr=*0x%x)", attr);
	return CELL_OK;
}

error_code cellCelpEncOpen(vm::ptr<CellCelpEncResource> res, vm::ptr<void> handle)
{
	cellCelpEnc.todo("cellCelpEncOpen(res=*0x%x ,attr=*0x%x)", res, handle);
	return CELL_OK;
}

error_code cellCelpEncOpenEx(vm::ptr<CellCelpEncResourceEx> res, vm::ptr<void> handle)
{
	cellCelpEnc.todo("cellCelpEncOpenEx(res=*0x%x ,attr=*0x%x)", res, handle);
	return CELL_OK;
}

error_code cellCelpEncOpenExt()
{
	cellCelpEnc.todo("cellCelpEncOpenExt()");
	return CELL_OK;
}

error_code cellCelpEncClose(vm::ptr<void> handle)
{
	cellCelpEnc.todo("cellCelpEncClose(handle=*0x%x)", handle);
	return CELL_OK;
}

error_code cellCelpEncStart(vm::ptr<void> handle, vm::ptr<CellCelpEncParam> param)
{
	cellCelpEnc.todo("cellCelpEncStart(handle=*0x%x, attr=*0x%x)", handle, param);
	return CELL_OK;
}

error_code cellCelpEncEnd(vm::ptr<void> handle)
{
	cellCelpEnc.todo("cellCelpEncEnd(handle=*0x%x)", handle);
	return CELL_OK;
}

error_code cellCelpEncEncodeFrame(vm::ptr<void> handle, vm::ptr<CellCelpEncPcmInfo> frameInfo)
{
	cellCelpEnc.todo("cellCelpEncEncodeFrame(handle=*0x%x, frameInfo=*0x%x)", handle, frameInfo);
	return CELL_OK;
}

error_code cellCelpEncWaitForOutput(vm::ptr<void> handle)
{
	cellCelpEnc.todo("cellCelpEncWaitForOutput(handle=*0x%x)", handle);
	return CELL_OK;
}

error_code cellCelpEncGetAu(vm::ptr<void> handle, vm::ptr<void> outBuffer, vm::ptr<CellCelpEncAuInfo> auItem)
{
	cellCelpEnc.todo("cellCelpEncGetAu(handle=*0x%x, outBuffer=*0x%x, auItem=*0x%x)", handle, outBuffer, auItem);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellCelpEnc)("cellCelpEnc", []()
{
	REG_FUNC(cellCelpEnc, cellCelpEncQueryAttr);
	REG_FUNC(cellCelpEnc, cellCelpEncOpen);
	REG_FUNC(cellCelpEnc, cellCelpEncOpenEx);
	REG_FUNC(cellCelpEnc, cellCelpEncOpenExt);
	REG_FUNC(cellCelpEnc, cellCelpEncClose);
	REG_FUNC(cellCelpEnc, cellCelpEncStart);
	REG_FUNC(cellCelpEnc, cellCelpEncEnd);
	REG_FUNC(cellCelpEnc, cellCelpEncEncodeFrame);
	REG_FUNC(cellCelpEnc, cellCelpEncWaitForOutput);
	REG_FUNC(cellCelpEnc, cellCelpEncGetAu);
});
