#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "libsvc1d.h"

LOG_CHANNEL(libsvc1d)

template<>
void fmt_class_string<Svc1dError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(SVC1D_ERROR_ARG);
		}

		return unknown;
	});
}

error_code svc1dGetMemorySize([[maybe_unused]] ppu_thread& ppu, vm::ptr<u32> memSize, u32 profileLevel)
{
	libsvc1d.todo("svc1dGetMemorySize(memSize=*0x%x, profileLevel=%d)", memSize, profileLevel);
	return CELL_OK;
}

error_code svc1dGetMemorySize2([[maybe_unused]] ppu_thread& ppu, vm::ptr<u32> memSize, u32 profileLevel, vm::cptr<Svc1dParams> params)
{
	libsvc1d.todo("svc1dGetMemorySize2(memSize=*0x%x, profileLevel=%d, params=*0x%x)", memSize, profileLevel, params);
	return CELL_OK;
}

error_code svc1dGetVersionNumber([[maybe_unused]] ppu_thread& ppu, vm::ptr<u32> version)
{
	libsvc1d.notice("svc1dGetVersionNumber(version=*0x%x)", version);

	if (!version)
	{
		return SVC1D_ERROR_ARG;
	}

	*version = 0x1090000;

	return CELL_OK;
}

error_code svc1dInitialize()
{
	UNIMPLEMENTED_FUNC(libsvc1d);
	return CELL_OK;
}

error_code svc1dCreateInstance2()
{
	UNIMPLEMENTED_FUNC(libsvc1d);
	return CELL_OK;
}

error_code svc1dReleaseInstance()
{
	UNIMPLEMENTED_FUNC(libsvc1d);
	return CELL_OK;
}

error_code svc1dCancelDecode()
{
	UNIMPLEMENTED_FUNC(libsvc1d);
	return CELL_OK;
}

error_code svc1dFlushPicture()
{
	UNIMPLEMENTED_FUNC(libsvc1d);
	return CELL_OK;
}

error_code svc1dGetNumPicture()
{
	UNIMPLEMENTED_FUNC(libsvc1d);
	return CELL_OK;
}

error_code svc1dGetPicture()
{
	UNIMPLEMENTED_FUNC(libsvc1d);
	return CELL_OK;
}

error_code svc1dReleasePicture()
{
	UNIMPLEMENTED_FUNC(libsvc1d);
	return CELL_OK;
}

error_code svc1dGetAuxData()
{
	UNIMPLEMENTED_FUNC(libsvc1d);
	return CELL_OK;
}

error_code svc1dGetSideInfoSeq()
{
	UNIMPLEMENTED_FUNC(libsvc1d);
	return CELL_OK;
}

error_code svc1dGetSideInfoPic()
{
	UNIMPLEMENTED_FUNC(libsvc1d);
	return CELL_OK;
}

error_code svc1dSetAuxData()
{
	UNIMPLEMENTED_FUNC(libsvc1d);
	return CELL_OK;
}

error_code svc1dSetPictureSize()
{
	UNIMPLEMENTED_FUNC(libsvc1d);
	return CELL_OK;
}

error_code svc1dDecodeSeqHeader()
{
	UNIMPLEMENTED_FUNC(libsvc1d);
	return CELL_OK;
}

error_code svc1dSetDecodeMode()
{
	UNIMPLEMENTED_FUNC(libsvc1d);
	return CELL_OK;
}

error_code svc1dDecodePicture()
{
	UNIMPLEMENTED_FUNC(libsvc1d);
	return CELL_OK;
}

error_code svc1dDecodePicture2()
{
	UNIMPLEMENTED_FUNC(libsvc1d);
	return CELL_OK;
}

error_code svc1dGetExistenceFlag()
{
	UNIMPLEMENTED_FUNC(libsvc1d);
	return CELL_OK;
}

error_code svc1dDiscontinue()
{
	UNIMPLEMENTED_FUNC(libsvc1d);
	return CELL_OK;
}

DECLARE(ppu_module_manager::libsvc1d)("libsvc1d", []
{
	REG_FUNC(libsvc1d, svc1dGetMemorySize);
	REG_FUNC(libsvc1d, svc1dGetMemorySize2);
	REG_FUNC(libsvc1d, svc1dGetVersionNumber);
	REG_FUNC(libsvc1d, svc1dInitialize);
	REG_FUNC(libsvc1d, svc1dCreateInstance2);
	REG_FUNC(libsvc1d, svc1dReleaseInstance);
	REG_FUNC(libsvc1d, svc1dCancelDecode);
	REG_FUNC(libsvc1d, svc1dFlushPicture);
	REG_FUNC(libsvc1d, svc1dGetNumPicture);
	REG_FUNC(libsvc1d, svc1dGetPicture);
	REG_FUNC(libsvc1d, svc1dReleasePicture);
	REG_FUNC(libsvc1d, svc1dGetAuxData);
	REG_FUNC(libsvc1d, svc1dGetSideInfoSeq);
	REG_FUNC(libsvc1d, svc1dGetSideInfoPic);
	REG_FUNC(libsvc1d, svc1dSetAuxData);
	REG_FUNC(libsvc1d, svc1dSetPictureSize);
	REG_FUNC(libsvc1d, svc1dDecodeSeqHeader);
	REG_FUNC(libsvc1d, svc1dSetDecodeMode);
	REG_FUNC(libsvc1d, svc1dDecodePicture);
	REG_FUNC(libsvc1d, svc1dDecodePicture2);
	REG_FUNC(libsvc1d, svc1dGetExistenceFlag);
	REG_FUNC(libsvc1d, svc1dDiscontinue);
});
