#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceJpeg.h"

s32 sceJpegInitMJpeg(s32 maxSplitDecoder)
{
	throw __FUNCTION__;
}

s32 sceJpegFinishMJpeg()
{
	throw __FUNCTION__;
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
	throw __FUNCTION__;
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
	throw __FUNCTION__;
}

s32 sceJpegMJpegCsc(
	vm::ptr<void> pRGBA,
	vm::cptr<u8> pYCbCr,
	s32 xysize,
	s32 iFrameWidth,
	s32 colorOption,
	s32 sampling)
{
	throw __FUNCTION__;
}

s32 sceJpegGetOutputInfo(
	vm::cptr<u8> pJpeg,
	u32 isize,
	s32 outputFormat,
	s32 decodeMode,
	vm::ptr<SceJpegOutputInfo> pOutputInfo)
{
	throw __FUNCTION__;
}

s32 sceJpegCreateSplitDecoder(vm::ptr<SceJpegSplitDecodeCtrl> pCtrl)
{
	throw __FUNCTION__;
}

s32 sceJpegDeleteSplitDecoder(vm::ptr<SceJpegSplitDecodeCtrl> pCtrl)
{
	throw __FUNCTION__;
}

s32 sceJpegSplitDecodeMJpeg(vm::ptr<SceJpegSplitDecodeCtrl> pCtrl)
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func(nid, &sceJpeg, #name, name)

psv_log_base sceJpeg("SceJpeg", []()
{
	sceJpeg.on_load = nullptr;
	sceJpeg.on_unload = nullptr;
	sceJpeg.on_stop = nullptr;
	sceJpeg.on_error = nullptr;

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
