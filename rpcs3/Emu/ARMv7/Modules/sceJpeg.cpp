#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceJpeg;

struct SceJpegOutputInfo
{
	s32 colorSpace;
	u16 imageWidth;
	u16 imageHeight;
	u32 outputBufferSize;
	u32 tempBufferSize;
	u32 coefBufferSize;

	struct { u32 x, y; } pitch[4];
};

struct SceJpegSplitDecodeCtrl
{
	vm::psv::ptr<u8> pStreamBuffer;
	u32 streamBufferSize;
	vm::psv::ptr<u8> pWriteBuffer;
	u32 writeBufferSize;
	s32 isEndOfStream;
	s32 decodeMode;

	SceJpegOutputInfo outputInfo;

	vm::psv::ptr<void> pOutputBuffer;
	vm::psv::ptr<void> pCoefBuffer;

	u32 internalData[3];
};

s32 sceJpegInitMJpeg(s32 maxSplitDecoder)
{
	throw __FUNCTION__;
}

s32 sceJpegFinishMJpeg()
{
	throw __FUNCTION__;
}

s32 sceJpegDecodeMJpeg(
	vm::psv::ptr<const u8> pJpeg,
	u32 isize,
	vm::psv::ptr<void> pRGBA,
	u32 osize,
	s32 decodeMode,
	vm::psv::ptr<void> pTempBuffer,
	u32 tempBufferSize,
	vm::psv::ptr<void> pCoefBuffer,
	u32 coefBufferSize)
{
	throw __FUNCTION__;
}

s32 sceJpegDecodeMJpegYCbCr(
	vm::psv::ptr<const u8> pJpeg,
	u32 isize,
	vm::psv::ptr<u8> pYCbCr,
	u32 osize,
	s32 decodeMode,
	vm::psv::ptr<void> pCoefBuffer,
	u32 coefBufferSize)
{
	throw __FUNCTION__;
}

s32 sceJpegMJpegCsc(
	vm::psv::ptr<void> pRGBA,
	vm::psv::ptr<const u8> pYCbCr,
	s32 xysize,
	s32 iFrameWidth,
	s32 colorOption,
	s32 sampling)
{
	throw __FUNCTION__;
}

s32 sceJpegGetOutputInfo(
	vm::psv::ptr<const u8> pJpeg,
	u32 isize,
	s32 outputFormat,
	s32 decodeMode,
	vm::psv::ptr<SceJpegOutputInfo> pOutputInfo)
{
	throw __FUNCTION__;
}

s32 sceJpegCreateSplitDecoder(vm::psv::ptr<SceJpegSplitDecodeCtrl> pCtrl)
{
	throw __FUNCTION__;
}

s32 sceJpegDeleteSplitDecoder(vm::psv::ptr<SceJpegSplitDecodeCtrl> pCtrl)
{
	throw __FUNCTION__;
}

s32 sceJpegSplitDecodeMJpeg(vm::psv::ptr<SceJpegSplitDecodeCtrl> pCtrl)
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func<name>(nid, &sceJpeg, #name, name)

psv_log_base sceJpeg("SceJpeg", []()
{
	sceJpeg.on_load = nullptr;
	sceJpeg.on_unload = nullptr;
	sceJpeg.on_stop = nullptr;

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
