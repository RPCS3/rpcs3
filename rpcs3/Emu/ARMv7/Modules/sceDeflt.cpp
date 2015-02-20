#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceDeflt;

s32 sceGzipIsValid(vm::psv::ptr<const void> pSrcGzip)
{
	throw __FUNCTION__;
}

s32 sceGzipGetInfo(vm::psv::ptr<const void> pSrcGzip, vm::psv::ptr<vm::psv::ptr<const void>> ppvExtra, vm::psv::ptr<vm::psv::ptr<const char>> ppszName, vm::psv::ptr<vm::psv::ptr<const char>> ppszComment, vm::psv::ptr<u16> pusCrc, vm::psv::ptr<vm::psv::ptr<const void>> ppvData)
{
	throw __FUNCTION__;
}

vm::psv::ptr<const char> sceGzipGetName(vm::psv::ptr<const void> pSrcGzip)
{
	throw __FUNCTION__;
}

vm::psv::ptr<const char> sceGzipGetComment(vm::psv::ptr<const void> pSrcGzip)
{
	throw __FUNCTION__;
}

vm::psv::ptr<const void> sceGzipGetCompressedData(vm::psv::ptr<const void> pSrcGzip)
{
	throw __FUNCTION__;
}

s32 sceGzipDecompress(vm::psv::ptr<void> pDst, u32 uiBufSize, vm::psv::ptr<const void> pSrcGzip, vm::psv::ptr<u32> puiCrc32)
{
	throw __FUNCTION__;
}

s32 sceZlibIsValid(vm::psv::ptr<const void> pSrcZlib)
{
	throw __FUNCTION__;
}

s32 sceZlibGetInfo(vm::psv::ptr<const void> pSrcZlib, vm::psv::ptr<u8> pbCmf, vm::psv::ptr<u8> pbFlg, vm::psv::ptr<u32> puiDictId, vm::psv::ptr<vm::psv::ptr<const void>> ppvData)
{
	throw __FUNCTION__;
}

vm::psv::ptr<const void> sceZlibGetCompressedData(vm::psv::ptr<const void> pSrcZlib)
{
	throw __FUNCTION__;
}

s32 sceZlibDecompress(vm::psv::ptr<void> pDst, u32 uiBufSize, vm::psv::ptr<const void> pSrcZlib, vm::psv::ptr<u32> puiAdler32)
{
	throw __FUNCTION__;
}

u32 sceZlibAdler32(u32 uiAdler, vm::psv::ptr<const u8> pSrc, u32 uiSize)
{
	throw __FUNCTION__;
}

s32 sceDeflateDecompress(vm::psv::ptr<void> pDst, u32 uiBufSize, vm::psv::ptr<const void> pSrcDeflate, vm::psv::ptr<vm::psv::ptr<const void>> ppNext)
{
	throw __FUNCTION__;
}

s32 sceZipGetInfo(vm::psv::ptr<const void> pSrc, vm::psv::ptr<vm::psv::ptr<const void>> ppvExtra, vm::psv::ptr<u32> puiCrc, vm::psv::ptr<vm::psv::ptr<const void>> ppvData)
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func(nid, &sceDeflt, #name, name)

psv_log_base sceDeflt("SceDeflt", []()
{
	sceDeflt.on_load = nullptr;
	sceDeflt.on_unload = nullptr;
	sceDeflt.on_stop = nullptr;

	REG_FUNC(0xCD83A464, sceZlibAdler32);
	REG_FUNC(0x110D5050, sceDeflateDecompress);
	REG_FUNC(0xE3CB51A3, sceGzipDecompress);
	REG_FUNC(0xBABCF5CF, sceGzipGetComment);
	REG_FUNC(0xE1844802, sceGzipGetCompressedData);
	REG_FUNC(0x1B8E5862, sceGzipGetInfo);
	REG_FUNC(0xAEBAABE6, sceGzipGetName);
	REG_FUNC(0xDEDADC31, sceGzipIsValid);
	REG_FUNC(0xE38F754D, sceZlibDecompress);
	REG_FUNC(0xE680A65A, sceZlibGetCompressedData);
	REG_FUNC(0x4C0A685D, sceZlibGetInfo);
	REG_FUNC(0x14A0698D, sceZlibIsValid);
});
