#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "libavcdec.h"

LOG_CHANNEL(libavcdec)

template<>
void fmt_class_string<AvcDecError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(AVC_DEC_ERROR_ARG);
		}

		return unknown;
	});
}

error_code avcDecQueryMemory([[maybe_unused]] ppu_thread& ppu, vm::cptr<AvcDecParams> params, vm::ptr<AvcDecAttr> attr)
{
	libavcdec.todo("avcDecQueryMemory(params=*0x%x, attr=*0x%x)", params, attr);
	return CELL_OK;
}

void avcDecGetVersion([[maybe_unused]] ppu_thread& ppu, vm::ptr<u32> version)
{
	libavcdec.notice("avcDecGetVersion(version=*0x%x)", version);

	if (version)
	{
		*version = 0x11300;
	}
}

error_code avcDecQueryCharacteristics([[maybe_unused]] ppu_thread& ppu, vm::ptr<u32> unk)
{
	libavcdec.notice("avcDecGetVersion(unk=*0x%x)", unk);

	if (!unk)
	{
		return AVC_DEC_ERROR_ARG;
	}

	unk[0] = 5; // Command queue depth
	unk[1] = 20;

	return CELL_OK;
}

error_code avcDecOpen()
{
	UNIMPLEMENTED_FUNC(libavcdec);
	return CELL_OK;
}

error_code avcDecClose()
{
	UNIMPLEMENTED_FUNC(libavcdec);
	return CELL_OK;
}

error_code avcDecReset()
{
	UNIMPLEMENTED_FUNC(libavcdec);
	return CELL_OK;
}

error_code avcDecFlush()
{
	UNIMPLEMENTED_FUNC(libavcdec);
	return CELL_OK;
}

error_code avcDecAccessUnit()
{
	UNIMPLEMENTED_FUNC(libavcdec);
	return CELL_OK;
}

error_code avcDecRlsFrame()
{
	UNIMPLEMENTED_FUNC(libavcdec);
	return CELL_OK;
}

error_code avcDecGetStatus()
{
	UNIMPLEMENTED_FUNC(libavcdec);
	return CELL_OK;
}

DECLARE(ppu_module_manager::libavcdec)("libavcdec", []
{
	REG_FUNC(libavcdec, avcDecQueryMemory);
	REG_FUNC(libavcdec, avcDecGetVersion);
	REG_FUNC(libavcdec, avcDecQueryCharacteristics);
	REG_FUNC(libavcdec, avcDecOpen);
	REG_FUNC(libavcdec, avcDecClose);
	REG_FUNC(libavcdec, avcDecReset);
	REG_FUNC(libavcdec, avcDecFlush);
	REG_FUNC(libavcdec, avcDecAccessUnit);
	REG_FUNC(libavcdec, avcDecRlsFrame);
	REG_FUNC(libavcdec, avcDecGetStatus);
});
