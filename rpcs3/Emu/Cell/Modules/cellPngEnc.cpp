#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "cellPngEnc.h"

LOG_CHANNEL(cellPngEnc);

template <>
void fmt_class_string<CellPngEncError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](CellPngEncError value)
	{
		switch (value)
		{
		STR_CASE(CELL_PNGENC_ERROR_ARG);
		STR_CASE(CELL_PNGENC_ERROR_SEQ);
		STR_CASE(CELL_PNGENC_ERROR_BUSY);
		STR_CASE(CELL_PNGENC_ERROR_EMPTY);
		STR_CASE(CELL_PNGENC_ERROR_RESET);
		STR_CASE(CELL_PNGENC_ERROR_FATAL);
		STR_CASE(CELL_PNGENC_ERROR_STREAM_ABORT);
		STR_CASE(CELL_PNGENC_ERROR_STREAM_SKIP);
		STR_CASE(CELL_PNGENC_ERROR_STREAM_OVERFLOW);
		STR_CASE(CELL_PNGENC_ERROR_STREAM_FILE_OPEN);
		}

		return unknown;
	});
}

error_code cellPngEncQueryAttr(vm::cptr<CellPngEncConfig> config, vm::ptr<CellPngEncAttr> attr)
{
	cellPngEnc.todo("cellPngEncQueryAttr(config=*0x%x, attr=*0x%x)", config, attr);
	return CELL_OK;
}

error_code cellPngEncOpen(vm::cptr<CellPngEncConfig> config, vm::cptr<CellPngEncResource> resource, vm::pptr<void> handle)
{
	cellPngEnc.todo("cellPngEncOpen(config=*0x%x, resource=*0x%x, handle=*0x%x)", config, resource, handle);
	return CELL_OK;
}

error_code cellPngEncOpenEx(vm::cptr<CellPngEncConfig> config, vm::cptr<CellPngEncResourceEx> resourceEx, vm::pptr<void> handle)
{
	cellPngEnc.todo("cellPngEncOpenEx(config=*0x%x, resourceEx=*0x%x, handle=*0x%x)", config, resourceEx, handle);
	return CELL_OK;
}

error_code cellPngEncClose(vm::ptr<void> handle)
{
	cellPngEnc.todo("cellPngEncClose(handle=*0x%x)", handle);
	return CELL_OK;
}

error_code cellPngEncWaitForInput(vm::ptr<void> handle, b8 block)
{
	cellPngEnc.todo("cellPngEncWaitForInput(handle=*0x%x, block=%d)", handle, block);
	return CELL_OK;
}

error_code cellPngEncEncodePicture(vm::ptr<void> handle, vm::cptr<CellPngEncPicture> picture, vm::cptr<CellPngEncEncodeParam> encodeParam, vm::cptr<CellPngEncOutputParam> outputParam)
{
	cellPngEnc.todo("cellPngEncEncodePicture(handle=*0x%x, picture=*0x%x, encodeParam=*0x%x, outputParam=*0x%x)", handle, picture, encodeParam, outputParam);
	return CELL_OK;
}

error_code cellPngEncWaitForOutput(vm::ptr<void> handle, vm::ptr<u32> streamInfoNum, b8 block)
{
	cellPngEnc.todo("cellPngEncWaitForOutput(handle=*0x%x, streamInfoNum=*0x%x, block=%d)", handle, streamInfoNum, block);
	return CELL_OK;
}

error_code cellPngEncGetStreamInfo(vm::ptr<void> handle, vm::ptr<CellPngEncStreamInfo> streamInfo)
{
	cellPngEnc.todo("cellPngEncGetStreamInfo(handle=*0x%x, streamInfo=*0x%x)", handle, streamInfo);
	return CELL_OK;
}

error_code cellPngEncReset(vm::ptr<void> handle)
{
	cellPngEnc.todo("cellPngEncReset(handle=*0x%x)", handle);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellPngEnc)("cellPngEnc", []()
{
	REG_FUNC(cellPngEnc, cellPngEncQueryAttr);
	REG_FUNC(cellPngEnc, cellPngEncOpen);
	REG_FUNC(cellPngEnc, cellPngEncOpenEx);
	REG_FUNC(cellPngEnc, cellPngEncClose);
	REG_FUNC(cellPngEnc, cellPngEncWaitForInput);
	REG_FUNC(cellPngEnc, cellPngEncEncodePicture);
	REG_FUNC(cellPngEnc, cellPngEncWaitForOutput);
	REG_FUNC(cellPngEnc, cellPngEncGetStreamInfo);
	REG_FUNC(cellPngEnc, cellPngEncReset);
});
