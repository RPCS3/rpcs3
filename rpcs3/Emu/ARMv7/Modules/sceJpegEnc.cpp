#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceJpegEnc;

typedef vm::psv::ptr<void> SceJpegEncoderContext;

s32 sceJpegEncoderGetContextSize()
{
	throw __FUNCTION__;
}

s32 sceJpegEncoderInit(
	SceJpegEncoderContext context,
	s32 iFrameWidth,
	s32 iFrameHeight,
	s32 pixelFormat,
	vm::psv::ptr<void> pJpeg,
	u32 oJpegbufSize)
{
	throw __FUNCTION__;
}

s32 sceJpegEncoderEncode(
	SceJpegEncoderContext context,
	vm::psv::ptr<const void> pYCbCr)
{
	throw __FUNCTION__;
}

s32 sceJpegEncoderEnd(SceJpegEncoderContext context)
{
	throw __FUNCTION__;
}

s32 sceJpegEncoderSetValidRegion(
	SceJpegEncoderContext context,
	s32 iFrameWidth,
	s32 iFrameHeight)
{
	throw __FUNCTION__;
}

s32 sceJpegEncoderSetCompressionRatio(
	SceJpegEncoderContext context,
	s32 compratio)
{
	throw __FUNCTION__;
}

s32 sceJpegEncoderSetHeaderMode(
	SceJpegEncoderContext context,
	s32 headerMode)
{
	throw __FUNCTION__;
}

s32 sceJpegEncoderSetOutputAddr(
	SceJpegEncoderContext context,
	vm::psv::ptr<void> pJpeg,
	u32 oJpegbufSize)
{
	throw __FUNCTION__;
}

s32 sceJpegEncoderCsc(
	SceJpegEncoderContext context,
	vm::psv::ptr<void> pYCbCr,
	vm::psv::ptr<const void> pRGBA,
	s32 iFrameWidth,
	s32 inputPixelFormat)
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func<(func_ptr)name>(nid, &sceJpegEnc, #name, name)

psv_log_base sceJpegEnc("SceJpegEnc", []()
{
	sceJpegEnc.on_load = nullptr;
	sceJpegEnc.on_unload = nullptr;
	sceJpegEnc.on_stop = nullptr;

	REG_FUNC(0x2B55844D, sceJpegEncoderGetContextSize);
	REG_FUNC(0x88DA92B4, sceJpegEncoderInit);
	REG_FUNC(0xC60DE94C, sceJpegEncoderEncode);
	REG_FUNC(0xC87AA849, sceJpegEncoderEnd);
	REG_FUNC(0x9511F3BC, sceJpegEncoderSetValidRegion);
	REG_FUNC(0xB2B828EC, sceJpegEncoderSetCompressionRatio);
	REG_FUNC(0x2F58B12C, sceJpegEncoderSetHeaderMode);
	REG_FUNC(0x25D52D97, sceJpegEncoderSetOutputAddr);
	REG_FUNC(0x824A7D4F, sceJpegEncoderCsc);
});
