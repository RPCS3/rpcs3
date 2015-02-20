#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceAudiodec;

struct SceAudiodecInitStreamParam
{
	u32 size;
	u32 totalStreams;
};

struct SceAudiodecInitChParam
{
	u32 size;
	u32 totalCh;
};

union SceAudiodecInitParam
{
	u32 size;
	SceAudiodecInitChParam     at9;
	SceAudiodecInitStreamParam mp3;
	SceAudiodecInitStreamParam aac;
	SceAudiodecInitStreamParam celp;
};

struct SceAudiodecInfoAt9
{
	u32 size;
	u8 configData[4];
	u32 ch;
	u32 bitRate;
	u32 samplingRate;
	u32 superFrameSize;
	u32 framesInSuperFrame;
};

struct SceAudiodecInfoMp3
{
	u32 size;
	u32 ch;
	u32 version;
};

struct SceAudiodecInfoAac
{
	u32 size;
	u32 isAdts;
	u32 ch;
	u32 samplingRate;
	u32 isSbr;
};

struct SceAudiodecInfoCelp
{
	u32 size;
	u32 excitationMode;
	u32 samplingRate;
	u32 bitRate;
	u32 lostCount;
};

union SceAudiodecInfo
{
	u32 size;
	SceAudiodecInfoAt9  at9;
	SceAudiodecInfoMp3  mp3;
	SceAudiodecInfoAac  aac;
	SceAudiodecInfoCelp celp;
};

struct SceAudiodecCtrl
{
	u32 size;
	s32 handle;
	vm::psv::ptr<u8> pEs;
	u32 inputEsSize;
	u32 maxEsSize;
	vm::psv::ptr<void> pPcm;
	u32 outputPcmSize;
	u32 maxPcmSize;
	u32 wordLength;
	vm::psv::ptr<SceAudiodecInfo> pInfo;
};

s32 sceAudiodecInitLibrary(u32 codecType, vm::psv::ptr<SceAudiodecInitParam> pInitParam)
{
	throw __FUNCTION__;
}

s32 sceAudiodecTermLibrary(u32 codecType)
{
	throw __FUNCTION__;
}

s32 sceAudiodecCreateDecoder(vm::psv::ptr<SceAudiodecCtrl> pCtrl, u32 codecType)
{
	throw __FUNCTION__;
}

s32 sceAudiodecDeleteDecoder(vm::psv::ptr<SceAudiodecCtrl> pCtrl)
{
	throw __FUNCTION__;
}

s32 sceAudiodecDecode(vm::psv::ptr<SceAudiodecCtrl> pCtrl)
{
	throw __FUNCTION__;
}

s32 sceAudiodecClearContext(vm::psv::ptr<SceAudiodecCtrl> pCtrl)
{
	throw __FUNCTION__;
}

s32 sceAudiodecGetInternalError(vm::psv::ptr<SceAudiodecCtrl> pCtrl, vm::psv::ptr<s32> pInternalError)
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func<name>(nid, &sceAudiodec, #name, name)

psv_log_base sceAudiodec("SceAudiodec", []()
{
	sceAudiodec.on_load = nullptr;
	sceAudiodec.on_unload = nullptr;
	sceAudiodec.on_stop = nullptr;

	REG_FUNC(0x445C2CEF, sceAudiodecInitLibrary);
	REG_FUNC(0x45719B9D, sceAudiodecTermLibrary);
	REG_FUNC(0x4DFD3AAA, sceAudiodecCreateDecoder);
	REG_FUNC(0xE7A24E16, sceAudiodecDeleteDecoder);
	REG_FUNC(0xCCDABA04, sceAudiodecDecode);
	REG_FUNC(0xF72F9B64, sceAudiodecClearContext);
	REG_FUNC(0x883B0CF5, sceAudiodecGetInternalError);
});
