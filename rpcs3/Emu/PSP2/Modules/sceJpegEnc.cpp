#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceJpegEnc.h"

logs::channel sceJpegEnc("sceJpegEnc");

s32 sceJpegEncoderGetContextSize()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceJpegEncoderInit(
	SceJpegEncoderContext context,
	s32 iFrameWidth,
	s32 iFrameHeight,
	s32 pixelFormat,
	vm::ptr<void> pJpeg,
	u32 oJpegbufSize)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceJpegEncoderEncode(
	SceJpegEncoderContext context,
	vm::cptr<void> pYCbCr)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceJpegEncoderEnd(SceJpegEncoderContext context)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceJpegEncoderSetValidRegion(
	SceJpegEncoderContext context,
	s32 iFrameWidth,
	s32 iFrameHeight)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceJpegEncoderSetCompressionRatio(
	SceJpegEncoderContext context,
	s32 compratio)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceJpegEncoderSetHeaderMode(
	SceJpegEncoderContext context,
	s32 headerMode)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceJpegEncoderSetOutputAddr(
	SceJpegEncoderContext context,
	vm::ptr<void> pJpeg,
	u32 oJpegbufSize)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceJpegEncoderCsc(
	SceJpegEncoderContext context,
	vm::ptr<void> pYCbCr,
	vm::cptr<void> pRGBA,
	s32 iFrameWidth,
	s32 inputPixelFormat)
{
	fmt::throw_exception("Unimplemented" HERE);
}


#define REG_FUNC(nid, name) REG_FNID(SceJpegEncUser, nid, name)

DECLARE(arm_module_manager::SceJpegEnc)("SceJpegEncUser", []()
{
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
