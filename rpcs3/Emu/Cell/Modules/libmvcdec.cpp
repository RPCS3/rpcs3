#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "libmvcdec.h"

LOG_CHANNEL(libmvcdec)

template<>
void fmt_class_string<MvcDecError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(MVC_DEC_ERROR_ARG);
		}

		return unknown;
	});
}

error_code mvcDecQueryMemory([[maybe_unused]] ppu_thread& ppu, vm::cptr<AvcDecParams> params, vm::ptr<AvcDecAttr> attr)
{
	libmvcdec.todo("mvcDecQueryMemory(params=*0x%x, attr=*0x%x)", params, attr);
	return CELL_OK;
}

void mvcDecGetVersion([[maybe_unused]] ppu_thread& ppu, vm::ptr<u32> version)
{
	libmvcdec.notice("mvcDecGetVersion(version=*0x%x)", version);

	if (version)
	{
		*version = 0x10804;
	}
}

error_code mvcDecQueryCharacteristics([[maybe_unused]] ppu_thread& ppu, vm::ptr<u32> unk)
{
	libmvcdec.notice("mvcDecGetVersion(unk=*0x%x)", unk);

	if (!unk)
	{
		return MVC_DEC_ERROR_ARG;
	}

	unk[0] = 5; // Command queue depth
	unk[1] = 36;

	return CELL_OK;
}

error_code mvcDecOpen()
{
	UNIMPLEMENTED_FUNC(libmvcdec);
	return CELL_OK;
}

error_code mvcDecClose()
{
	UNIMPLEMENTED_FUNC(libmvcdec);
	return CELL_OK;
}

error_code mvcDecReset()
{
	UNIMPLEMENTED_FUNC(libmvcdec);
	return CELL_OK;
}

error_code mvcDecFlush()
{
	UNIMPLEMENTED_FUNC(libmvcdec);
	return CELL_OK;
}

error_code mvcDecAccessUnit()
{
	UNIMPLEMENTED_FUNC(libmvcdec);
	return CELL_OK;
}

error_code mvcDecRlsFrame()
{
	UNIMPLEMENTED_FUNC(libmvcdec);
	return CELL_OK;
}

DECLARE(ppu_module_manager::libmvcdec)("libmvcdec", []
{
	REG_FUNC(libmvcdec, mvcDecQueryMemory);
	REG_FUNC(libmvcdec, mvcDecGetVersion);
	REG_FUNC(libmvcdec, mvcDecQueryCharacteristics);
	REG_FUNC(libmvcdec, mvcDecOpen);
	REG_FUNC(libmvcdec, mvcDecClose);
	REG_FUNC(libmvcdec, mvcDecReset);
	REG_FUNC(libmvcdec, mvcDecFlush);
	REG_FUNC(libmvcdec, mvcDecAccessUnit);
	REG_FUNC(libmvcdec, mvcDecRlsFrame);
});
