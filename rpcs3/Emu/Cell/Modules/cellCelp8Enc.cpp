#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "cellCelp8Enc.h"

LOG_CHANNEL(cellCelp8Enc);

template <>
void fmt_class_string<CellCelp8EncError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](CellCelp8EncError value)
	{
		switch (value)
		{
		STR_CASE(CELL_CELP8ENC_ERROR_FAILED);
		STR_CASE(CELL_CELP8ENC_ERROR_SEQ);
		STR_CASE(CELL_CELP8ENC_ERROR_ARG);
		STR_CASE(CELL_CELP8ENC_ERROR_CORE_FAILED);
		STR_CASE(CELL_CELP8ENC_ERROR_CORE_SEQ);
		STR_CASE(CELL_CELP8ENC_ERROR_CORE_ARG);
		}

		return unknown;
	});
}

error_code cellCelp8EncQueryAttr(vm::ptr<CellCelp8EncAttr> attr)
{
	cellCelp8Enc.todo("cellCelp8EncQueryAttr(attr=*0x%x)", attr);
	return CELL_OK;
}

error_code cellCelp8EncOpen(vm::ptr<CellCelp8EncResource> res, vm::pptr<void> handle)
{
	cellCelp8Enc.todo("cellCelp8EncOpen(res=*0x%x, handle=*0x%x)", res, handle);
	return CELL_OK;
}

error_code cellCelp8EncOpenEx(vm::ptr<CellCelp8EncResource> res, vm::pptr<void> handle)
{
	cellCelp8Enc.todo("cellCelp8EncOpenEx(res=*0x%x, handle=*0x%x)", res, handle);
	return CELL_OK;
}

error_code cellCelp8EncClose(vm::ptr<void> handle)
{
	cellCelp8Enc.todo("cellCelp8EncClose(handle=*0x%x)", handle);
	return CELL_OK;
}

error_code cellCelp8EncStart(vm::ptr<void> handle, vm::ptr<CellCelp8EncParam> param)
{
	cellCelp8Enc.todo("cellCelp8EncStart(handle=*0x%x, param=*0x%x)", handle, param);
	return CELL_OK;
}

error_code cellCelp8EncEnd(vm::ptr<void> handle)
{
	cellCelp8Enc.todo("cellCelp8EncEnd(handle=*0x%x)", handle);
	return CELL_OK;
}

error_code cellCelp8EncEncodeFrame(vm::ptr<void> handle, vm::ptr<CellCelp8EncPcmInfo> frameInfo)
{
	cellCelp8Enc.todo("cellCelp8EncEncodeFrame(handle=*0x%x, frameInfo=*0x%x)", handle, frameInfo);
	return CELL_OK;
}

error_code cellCelp8EncWaitForOutput(vm::ptr<void> handle)
{
	cellCelp8Enc.todo("cellCelp8EncWaitForOutput(handle=*0x%x)", handle);
	return CELL_OK;
}

error_code cellCelp8EncGetAu(vm::ptr<void> handle, vm::ptr<void> outBuffer, vm::ptr<CellCelp8EncAuInfo> auItem)
{
	cellCelp8Enc.todo("cellCelp8EncGetAu(handle=*0x%x, outBuffer=*0x%x, auItem=*0x%x)", handle, outBuffer, auItem);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellCelp8Enc)("cellCelp8Enc", []()
{
	REG_FUNC(cellCelp8Enc, cellCelp8EncQueryAttr);
	REG_FUNC(cellCelp8Enc, cellCelp8EncOpen);
	REG_FUNC(cellCelp8Enc, cellCelp8EncOpenEx);
	REG_FUNC(cellCelp8Enc, cellCelp8EncClose);
	REG_FUNC(cellCelp8Enc, cellCelp8EncStart);
	REG_FUNC(cellCelp8Enc, cellCelp8EncEnd);
	REG_FUNC(cellCelp8Enc, cellCelp8EncEncodeFrame);
	REG_FUNC(cellCelp8Enc, cellCelp8EncWaitForOutput);
	REG_FUNC(cellCelp8Enc, cellCelp8EncGetAu);
});
