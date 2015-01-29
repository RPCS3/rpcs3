#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceAudioenc;

struct SceAudioencInitStreamParam
{
	u32 size;
	u32 totalStreams;
};

struct SceAudioencInfoCelp
{
	u32 size;
	u32 excitationMode;
	u32 samplingRate;
	u32 bitRate;
};

struct SceAudioencOptInfoCelp
{
	u32 size;
	u8 header[32];
	u32 headerSize;
	u32 encoderVersion;
};


union SceAudioencInitParam
{
	u32 size;
	SceAudioencInitStreamParam celp;
};

union SceAudioencInfo
{
	u32 size;
	SceAudioencInfoCelp celp;
};

union SceAudioencOptInfo
{
	u32 size;
	SceAudioencOptInfoCelp celp;
};

struct SceAudioencCtrl
{
	u32 size;
	s32 handle;
	vm::psv::ptr<u8> pInputPcm;
	u32 inputPcmSize;
	u32 maxPcmSize;
	vm::psv::ptr<void> pOutputEs;
	u32 outputEsSize;
	u32 maxEsSize;
	u32 wordLength;
	vm::psv::ptr<SceAudioencInfo> pInfo;
	vm::psv::ptr<SceAudioencOptInfo> pOptInfo;
};


s32 sceAudioencInitLibrary(u32 codecType, vm::psv::ptr<SceAudioencInitParam> pInitParam)
{
	throw __FUNCTION__;
}

s32 sceAudioencTermLibrary(u32 codecType)
{
	throw __FUNCTION__;
}

s32 sceAudioencCreateEncoder(vm::psv::ptr<SceAudioencCtrl> pCtrl, u32 codecType)
{
	throw __FUNCTION__;
}

s32 sceAudioencDeleteEncoder(vm::psv::ptr<SceAudioencCtrl> pCtrl)
{
	throw __FUNCTION__;
}

s32 sceAudioencEncode(vm::psv::ptr<SceAudioencCtrl> pCtrl)
{
	throw __FUNCTION__;
}

s32 sceAudioencClearContext(vm::psv::ptr<SceAudioencCtrl> pCtrl)
{
	throw __FUNCTION__;
}

s32 sceAudioencGetOptInfo(vm::psv::ptr<SceAudioencCtrl> pCtrl)
{
	throw __FUNCTION__;
}

s32 sceAudioencGetInternalError(vm::psv::ptr<SceAudioencCtrl> pCtrl, vm::psv::ptr<s32> pInternalError)
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func(nid, &sceAudioenc, #name, name)

psv_log_base sceAudioenc("SceAudioenc", []()
{
	sceAudioenc.on_load = nullptr;
	sceAudioenc.on_unload = nullptr;
	sceAudioenc.on_stop = nullptr;

	REG_FUNC(0x76EE4DC6, sceAudioencInitLibrary);
	REG_FUNC(0xAB32D022, sceAudioencTermLibrary);
	REG_FUNC(0x64C04AE8, sceAudioencCreateEncoder);
	REG_FUNC(0xC6BA5EE6, sceAudioencDeleteEncoder);
	REG_FUNC(0xD85DB29C, sceAudioencEncode);
	REG_FUNC(0x9386F42D, sceAudioencClearContext);
	REG_FUNC(0xD01C63A3, sceAudioencGetOptInfo);
	REG_FUNC(0x452246D0, sceAudioencGetInternalError);
});
