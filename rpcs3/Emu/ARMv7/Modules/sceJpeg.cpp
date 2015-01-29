#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceJpeg;

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceJpeg, #name, name)

psv_log_base sceJpeg("SceJpeg", []()
{
	sceJpeg.on_load = nullptr;
	sceJpeg.on_unload = nullptr;
	sceJpeg.on_stop = nullptr;

	//REG_FUNC(0xB030773B, sceJpegInitMJpeg);
	//REG_FUNC(0x62842598, sceJpegFinishMJpeg);
	//REG_FUNC(0x6215B095, sceJpegDecodeMJpeg);
	//REG_FUNC(0x2A769BD8, sceJpegDecodeMJpegYCbCr);
	//REG_FUNC(0xC2380E3A, sceJpegMJpegCsc);
	//REG_FUNC(0x353BA9B0, sceJpegGetOutputInfo);
	//REG_FUNC(0x123B4734, sceJpegCreateSplitDecoder);
	//REG_FUNC(0xDE8D5FA1, sceJpegDeleteSplitDecoder);
	//REG_FUNC(0x4598EC9C, sceJpegSplitDecodeMJpeg);
});
