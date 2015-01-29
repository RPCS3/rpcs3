#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceDeflt;

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceDeflt, #name, name)

psv_log_base sceDeflt("SceDeflt", []()
{
	sceDeflt.on_load = nullptr;
	sceDeflt.on_unload = nullptr;
	sceDeflt.on_stop = nullptr;

	//REG_FUNC(0xCD83A464, sceZlibAdler32);
	//REG_FUNC(0x110D5050, sceDeflateDecompress);
	//REG_FUNC(0xE3CB51A3, sceGzipDecompress);
	//REG_FUNC(0xBABCF5CF, sceGzipGetComment);
	//REG_FUNC(0xE1844802, sceGzipGetCompressedData);
	//REG_FUNC(0x1B8E5862, sceGzipGetInfo);
	//REG_FUNC(0xAEBAABE6, sceGzipGetName);
	//REG_FUNC(0xDEDADC31, sceGzipIsValid);
	//REG_FUNC(0xE38F754D, sceZlibDecompress);
	//REG_FUNC(0xE680A65A, sceZlibGetCompressedData);
	//REG_FUNC(0x4C0A685D, sceZlibGetInfo);
	//REG_FUNC(0x14A0698D, sceZlibIsValid);
});
