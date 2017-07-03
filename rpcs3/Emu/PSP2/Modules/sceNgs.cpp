#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceNgs.h"

logs::channel sceNgs("sceNgs");

s32 sceNgsSystemGetRequiredMemorySize(vm::cptr<SceNgsSystemInitParams> pSynthParams, vm::ptr<u32> pnSize)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsSystemInit(vm::ptr<void> pSynthSysMemory, const u32 uMemSize, vm::cptr<SceNgsSystemInitParams> pSynthParams, vm::ptr<SceNgsHSynSystem> pSystemHandle)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsSystemUpdate(SceNgsHSynSystem hSystemHandle)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsSystemRelease(SceNgsHSynSystem hSystemHandle)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsSystemLock(SceNgsHSynSystem hSystemHandle)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsSystemUnlock(SceNgsHSynSystem hSystemHandle)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsSystemSetParamErrorCallback(SceNgsHSynSystem hSystemHandle, vm::ptr<SceNgsCallbackFunc> callbackFuncPtr)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsSystemSetFlags(SceNgsHSynSystem hSystemHandle, const u32 uSystemFlags)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsRackGetRequiredMemorySize(SceNgsHSynSystem hSystemHandle, vm::cptr<SceNgsRackDescription> pRackDesc, vm::ptr<u32> pnSize)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsRackInit(SceNgsHSynSystem hSystemHandle, vm::ptr<SceNgsBufferInfo> pRackBuffer, vm::cptr<SceNgsRackDescription> pRackDesc, vm::ptr<SceNgsHRack> pRackHandle)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsRackGetVoiceHandle(SceNgsHRack hRackHandle, const u32 uIndex, vm::ptr<SceNgsHVoice> pVoiceHandle)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsRackRelease(SceNgsHRack hRackHandle, vm::ptr<SceNgsCallbackFunc> callbackFuncPtr)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsRackSetParamErrorCallback(SceNgsHRack hRackHandle, vm::ptr<SceNgsCallbackFunc> callbackFuncPtr)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsVoiceInit(SceNgsHVoice hVoiceHandle, vm::cptr<SceNgsVoicePreset> pPreset, const u32 uInitFlags)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsVoicePlay(SceNgsHVoice hVoiceHandle)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsVoiceKeyOff(SceNgsHVoice hVoiceHandle)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsVoiceKill(SceNgsHVoice hVoiceHandle)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsVoicePause(SceNgsHVoice hVoiceHandle)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsVoiceResume(SceNgsHVoice hVoiceHandle)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsVoiceSetPreset(SceNgsHVoice hVoiceHandle, vm::cptr<SceNgsVoicePreset> pVoicePreset)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsVoiceLockParams(SceNgsHVoice hVoiceHandle, const u32 uModule, const u32 uParamsInterfaceId, vm::ptr<SceNgsBufferInfo> pParamsBuffer)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsVoiceUnlockParams(SceNgsHVoice hVoiceHandle, const u32 uModule)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsVoiceSetParamsBlock(SceNgsHVoice hVoiceHandle, vm::cptr<SceNgsModuleParamHeader> pParamData, const u32 uSize, vm::ptr<s32> pnErrorCount)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsVoiceBypassModule(SceNgsHVoice hVoiceHandle, const u32 uModule, const u32 uBypassFlag)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsVoiceSetModuleCallback(SceNgsHVoice hVoiceHandle, const u32 uModule, vm::ptr<SceNgsCallbackFunc> callbackFuncPtr, vm::ptr<void> pUserData)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsVoiceSetFinishedCallback(SceNgsHVoice hVoiceHandle, vm::ptr<SceNgsCallbackFunc> callbackFuncPtr, vm::ptr<void> pUserData)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsVoiceGetStateData(SceNgsHVoice hVoiceHandle, const u32 uModule, vm::ptr<void> pMem, const u32 uMemSize)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsVoiceGetInfo(SceNgsHVoice hVoiceHandle, vm::ptr<SceNgsVoiceInfo> pInfo)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsVoiceGetModuleType(SceNgsHVoice hVoiceHandle, const u32 uModule, vm::ptr<u32> pModuleType)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsVoiceGetModuleBypass(SceNgsHVoice hVoiceHandle, const u32 uModule, vm::ptr<u32> puBypassFlag)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsVoiceGetParamsOutOfRange(SceNgsHVoice hVoiceHandle, const u32 uModule, vm::ptr<char> pszMessageBuffer)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsPatchCreateRouting(vm::cptr<SceNgsPatchSetupInfo> pPatchInfo, vm::ptr<SceNgsHPatch> pPatchHandle)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsPatchGetInfo(SceNgsHPatch hPatchHandle, vm::ptr<SceNgsPatchRouteInfo> pRouteInfo, vm::ptr<SceNgsPatchSetupInfo> pSetup)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsVoiceGetOutputPatch(SceNgsHVoice hVoiceHandle, const s32 nOutputIndex, const s32 nSubIndex, vm::ptr<SceNgsHPatch> pPatchHandle)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsPatchRemoveRouting(SceNgsHPatch hPatchHandle)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsVoicePatchSetVolume(SceNgsHPatch hPatchHandle, const s32 nOutputChannel, const s32 nInputChannel, const float fVol)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsVoicePatchSetVolumes(SceNgsHPatch hPatchHandle, const s32 nOutputChannel, vm::cptr<float> pVolumes, const s32 nVols)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsVoicePatchSetVolumesMatrix(SceNgsHPatch hPatchHandle, vm::cptr<SceNgsVolumeMatrix> pMatrix)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsModuleGetNumPresets(SceNgsHSynSystem hSystemHandle, const u32 uModuleID, vm::ptr<u32> puNumPresets)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNgsModuleGetPreset(SceNgsHSynSystem hSystemHandle, const u32 uModuleID, const u32 uPresetIndex, vm::ptr<SceNgsBufferInfo> pParamsBuffer)
{
	fmt::throw_exception("Unimplemented" HERE);
}

