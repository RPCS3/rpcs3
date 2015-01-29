#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceAudiodec;

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceAudiodec, #name, name)

psv_log_base sceAudiodec("SceAudiodec", []()
{
	sceAudiodec.on_load = nullptr;
	sceAudiodec.on_unload = nullptr;
	sceAudiodec.on_stop = nullptr;

	//REG_FUNC(0x445C2CEF, sceAudiodecInitLibrary);
	//REG_FUNC(0x45719B9D, sceAudiodecTermLibrary);
	//REG_FUNC(0x4DFD3AAA, sceAudiodecCreateDecoder);
	//REG_FUNC(0xE7A24E16, sceAudiodecDeleteDecoder);
	//REG_FUNC(0xCCDABA04, sceAudiodecDecode);
	//REG_FUNC(0xF72F9B64, sceAudiodecClearContext);
	//REG_FUNC(0x883B0CF5, sceAudiodecGetInternalError);
});
