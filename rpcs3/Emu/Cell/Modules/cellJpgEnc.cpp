#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "cellJpgEnc.h"

LOG_CHANNEL(cellJpgEnc);

template <>
void fmt_class_string<CellJpgEncError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_JPGENC_ERROR_ARG);
			STR_CASE(CELL_JPGENC_ERROR_SEQ);
			STR_CASE(CELL_JPGENC_ERROR_BUSY);
			STR_CASE(CELL_JPGENC_ERROR_EMPTY);
			STR_CASE(CELL_JPGENC_ERROR_RESET);
			STR_CASE(CELL_JPGENC_ERROR_FATAL);
			STR_CASE(CELL_JPGENC_ERROR_STREAM_ABORT);
			STR_CASE(CELL_JPGENC_ERROR_STREAM_SKIP);
			STR_CASE(CELL_JPGENC_ERROR_STREAM_OVERFLOW);
			STR_CASE(CELL_JPGENC_ERROR_STREAM_FILE_OPEN);
		}

		return unknown;
	});
}


s32 cellJpgEncQueryAttr()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

s32 cellJpgEncOpen()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

s32 cellJpgEncOpenEx()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

s32 cellJpgEncClose()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

s32 cellJpgEncWaitForInput()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

s32 cellJpgEncEncodePicture()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

s32 cellJpgEncEncodePicture2()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

s32 cellJpgEncWaitForOutput()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

s32 cellJpgEncGetStreamInfo()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

s32 cellJpgEncReset()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellJpgEnc)("cellJpgEnc", []()
{
	REG_FUNC(cellJpgEnc, cellJpgEncQueryAttr);
	REG_FUNC(cellJpgEnc, cellJpgEncOpen);
	REG_FUNC(cellJpgEnc, cellJpgEncOpenEx);
	REG_FUNC(cellJpgEnc, cellJpgEncClose);
	REG_FUNC(cellJpgEnc, cellJpgEncWaitForInput);
	REG_FUNC(cellJpgEnc, cellJpgEncEncodePicture);
	REG_FUNC(cellJpgEnc, cellJpgEncEncodePicture2);
	REG_FUNC(cellJpgEnc, cellJpgEncWaitForOutput);
	REG_FUNC(cellJpgEnc, cellJpgEncGetStreamInfo);
	REG_FUNC(cellJpgEnc, cellJpgEncReset);
});