vm::cptr<SceNgsVoiceDefinition> sceNgsVoiceDefGetCompressorBuss()
{
	fmt::throw_exception("Unimplemented" HERE);
}

vm::cptr<SceNgsVoiceDefinition> sceNgsVoiceDefGetCompressorSideChainBuss()
{
	fmt::throw_exception("Unimplemented" HERE);
}

vm::cptr<SceNgsVoiceDefinition> sceNgsVoiceDefGetDelayBuss()
{
	fmt::throw_exception("Unimplemented" HERE);
}

vm::cptr<SceNgsVoiceDefinition> sceNgsVoiceDefGetDistortionBuss()
{
	fmt::throw_exception("Unimplemented" HERE);
}

vm::cptr<SceNgsVoiceDefinition> sceNgsVoiceDefGetEnvelopeBuss()
{
	fmt::throw_exception("Unimplemented" HERE);
}

vm::cptr<SceNgsVoiceDefinition> sceNgsVoiceDefGetEqBuss()
{
	fmt::throw_exception("Unimplemented" HERE);
}

vm::cptr<SceNgsVoiceDefinition> sceNgsVoiceDefGetMasterBuss()
{
	fmt::throw_exception("Unimplemented" HERE);
}

vm::cptr<SceNgsVoiceDefinition> sceNgsVoiceDefGetMixerBuss()
{
	fmt::throw_exception("Unimplemented" HERE);
}

vm::cptr<SceNgsVoiceDefinition> sceNgsVoiceDefGetPauserBuss()
{
	fmt::throw_exception("Unimplemented" HERE);
}

vm::cptr<SceNgsVoiceDefinition> sceNgsVoiceDefGetReverbBuss()
{
	fmt::throw_exception("Unimplemented" HERE);
}

vm::cptr<SceNgsVoiceDefinition> sceNgsVoiceDefGetSasEmuVoice()
{
	fmt::throw_exception("Unimplemented" HERE);
}

vm::cptr<SceNgsVoiceDefinition> sceNgsVoiceDefGetSimpleVoice()
{
	fmt::throw_exception("Unimplemented" HERE);
}

vm::cptr<SceNgsVoiceDefinition> sceNgsVoiceDefGetTemplate1()
{
	fmt::throw_exception("Unimplemented" HERE);
}

vm::cptr<SceNgsVoiceDefinition> sceNgsVoiceDefGetAtrac9Voice()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSulphaNgsGetDefaultConfig(vm::ptr<SceSulphaNgsConfig> config)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSulphaNgsGetNeededMemory(vm::cptr<SceSulphaNgsConfig> config, vm::ptr<u32> sizeInBytes)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSulphaNgsInit(vm::cptr<SceSulphaNgsConfig> config, vm::ptr<void> buffer, u32 sizeInBytes)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSulphaNgsShutdown()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSulphaNgsSetSynthName(SceNgsHSynSystem synthHandle, vm::cptr<char> name)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSulphaNgsSetRackName(SceNgsHRack rackHandle, vm::cptr<char> name)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSulphaNgsSetVoiceName(SceNgsHVoice voiceHandle, vm::cptr<char> name)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSulphaNgsSetSampleName(vm::cptr<void> location, u32 length, vm::cptr<char> name)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSulphaNgsTrace(vm::cptr<char> message)
{
	fmt::throw_exception("Unimplemented" HERE);
}


#define REG_FUNC(nid, name) REG_FNID(SceNgs, nid, name)

