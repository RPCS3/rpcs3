#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "libsmvd2.h"

LOG_CHANNEL(libsmvd2)

template<>
void fmt_class_string<Smvd2Error>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(SMVD2_ERROR_ARG);
		}

		return unknown;
	});
}

error_code smvd2GetMemorySize([[maybe_unused]] ppu_thread& ppu, vm::ptr<u32> memSize, u32 profileLevel)
{
	libsmvd2.todo("smvd2GetMemorySize(memSize=*0x%x, profileLevel=%d)", memSize, +profileLevel);
	return CELL_OK;
}

error_code smvd2GetMemorySize2([[maybe_unused]] ppu_thread& ppu, vm::ptr<u32> memSize, u32 profileLevel, vm::cptr<Smvd2Params> params)
{
	libsmvd2.todo("smvd2GetMemorySize2(memSize=*0x%x, profileLevel=%d, params=*0x%x)", memSize, profileLevel, params);
	return CELL_OK;
}

error_code smvd2GetVersionNumber([[maybe_unused]] ppu_thread& ppu, vm::ptr<u32> version)
{
	libsmvd2.notice("smvd2GetVersionNumber(version=*0x%x)", version);

	if (!version)
	{
		return SMVD2_ERROR_ARG;
	}

	*version = 0x1030000;

	return CELL_OK;
}

error_code smvd2Initialize()
{
	UNIMPLEMENTED_FUNC(libsmvd2);
	return CELL_OK;
}

error_code smvd2CreateInstance2()
{
	UNIMPLEMENTED_FUNC(libsmvd2);
	return CELL_OK;
}

error_code smvd2ReleaseInstance()
{
	UNIMPLEMENTED_FUNC(libsmvd2);
	return CELL_OK;
}

error_code smvd2CancelDecode()
{
	UNIMPLEMENTED_FUNC(libsmvd2);
	return CELL_OK;
}

error_code smvd2FlushPicture()
{
	UNIMPLEMENTED_FUNC(libsmvd2);
	return CELL_OK;
}

error_code smvd2GetNumPicture()
{
	UNIMPLEMENTED_FUNC(libsmvd2);
	return CELL_OK;
}

error_code smvd2GetPicture()
{
	UNIMPLEMENTED_FUNC(libsmvd2);
	return CELL_OK;
}

error_code smvd2ReleasePicture()
{
	UNIMPLEMENTED_FUNC(libsmvd2);
	return CELL_OK;
}

error_code smvd2GetAuxData()
{
	UNIMPLEMENTED_FUNC(libsmvd2);
	return CELL_OK;
}

error_code smvd2GetSideInfoSeq()
{
	UNIMPLEMENTED_FUNC(libsmvd2);
	return CELL_OK;
}

error_code smvd2GetBnrData()
{
	UNIMPLEMENTED_FUNC(libsmvd2);
	return CELL_OK;
}

error_code smvd2GetSideInfoPic()
{
	UNIMPLEMENTED_FUNC(libsmvd2);
	return CELL_OK;
}

error_code smvd2SetAuxData()
{
	UNIMPLEMENTED_FUNC(libsmvd2);
	return CELL_OK;
}

error_code smvd2SetDecodeMode()
{
	UNIMPLEMENTED_FUNC(libsmvd2);
	return CELL_OK;
}

error_code smvd2DecodePicture()
{
	UNIMPLEMENTED_FUNC(libsmvd2);
	return CELL_OK;
}

error_code smvd2DecodePicture2()
{
	UNIMPLEMENTED_FUNC(libsmvd2);
	return CELL_OK;
}

error_code smvd2GetExistenceFlag()
{
	UNIMPLEMENTED_FUNC(libsmvd2);
	return CELL_OK;
}

error_code smvd2Discontinue()
{
	UNIMPLEMENTED_FUNC(libsmvd2);
	return CELL_OK;
}

DECLARE(ppu_module_manager::libsmvd2)("libsmvd2", []
{
	REG_FUNC(libsmvd2, smvd2GetMemorySize);
	REG_FUNC(libsmvd2, smvd2GetMemorySize2);
	REG_FUNC(libsmvd2, smvd2GetVersionNumber);
	REG_FUNC(libsmvd2, smvd2Initialize);
	REG_FUNC(libsmvd2, smvd2CreateInstance2);
	REG_FUNC(libsmvd2, smvd2ReleaseInstance);
	REG_FUNC(libsmvd2, smvd2CancelDecode);
	REG_FUNC(libsmvd2, smvd2FlushPicture);
	REG_FUNC(libsmvd2, smvd2GetNumPicture);
	REG_FUNC(libsmvd2, smvd2GetPicture);
	REG_FUNC(libsmvd2, smvd2ReleasePicture);
	REG_FUNC(libsmvd2, smvd2GetAuxData);
	REG_FUNC(libsmvd2, smvd2GetSideInfoSeq);
	REG_FUNC(libsmvd2, smvd2GetBnrData);
	REG_FUNC(libsmvd2, smvd2GetSideInfoPic);
	REG_FUNC(libsmvd2, smvd2SetAuxData);
	REG_FUNC(libsmvd2, smvd2SetDecodeMode);
	REG_FUNC(libsmvd2, smvd2DecodePicture);
	REG_FUNC(libsmvd2, smvd2DecodePicture2);
	REG_FUNC(libsmvd2, smvd2GetExistenceFlag);
	REG_FUNC(libsmvd2, smvd2Discontinue);
});
