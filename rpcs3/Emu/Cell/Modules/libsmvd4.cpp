#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "libsmvd4.h"

LOG_CHANNEL(libsmvd4)

template<>
void fmt_class_string<Smvd4Error>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(SMVD4_ERROR_ARG);
		}

		return unknown;
	});
}

error_code smvd4GetMemorySize([[maybe_unused]] ppu_thread& ppu, vm::ptr<u32> memSize, u32 profileLevel)
{
	libsmvd4.todo("smvd4GetMemorySize(memSize=*0x%x, profileLevel=%d)", memSize, +profileLevel);
	return CELL_OK;
}

error_code smvd4GetMemorySize2([[maybe_unused]] ppu_thread& ppu, vm::ptr<u32> memSize, u32 profileLevel, vm::cptr<Smvd4Params> params)
{
	libsmvd4.todo("smvd4GetMemorySize2(memSize=*0x%x, profileLevel=%d, params=*0x%x)", memSize, profileLevel, params);
	return CELL_OK;
}

error_code smvd4GetVersionNumber([[maybe_unused]] ppu_thread& ppu, vm::ptr<u32> version)
{
	libsmvd4.notice("smvd4GetVersionNumber(version=*0x%x)", version);

	if (!version)
	{
		return SMVD4_ERROR_ARG;
	}

	*version = 0x1080000;

	return CELL_OK;
}

error_code smvd4Initialize()
{
	UNIMPLEMENTED_FUNC(libsmvd4);
	return CELL_OK;
}

error_code smvd4CreateInstance2()
{
	UNIMPLEMENTED_FUNC(libsmvd4);
	return CELL_OK;
}

error_code smvd4ReleaseInstance()
{
	UNIMPLEMENTED_FUNC(libsmvd4);
	return CELL_OK;
}

error_code smvd4CancelDecode()
{
	UNIMPLEMENTED_FUNC(libsmvd4);
	return CELL_OK;
}

error_code smvd4FlushPicture()
{
	UNIMPLEMENTED_FUNC(libsmvd4);
	return CELL_OK;
}

error_code smvd4GetNumPicture()
{
	UNIMPLEMENTED_FUNC(libsmvd4);
	return CELL_OK;
}

error_code smvd4GetPicture()
{
	UNIMPLEMENTED_FUNC(libsmvd4);
	return CELL_OK;
}

error_code smvd4ReleasePicture()
{
	UNIMPLEMENTED_FUNC(libsmvd4);
	return CELL_OK;
}

error_code smvd4GetAuxData()
{
	UNIMPLEMENTED_FUNC(libsmvd4);
	return CELL_OK;
}

error_code smvd4GetSideInfoSeq()
{
	UNIMPLEMENTED_FUNC(libsmvd4);
	return CELL_OK;
}

error_code smvd4GetSideInfoPic()
{
	UNIMPLEMENTED_FUNC(libsmvd4);
	return CELL_OK;
}

error_code smvd4SetAuxData()
{
	UNIMPLEMENTED_FUNC(libsmvd4);
	return CELL_OK;
}

error_code smvd4DecodePicture()
{
	UNIMPLEMENTED_FUNC(libsmvd4);
	return CELL_OK;
}

error_code smvd4GetExistenceFlag()
{
	UNIMPLEMENTED_FUNC(libsmvd4);
	return CELL_OK;
}

error_code smvd4Discontinue()
{
	UNIMPLEMENTED_FUNC(libsmvd4);
	return CELL_OK;
}

DECLARE(ppu_module_manager::libsmvd4)("libsmvd4", []
{
	REG_FUNC(libsmvd4, smvd4GetMemorySize);
	REG_FUNC(libsmvd4, smvd4GetMemorySize2);
	REG_FUNC(libsmvd4, smvd4GetVersionNumber);
	REG_FUNC(libsmvd4, smvd4Initialize);
	REG_FUNC(libsmvd4, smvd4CreateInstance2);
	REG_FUNC(libsmvd4, smvd4ReleaseInstance);
	REG_FUNC(libsmvd4, smvd4CancelDecode);
	REG_FUNC(libsmvd4, smvd4FlushPicture);
	REG_FUNC(libsmvd4, smvd4GetNumPicture);
	REG_FUNC(libsmvd4, smvd4GetPicture);
	REG_FUNC(libsmvd4, smvd4ReleasePicture);
	REG_FUNC(libsmvd4, smvd4GetAuxData);
	REG_FUNC(libsmvd4, smvd4GetSideInfoSeq);
	REG_FUNC(libsmvd4, smvd4GetSideInfoPic);
	REG_FUNC(libsmvd4, smvd4SetAuxData);
	REG_FUNC(libsmvd4, smvd4DecodePicture);
	REG_FUNC(libsmvd4, smvd4GetExistenceFlag);
	REG_FUNC(libsmvd4, smvd4Discontinue);
});