DECLARE(arm_module_manager::SceNgs)("SceNgs", []()
{
	REG_FUNC(0x6CE8B36F, sceNgsSystemGetRequiredMemorySize);
	REG_FUNC(0xED14CF4A, sceNgsSystemInit);
	REG_FUNC(0x684F080C, sceNgsSystemUpdate);
	REG_FUNC(0x4A25BEBC, sceNgsSystemRelease);
	REG_FUNC(0xB9D971F2, sceNgsSystemLock);
	REG_FUNC(0x0A93EA96, sceNgsSystemUnlock);
	REG_FUNC(0x5ADD22DC, sceNgsSystemSetParamErrorCallback);
	REG_FUNC(0x64D80013, sceNgsSystemSetFlags);
	REG_FUNC(0x477318C0, sceNgsRackGetRequiredMemorySize);
	REG_FUNC(0x0A92E4EC, sceNgsRackInit);
	REG_FUNC(0xFE1A98E9, sceNgsRackGetVoiceHandle);
	REG_FUNC(0xDD5CA10B, sceNgsRackRelease);
	REG_FUNC(0x534B6E3F, sceNgsRackSetParamErrorCallback);
	REG_FUNC(0x1DDBEBEB, sceNgsVoiceInit);
	REG_FUNC(0xFA0A0F34, sceNgsVoicePlay);
	REG_FUNC(0xBB13373D, sceNgsVoiceKeyOff);
	REG_FUNC(0x0E291AAD, sceNgsVoiceKill);
	REG_FUNC(0xD7786E99, sceNgsVoicePause);
	REG_FUNC(0x54CFB981, sceNgsVoiceResume);
	REG_FUNC(0x8A88E665, sceNgsVoiceSetPreset);
	REG_FUNC(0xAB6BEF8F, sceNgsVoiceLockParams);
	REG_FUNC(0x3D46D8A7, sceNgsVoiceUnlockParams);
	REG_FUNC(0xFB8174B1, sceNgsVoiceSetParamsBlock);
	REG_FUNC(0x9AB87E71, sceNgsVoiceBypassModule);
	REG_FUNC(0x24E909A8, sceNgsVoiceSetModuleCallback);
	REG_FUNC(0x17A6F564, sceNgsVoiceSetFinishedCallback);
	REG_FUNC(0xC9B8C0B4, sceNgsVoiceGetStateData);
	REG_FUNC(0x5551410D, sceNgsVoiceGetInfo);
	REG_FUNC(0xB307185E, sceNgsVoiceGetModuleType);
	REG_FUNC(0x431BF3AB, sceNgsVoiceGetModuleBypass);
	REG_FUNC(0xD668B49C, sceNgsPatchCreateRouting);
	REG_FUNC(0x98703DBC, sceNgsPatchGetInfo);
	REG_FUNC(0x01A52E3A, sceNgsVoiceGetOutputPatch);
	REG_FUNC(0xD0C9AE5A, sceNgsPatchRemoveRouting);
	//REG_FUNC(0xA3C807BC, sceNgsVoicePatchSetVolume);
	REG_FUNC(0xBD6F57F0, sceNgsVoicePatchSetVolumes);
	REG_FUNC(0xA0F5402D, sceNgsVoicePatchSetVolumesMatrix);
	REG_FUNC(0xF6B68C31, sceNgsVoiceDefGetEnvelopeBuss);
	REG_FUNC(0x9DCF50F5, sceNgsVoiceDefGetReverbBuss);
	REG_FUNC(0x214485D6, sceNgsVoiceDefGetPauserBuss);
	REG_FUNC(0xE0AC8776, sceNgsVoiceDefGetMixerBuss);
	REG_FUNC(0x79A121D1, sceNgsVoiceDefGetMasterBuss);
	REG_FUNC(0x0E0ACB68, sceNgsVoiceDefGetCompressorBuss);
	REG_FUNC(0x1AF83512, sceNgsVoiceDefGetCompressorSideChainBuss);
	REG_FUNC(0xAAD90DEB, sceNgsVoiceDefGetDistortionBuss);
	REG_FUNC(0xF964120E, sceNgsVoiceDefGetEqBuss);
	REG_FUNC(0xE9B572B7, sceNgsVoiceDefGetTemplate1);
	REG_FUNC(0x0D5399CF, sceNgsVoiceDefGetSimpleVoice);
	REG_FUNC(0x1F51C2BA, sceNgsVoiceDefGetSasEmuVoice);
	REG_FUNC(0x4CBE08F3, sceNgsVoiceGetParamsOutOfRange);
	REG_FUNC(0x14EF65A0, sceNgsVoiceDefGetAtrac9Voice);
	REG_FUNC(0x4D705E3E, sceNgsVoiceDefGetDelayBuss);
	REG_FUNC(0x5FD8AEDB, sceSulphaNgsGetDefaultConfig);
	REG_FUNC(0x793E3E8C, sceSulphaNgsGetNeededMemory);
	REG_FUNC(0xAFCD824F, sceSulphaNgsInit);
	REG_FUNC(0xD124BFB1, sceSulphaNgsShutdown);
	REG_FUNC(0x2F3F7515, sceSulphaNgsSetSynthName);
	REG_FUNC(0x251AF6A9, sceSulphaNgsSetRackName);
	REG_FUNC(0x508975BD, sceSulphaNgsSetVoiceName);
	REG_FUNC(0x54EC5B8D, sceSulphaNgsSetSampleName);
	REG_FUNC(0xDC7C0F05, sceSulphaNgsTrace);
	REG_FUNC(0x5C71FE09, sceNgsModuleGetNumPresets);
	REG_FUNC(0xC58298A7, sceNgsModuleGetPreset);
});
