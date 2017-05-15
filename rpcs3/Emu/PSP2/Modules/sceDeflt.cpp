#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceDeflt.h"

logs::channel sceDeflt("sceDeflt");

s32 sceGzipIsValid(vm::cptr<void> pSrcGzip)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceGzipGetInfo(vm::cptr<void> pSrcGzip, vm::cpptr<void> ppvExtra, vm::cpptr<char> ppszName, vm::cpptr<char> ppszComment, vm::ptr<u16> pusCrc, vm::cpptr<void> ppvData)
{
	fmt::throw_exception("Unimplemented" HERE);
}

vm::cptr<char> sceGzipGetName(vm::cptr<void> pSrcGzip)
{
	fmt::throw_exception("Unimplemented" HERE);
}

vm::cptr<char> sceGzipGetComment(vm::cptr<void> pSrcGzip)
{
	fmt::throw_exception("Unimplemented" HERE);
}

vm::cptr<void> sceGzipGetCompressedData(vm::cptr<void> pSrcGzip)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceGzipDecompress(vm::ptr<void> pDst, u32 uiBufSize, vm::cptr<void> pSrcGzip, vm::ptr<u32> puiCrc32)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceZlibIsValid(vm::cptr<void> pSrcZlib)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceZlibGetInfo(vm::cptr<void> pSrcZlib, vm::ptr<u8> pbCmf, vm::ptr<u8> pbFlg, vm::ptr<u32> puiDictId, vm::cpptr<void> ppvData)
{
	fmt::throw_exception("Unimplemented" HERE);
}

vm::cptr<void> sceZlibGetCompressedData(vm::cptr<void> pSrcZlib)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceZlibDecompress(vm::ptr<void> pDst, u32 uiBufSize, vm::cptr<void> pSrcZlib, vm::ptr<u32> puiAdler32)
{
	fmt::throw_exception("Unimplemented" HERE);
}

u32 sceZlibAdler32(u32 uiAdler, vm::cptr<u8> pSrc, u32 uiSize)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceDeflateDecompress(vm::ptr<void> pDst, u32 uiBufSize, vm::cptr<void> pSrcDeflate, vm::cpptr<void> ppNext)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceZipGetInfo(vm::cptr<void> pSrc, vm::cpptr<void> ppvExtra, vm::ptr<u32> puiCrc, vm::cpptr<void> ppvData)
{
	fmt::throw_exception("Unimplemented" HERE);
}


#define REG_FUNC(nid, name) REG_FNID(SceDeflt, nid, name)

DECLARE(arm_module_manager::SceDeflt)("SceDeflt", []()
{
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
