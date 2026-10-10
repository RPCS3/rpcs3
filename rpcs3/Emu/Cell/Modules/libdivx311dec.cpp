#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "libdivx311dec.h"

LOG_CHANNEL(libdivx311dec)

template<>
void fmt_class_string<Divx311DecError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(DIVX311_DEC_ERROR_ARG);
		}

		return unknown;
	});
}

error_code divx311DecQueryAttr([[maybe_unused]] ppu_thread& ppu, vm::cptr<DivxDecParams> params, vm::ptr<u32> memSize, vm::ptr<u32> version)
{
	libdivx311dec.todo("divx311DecQueryAttr(params=*0x%x, memSize=*0x%x, version=*0x%x)", params, memSize, version);
	return CELL_OK;
}

error_code divx311DecOpen()
{
	UNIMPLEMENTED_FUNC(libdivx311dec);
	return CELL_OK;
}

error_code divx311DecClose()
{
	UNIMPLEMENTED_FUNC(libdivx311dec);
	return CELL_OK;
}

error_code divx311DecReset()
{
	UNIMPLEMENTED_FUNC(libdivx311dec);
	return CELL_OK;
}

error_code divx311DecFlushPicture()
{
	UNIMPLEMENTED_FUNC(libdivx311dec);
	return CELL_OK;
}

error_code divx311DecGetNumOfPictures()
{
	UNIMPLEMENTED_FUNC(libdivx311dec);
	return CELL_OK;
}

error_code divx311DecGetPicture()
{
	UNIMPLEMENTED_FUNC(libdivx311dec);
	return CELL_OK;
}

error_code divx311DecDecodeAu()
{
	UNIMPLEMENTED_FUNC(libdivx311dec);
	return CELL_OK;
}

error_code divx311DecReleasePicture()
{
	UNIMPLEMENTED_FUNC(libdivx311dec);
	return CELL_OK;
}

DECLARE(ppu_module_manager::libdivx311dec)("libdivx311dec", []
{
	REG_FUNC(libdivx311dec, divx311DecQueryAttr);
	REG_FUNC(libdivx311dec, divx311DecOpen);
	REG_FUNC(libdivx311dec, divx311DecClose);
	REG_FUNC(libdivx311dec, divx311DecReset);
	REG_FUNC(libdivx311dec, divx311DecFlushPicture);
	REG_FUNC(libdivx311dec, divx311DecGetNumOfPictures);
	REG_FUNC(libdivx311dec, divx311DecGetPicture);
	REG_FUNC(libdivx311dec, divx311DecDecodeAu);
	REG_FUNC(libdivx311dec, divx311DecReleasePicture);
});
