#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceJpegEnc;

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceJpegEnc, #name, name)

psv_log_base sceJpegEnc("SceJpegEnc", []()
{
	sceJpegEnc.on_load = nullptr;
	sceJpegEnc.on_unload = nullptr;
	sceJpegEnc.on_stop = nullptr;

	//REG_FUNC(0x2B55844D, sceJpegEncoderGetContextSize);
	//REG_FUNC(0x88DA92B4, sceJpegEncoderInit);
	//REG_FUNC(0xC60DE94C, sceJpegEncoderEncode);
	//REG_FUNC(0xC87AA849, sceJpegEncoderEnd);
	//REG_FUNC(0x9511F3BC, sceJpegEncoderSetValidRegion);
	//REG_FUNC(0xB2B828EC, sceJpegEncoderSetCompressionRatio);
	//REG_FUNC(0x2F58B12C, sceJpegEncoderSetHeaderMode);
	//REG_FUNC(0x25D52D97, sceJpegEncoderSetOutputAddr);
	//REG_FUNC(0x824A7D4F, sceJpegEncoderCsc);
});
