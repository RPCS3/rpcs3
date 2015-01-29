#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceNgs;

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceNgs, #name, name)

psv_log_base sceNgs("SceNgs", []()
{
	sceNgs.on_load = nullptr;
	sceNgs.on_unload = nullptr;
	sceNgs.on_stop = nullptr;

	//REG_FUNC(0x6CE8B36F, sceNgsSystemGetRequiredMemorySize);
	//REG_FUNC(0xED14CF4A, sceNgsSystemInit);
	//REG_FUNC(0x684F080C, sceNgsSystemUpdate);
	//REG_FUNC(0x4A25BEBC, sceNgsSystemRelease);
	//REG_FUNC(0xB9D971F2, sceNgsSystemLock);
	//REG_FUNC(0x0A93EA96, sceNgsSystemUnlock);
	//REG_FUNC(0x5ADD22DC, sceNgsSystemSetParamErrorCallback);
	//REG_FUNC(0x64D80013, sceNgsSystemSetFlags);
	//REG_FUNC(0x477318C0, sceNgsRackGetRequiredMemorySize);
	//REG_FUNC(0x0A92E4EC, sceNgsRackInit);
	//REG_FUNC(0xFE1A98E9, sceNgsRackGetVoiceHandle);
	//REG_FUNC(0xDD5CA10B, sceNgsRackRelease);
	//REG_FUNC(0x534B6E3F, sceNgsRackSetParamErrorCallback);
	//REG_FUNC(0x1DDBEBEB, sceNgsVoiceInit);
	//REG_FUNC(0xFA0A0F34, sceNgsVoicePlay);
	//REG_FUNC(0xBB13373D, sceNgsVoiceKeyOff);
	//REG_FUNC(0x0E291AAD, sceNgsVoiceKill);
	//REG_FUNC(0xD7786E99, sceNgsVoicePause);
	//REG_FUNC(0x54CFB981, sceNgsVoiceResume);
	//REG_FUNC(0x8A88E665, sceNgsVoiceSetPreset);
	//REG_FUNC(0xAB6BEF8F, sceNgsVoiceLockParams);
	//REG_FUNC(0x3D46D8A7, sceNgsVoiceUnlockParams);
	//REG_FUNC(0xFB8174B1, sceNgsVoiceSetParamsBlock);
	//REG_FUNC(0x9AB87E71, sceNgsVoiceBypassModule);
	//REG_FUNC(0x24E909A8, sceNgsVoiceSetModuleCallback);
	//REG_FUNC(0x17A6F564, sceNgsVoiceSetFinishedCallback);
	//REG_FUNC(0xC9B8C0B4, sceNgsVoiceGetStateData);
	//REG_FUNC(0x5551410D, sceNgsVoiceGetInfo);
	//REG_FUNC(0xB307185E, sceNgsVoiceGetModuleType);
	//REG_FUNC(0x431BF3AB, sceNgsVoiceGetModuleBypass);
	//REG_FUNC(0xD668B49C, sceNgsPatchCreateRouting);
	//REG_FUNC(0x98703DBC, sceNgsPatchGetInfo);
	//REG_FUNC(0x01A52E3A, sceNgsVoiceGetOutputPatch);
	//REG_FUNC(0xD0C9AE5A, sceNgsPatchRemoveRouting);
	//REG_FUNC(0xA3C807BC, sceNgsVoicePatchSetVolume);
	//REG_FUNC(0xBD6F57F0, sceNgsVoicePatchSetVolumes);
	//REG_FUNC(0xA0F5402D, sceNgsVoicePatchSetVolumesMatrix);
	//REG_FUNC(0xF6B68C31, sceNgsVoiceDefGetEnvelopeBuss);
	//REG_FUNC(0x9DCF50F5, sceNgsVoiceDefGetReverbBuss);
	//REG_FUNC(0x214485D6, sceNgsVoiceDefGetPauserBuss);
	//REG_FUNC(0xE0AC8776, sceNgsVoiceDefGetMixerBuss);
	//REG_FUNC(0x79A121D1, sceNgsVoiceDefGetMasterBuss);
	//REG_FUNC(0x0E0ACB68, sceNgsVoiceDefGetCompressorBuss);
	//REG_FUNC(0x1AF83512, sceNgsVoiceDefGetCompressorSideChainBuss);
	//REG_FUNC(0xAAD90DEB, sceNgsVoiceDefGetDistortionBuss);
	//REG_FUNC(0xF964120E, sceNgsVoiceDefGetEqBuss);
	//REG_FUNC(0xE9B572B7, sceNgsVoiceDefGetTemplate1);
	//REG_FUNC(0x0D5399CF, sceNgsVoiceDefGetSimpleVoice);
	//REG_FUNC(0x1F51C2BA, sceNgsVoiceDefGetSasEmuVoice);
	//REG_FUNC(0x4CBE08F3, sceNgsVoiceGetParamsOutOfRange);
	//REG_FUNC(0x14EF65A0, sceNgsVoiceDefGetAtrac9Voice);
	//REG_FUNC(0x4D705E3E, sceNgsVoiceDefGetDelayBuss);
	//REG_FUNC(0x5FD8AEDB, sceSulphaNgsGetDefaultConfig);
	//REG_FUNC(0x793E3E8C, sceSulphaNgsGetNeededMemory);
	//REG_FUNC(0xAFCD824F, sceSulphaNgsInit);
	//REG_FUNC(0xD124BFB1, sceSulphaNgsShutdown);
	//REG_FUNC(0x2F3F7515, sceSulphaNgsSetSynthName);
	//REG_FUNC(0x251AF6A9, sceSulphaNgsSetRackName);
	//REG_FUNC(0x508975BD, sceSulphaNgsSetVoiceName);
	//REG_FUNC(0x54EC5B8D, sceSulphaNgsSetSampleName);
	//REG_FUNC(0xDC7C0F05, sceSulphaNgsTrace);
	//REG_FUNC(0x5C71FE09, sceNgsModuleGetNumPresets);
	//REG_FUNC(0xC58298A7, sceNgsModuleGetPreset);
});
