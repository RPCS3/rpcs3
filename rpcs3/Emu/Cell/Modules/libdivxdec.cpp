#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "libdivxdec.h"

LOG_CHANNEL(libdivxdec)

template<>
void fmt_class_string<DivxDecError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(DIVX_DEC_ERROR_ARG);
		}

		return unknown;
	});
}

error_code divxDecQueryAttr([[maybe_unused]] ppu_thread& ppu, vm::cptr<DivxDecParams> params, vm::ptr<u32> memSize, vm::ptr<u32> version)
{
	libdivxdec.todo("divxDecQueryAttr(params=*0x%x, memSize=*0x%x, version=*0x%x)", params, memSize, version);
	return CELL_OK;
}

error_code divxDecOpen()
{
	UNIMPLEMENTED_FUNC(libdivxdec);
	return CELL_OK;
}

error_code divxDecClose()
{
	UNIMPLEMENTED_FUNC(libdivxdec);
	return CELL_OK;
}

error_code divxDecReset()
{
	UNIMPLEMENTED_FUNC(libdivxdec);
	return CELL_OK;
}

error_code divxDecFlushPicture()
{
	UNIMPLEMENTED_FUNC(libdivxdec);
	return CELL_OK;
}

error_code divxDecGetNumOfPictures()
{
	UNIMPLEMENTED_FUNC(libdivxdec);
	return CELL_OK;
}

error_code divxDecGetPicture()
{
	UNIMPLEMENTED_FUNC(libdivxdec);
	return CELL_OK;
}

error_code divxDecDecodeAu()
{
	UNIMPLEMENTED_FUNC(libdivxdec);
	return CELL_OK;
}

error_code divxDecReleasePicture()
{
	UNIMPLEMENTED_FUNC(libdivxdec);
	return CELL_OK;
}

DECLARE(ppu_module_manager::libdivxdec)("libdivxdec", []
{
	REG_FUNC(libdivxdec, divxDecQueryAttr);
	REG_FUNC(libdivxdec, divxDecOpen);
	REG_FUNC(libdivxdec, divxDecClose);
	REG_FUNC(libdivxdec, divxDecReset);
	REG_FUNC(libdivxdec, divxDecFlushPicture);
	REG_FUNC(libdivxdec, divxDecGetNumOfPictures);
	REG_FUNC(libdivxdec, divxDecGetPicture);
	REG_FUNC(libdivxdec, divxDecDecodeAu);
	REG_FUNC(libdivxdec, divxDecReleasePicture);
});
