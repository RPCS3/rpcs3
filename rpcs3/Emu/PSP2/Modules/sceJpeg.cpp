#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceJpeg.h"

logs::channel sceJpeg("sceJpeg");

s32 sceJpegInitMJpeg(s32 maxSplitDecoder)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceJpegFinishMJpeg()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceJpegDecodeMJpeg(
	vm::cptr<u8> pJpeg,
	u32 isize,
	vm::ptr<void> pRGBA,
	u32 osize,
	s32 decodeMode,
	vm::ptr<void> pTempBuffer,
	u32 tempBufferSize,
	vm::ptr<void> pCoefBuffer,
	u32 coefBufferSize)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceJpegDecodeMJpegYCbCr(
	vm::cptr<u8> pJpeg,
	u32 isize,
	vm::ptr<u8> pYCbCr,
	u32 osize,
	s32 decodeMode,
	vm::ptr<void> pCoefBuffer,
	u32 coefBufferSize)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceJpegMJpegCsc(
	vm::ptr<void> pRGBA,
	vm::cptr<u8> pYCbCr,
	s32 xysize,
	s32 iFrameWidth,
	s32 colorOption,
	s32 sampling)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceJpegGetOutputInfo(
	vm::cptr<u8> pJpeg,
	u32 isize,
	s32 outputFormat,
	s32 decodeMode,
	vm::ptr<SceJpegOutputInfo> pOutputInfo)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceJpegCreateSplitDecoder(vm::ptr<SceJpegSplitDecodeCtrl> pCtrl)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceJpegDeleteSplitDecoder(vm::ptr<SceJpegSplitDecodeCtrl> pCtrl)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceJpegSplitDecodeMJpeg(vm::ptr<SceJpegSplitDecodeCtrl> pCtrl)
{
	fmt::throw_exception("Unimplemented" HERE);
}


#define REG_FUNC(nid, name) REG_FNID(SceJpegUser, nid, name)

DECLARE(arm_module_manager::SceJpeg)("SceJpegUser", []()
{
	REG_FUNC(0xB030773B, sceJpegInitMJpeg);
	REG_FUNC(0x62842598, sceJpegFinishMJpeg);
	REG_FUNC(0x6215B095, sceJpegDecodeMJpeg);
	REG_FUNC(0x2A769BD8, sceJpegDecodeMJpegYCbCr);
	REG_FUNC(0xC2380E3A, sceJpegMJpegCsc);
	REG_FUNC(0x353BA9B0, sceJpegGetOutputInfo);
	REG_FUNC(0x123B4734, sceJpegCreateSplitDecoder);
	REG_FUNC(0xDE8D5FA1, sceJpegDeleteSplitDecoder);
	REG_FUNC(0x4598EC9C, sceJpegSplitDecodeMJpeg);
});
