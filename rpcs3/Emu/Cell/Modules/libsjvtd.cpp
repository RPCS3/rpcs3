#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "libsjvtd.h"

LOG_CHANNEL(libsjvtd)

template<>
void fmt_class_string<SjvtdError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(SJVTD_ERROR_ARG);
		}

		return unknown;
	});
}

error_code sjvtdGetMemorySize([[maybe_unused]] ppu_thread& ppu, vm::ptr<u32> memSize, u32 profileLevel)
{
	libsjvtd.todo("sjvtdGetMemorySize(memSize=*0x%x, profileLevel=%d)", memSize, +profileLevel);
	return CELL_OK;
}

error_code sjvtdGetMemorySize2([[maybe_unused]] ppu_thread& ppu, vm::ptr<u32> memSize, u32 profileLevel, vm::cptr<SjvtdParams> params)
{
	libsjvtd.todo("sjvtdGetMemorySize2(memSize=*0x%x, profileLevel=%d, params=*0x%x)", memSize, profileLevel, params);
	return CELL_OK;
}

error_code sjvtdGetVersionNumber([[maybe_unused]] ppu_thread& ppu, vm::ptr<u32> version)
{
	libsjvtd.notice("sjvtdGetVersionNumber(version=*0x%x)", version);

	if (!version)
	{
		return SJVTD_ERROR_ARG;
	}

	*version = 0x1050000;

	return CELL_OK;
}

error_code sjvtdInitialize()
{
	UNIMPLEMENTED_FUNC(libsjvtd);
	return CELL_OK;
}

error_code sjvtdCreateInstance2()
{
	UNIMPLEMENTED_FUNC(libsjvtd);
	return CELL_OK;
}

error_code sjvtdReleaseInstance()
{
	UNIMPLEMENTED_FUNC(libsjvtd);
	return CELL_OK;
}

error_code sjvtdCancelDecode()
{
	UNIMPLEMENTED_FUNC(libsjvtd);
	return CELL_OK;
}

error_code sjvtdFlushPicture()
{
	UNIMPLEMENTED_FUNC(libsjvtd);
	return CELL_OK;
}

error_code sjvtdGetNumPicture()
{
	UNIMPLEMENTED_FUNC(libsjvtd);
	return CELL_OK;
}

error_code sjvtdGetPicture()
{
	UNIMPLEMENTED_FUNC(libsjvtd);
	return CELL_OK;
}

error_code sjvtdReleasePicture()
{
	UNIMPLEMENTED_FUNC(libsjvtd);
	return CELL_OK;
}

error_code sjvtdGetAuxData()
{
	UNIMPLEMENTED_FUNC(libsjvtd);
	return CELL_OK;
}

error_code sjvtdGetSideInfoSeq()
{
	UNIMPLEMENTED_FUNC(libsjvtd);
	return CELL_OK;
}

error_code sjvtdGetSideInfoPic()
{
	UNIMPLEMENTED_FUNC(libsjvtd);
	return CELL_OK;
}

error_code sjvtdSetAuxData()
{
	UNIMPLEMENTED_FUNC(libsjvtd);
	return CELL_OK;
}

error_code sjvtdSetDecodeMode()
{
	UNIMPLEMENTED_FUNC(libsjvtd);
	return CELL_OK;
}

error_code sjvtdDecodePicture()
{
	UNIMPLEMENTED_FUNC(libsjvtd);
	return CELL_OK;
}

error_code sjvtdGetExistenceFlag()
{
	UNIMPLEMENTED_FUNC(libsjvtd);
	return CELL_OK;
}

error_code sjvtdDiscontinue()
{
	UNIMPLEMENTED_FUNC(libsjvtd);
	return CELL_OK;
}

DECLARE(ppu_module_manager::libsjvtd)("libsjvtd", []
{
	REG_FUNC(libsjvtd, sjvtdGetMemorySize);
	REG_FUNC(libsjvtd, sjvtdGetMemorySize2);
	REG_FUNC(libsjvtd, sjvtdGetVersionNumber);
	REG_FUNC(libsjvtd, sjvtdInitialize);
	REG_FUNC(libsjvtd, sjvtdCreateInstance2);
	REG_FUNC(libsjvtd, sjvtdReleaseInstance);
	REG_FUNC(libsjvtd, sjvtdCancelDecode);
	REG_FUNC(libsjvtd, sjvtdFlushPicture);
	REG_FUNC(libsjvtd, sjvtdGetNumPicture);
	REG_FUNC(libsjvtd, sjvtdGetPicture);
	REG_FUNC(libsjvtd, sjvtdReleasePicture);
	REG_FUNC(libsjvtd, sjvtdGetAuxData);
	REG_FUNC(libsjvtd, sjvtdGetSideInfoSeq);
	REG_FUNC(libsjvtd, sjvtdGetSideInfoPic);
	REG_FUNC(libsjvtd, sjvtdSetAuxData);
	REG_FUNC(libsjvtd, sjvtdSetDecodeMode);
	REG_FUNC(libsjvtd, sjvtdDecodePicture);
	REG_FUNC(libsjvtd, sjvtdGetExistenceFlag);
	REG_FUNC(libsjvtd, sjvtdDiscontinue);
});
