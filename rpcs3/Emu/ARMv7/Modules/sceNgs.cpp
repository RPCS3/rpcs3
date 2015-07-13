#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceNgs;

struct SceNgsVoiceDefinition;

typedef u32 SceNgsModuleID;
typedef u32 SceNgsParamsID;
typedef vm::psv::ptr<void> SceNgsHVoice;
typedef vm::psv::ptr<void> SceNgsHPatch;
typedef vm::psv::ptr<void> SceNgsHSynSystem;
typedef vm::psv::ptr<void> SceNgsHRack;

struct SceNgsModuleParamHeader
{
	s32 moduleId;
	s32 chan;
};

struct SceNgsParamsDescriptor
{
	SceNgsParamsID id;
	u32 size;
};

struct SceNgsBufferInfo
{
	vm::psv::ptr<void> data;
	u32 size;
};

struct SceNgsVoicePreset
{
	s32 nNameOffset;
	u32 uNameLength;
	s32 nPresetDataOffset;
	u32 uSizePresetData;
	s32 nBypassFlagsOffset;
	u32 uNumBypassFlags;
};

struct SceNgsSystemInitParams
{
	s32 nMaxRacks;
	s32 nMaxVoices;
	s32 nGranularity;
	s32 nSampleRate;
	s32 nMaxModules;
};

struct SceNgsRackDescription
{
	vm::psv::ptr<const SceNgsVoiceDefinition> pVoiceDefn;
	s32 nVoices;
	s32 nChannelsPerVoice;
	s32 nMaxPatchesPerInput;
	s32 nPatchesPerOutput;
	vm::psv::ptr<void> pUserReleaseData;
};

struct SceNgsPatchSetupInfo
{
	SceNgsHVoice hVoiceSource;
	s32 nSourceOutputIndex;
	s32 nSourceOutputSubIndex;
	SceNgsHVoice hVoiceDestination;
	s32 nTargetInputIndex;
};

struct SceNgsVolumeMatrix
{
	float m[2][2];
};

struct SceNgsPatchRouteInfo
{
	s32 nOutputChannels;
	s32 nInputChannels;
	SceNgsVolumeMatrix vols;
};

struct SceNgsVoiceInfo
{
	u32 uVoiceState;
	u32 uNumModules;
	u32 uNumInputs;
	u32 uNumOutputs;
	u32 uNumPatchesPerOutput;
};

struct SceNgsCallbackInfo
{
	SceNgsHVoice hVoiceHandle;
	SceNgsHRack hRackHandle;
	SceNgsModuleID uModuleID;
	s32 nCallbackData;
	s32 nCallbackData2;
	vm::psv::ptr<void> pCallbackPtr;
	vm::psv::ptr<void> pUserData;
};

typedef vm::psv::ptr<void(vm::psv::ptr<const SceNgsCallbackInfo> pCallbackInfo)> SceNgsCallbackFunc;

typedef SceNgsCallbackFunc SceNgsRackReleaseCallbackFunc;
typedef SceNgsCallbackFunc SceNgsModuleCallbackFunc;
typedef SceNgsCallbackFunc SceNgsParamsErrorCallbackFunc;

struct SceSulphaNgsConfig
{
	u32 maxNamedObjects;
	u32 maxTraceBufferBytes;
};

s32 sceNgsSystemGetRequiredMemorySize(vm::psv::ptr<const SceNgsSystemInitParams> pSynthParams, vm::psv::ptr<u32> pnSize)
{
	throw __FUNCTION__;
}

s32 sceNgsSystemInit(vm::psv::ptr<void> pSynthSysMemory, const u32 uMemSize, vm::psv::ptr<const SceNgsSystemInitParams> pSynthParams, vm::psv::ptr<SceNgsHSynSystem> pSystemHandle)
{
	throw __FUNCTION__;
}

s32 sceNgsSystemUpdate(SceNgsHSynSystem hSystemHandle)
{
	throw __FUNCTION__;
}

s32 sceNgsSystemRelease(SceNgsHSynSystem hSystemHandle)
{
	throw __FUNCTION__;
}

s32 sceNgsSystemLock(SceNgsHSynSystem hSystemHandle)
{
	throw __FUNCTION__;
}

s32 sceNgsSystemUnlock(SceNgsHSynSystem hSystemHandle)
{
	throw __FUNCTION__;
}

s32 sceNgsSystemSetParamErrorCallback(SceNgsHSynSystem hSystemHandle, const SceNgsParamsErrorCallbackFunc callbackFuncPtr)
{
	throw __FUNCTION__;
}

s32 sceNgsSystemSetFlags(SceNgsHSynSystem hSystemHandle, const u32 uSystemFlags)
{
	throw __FUNCTION__;
}

s32 sceNgsRackGetRequiredMemorySize(SceNgsHSynSystem hSystemHandle, vm::psv::ptr<const SceNgsRackDescription> pRackDesc, vm::psv::ptr<u32> pnSize)
{
	throw __FUNCTION__;
}

s32 sceNgsRackInit(SceNgsHSynSystem hSystemHandle, vm::psv::ptr<SceNgsBufferInfo> pRackBuffer, vm::psv::ptr<const SceNgsRackDescription> pRackDesc, vm::psv::ptr<SceNgsHRack> pRackHandle)
{
	throw __FUNCTION__;
}

s32 sceNgsRackGetVoiceHandle(SceNgsHRack hRackHandle, const u32 uIndex, vm::psv::ptr<SceNgsHVoice> pVoiceHandle)
{
	throw __FUNCTION__;
}

s32 sceNgsRackRelease(SceNgsHRack hRackHandle, const SceNgsRackReleaseCallbackFunc callbackFuncPtr)
{
	throw __FUNCTION__;
}

s32 sceNgsRackSetParamErrorCallback(SceNgsHRack hRackHandle, const SceNgsParamsErrorCallbackFunc callbackFuncPtr)
{
	throw __FUNCTION__;
}

s32 sceNgsVoiceInit(SceNgsHVoice hVoiceHandle, vm::psv::ptr<const SceNgsVoicePreset> pPreset, const u32 uInitFlags)
{
	throw __FUNCTION__;
}

s32 sceNgsVoicePlay(SceNgsHVoice hVoiceHandle)
{
	throw __FUNCTION__;
}

s32 sceNgsVoiceKeyOff(SceNgsHVoice hVoiceHandle)
{
	throw __FUNCTION__;
}

s32 sceNgsVoiceKill(SceNgsHVoice hVoiceHandle)
{
	throw __FUNCTION__;
}

s32 sceNgsVoicePause(SceNgsHVoice hVoiceHandle)
{
	throw __FUNCTION__;
}

s32 sceNgsVoiceResume(SceNgsHVoice hVoiceHandle)
{
	throw __FUNCTION__;
}

s32 sceNgsVoiceSetPreset(SceNgsHVoice hVoiceHandle, vm::psv::ptr<const SceNgsVoicePreset> pVoicePreset)
{
	throw __FUNCTION__;
}

s32 sceNgsVoiceLockParams(SceNgsHVoice hVoiceHandle, const u32 uModule, const SceNgsParamsID uParamsInterfaceId, vm::psv::ptr<SceNgsBufferInfo> pParamsBuffer)
{
	throw __FUNCTION__;
}

s32 sceNgsVoiceUnlockParams(SceNgsHVoice hVoiceHandle, const u32 uModule)
{
	throw __FUNCTION__;
}

s32 sceNgsVoiceSetParamsBlock(SceNgsHVoice hVoiceHandle, vm::psv::ptr<const SceNgsModuleParamHeader> pParamData, const u32 uSize, vm::psv::ptr<s32> pnErrorCount)
{
	throw __FUNCTION__;
}

s32 sceNgsVoiceBypassModule(SceNgsHVoice hVoiceHandle, const u32 uModule, const u32 uBypassFlag)
{
	throw __FUNCTION__;
}

s32 sceNgsVoiceSetModuleCallback(SceNgsHVoice hVoiceHandle, const u32 uModule, const SceNgsModuleCallbackFunc callbackFuncPtr, vm::psv::ptr<void> pUserData)
{
	throw __FUNCTION__;
}

s32 sceNgsVoiceSetFinishedCallback(SceNgsHVoice hVoiceHandle, const SceNgsCallbackFunc callbackFuncPtr, vm::psv::ptr<void> pUserData)
{
	throw __FUNCTION__;
}

s32 sceNgsVoiceGetStateData(SceNgsHVoice hVoiceHandle, const u32 uModule, vm::psv::ptr<void> pMem, const u32 uMemSize)
{
	throw __FUNCTION__;
}

s32 sceNgsVoiceGetInfo(SceNgsHVoice hVoiceHandle, vm::psv::ptr<SceNgsVoiceInfo> pInfo)
{
	throw __FUNCTION__;
}

s32 sceNgsVoiceGetModuleType(SceNgsHVoice hVoiceHandle, const u32 uModule, vm::psv::ptr<SceNgsModuleID> pModuleType)
{
	throw __FUNCTION__;
}

s32 sceNgsVoiceGetModuleBypass(SceNgsHVoice hVoiceHandle, const u32 uModule, vm::psv::ptr<u32> puBypassFlag)
{
	throw __FUNCTION__;
}

s32 sceNgsVoiceGetParamsOutOfRange(SceNgsHVoice hVoiceHandle, const u32 uModule, vm::psv::ptr<char> pszMessageBuffer)
{
	throw __FUNCTION__;
}

s32 sceNgsPatchCreateRouting(vm::psv::ptr<const SceNgsPatchSetupInfo> pPatchInfo, vm::psv::ptr<SceNgsHPatch> pPatchHandle)
{
	throw __FUNCTION__;
}

s32 sceNgsPatchGetInfo(SceNgsHPatch hPatchHandle, vm::psv::ptr<SceNgsPatchRouteInfo> pRouteInfo, vm::psv::ptr<SceNgsPatchSetupInfo> pSetup)
{
	throw __FUNCTION__;
}

s32 sceNgsVoiceGetOutputPatch(SceNgsHVoice hVoiceHandle, const s32 nOutputIndex, const s32 nSubIndex, vm::psv::ptr<SceNgsHPatch> pPatchHandle)
{
	throw __FUNCTION__;
}

s32 sceNgsPatchRemoveRouting(SceNgsHPatch hPatchHandle)
{
	throw __FUNCTION__;
}

//s32 sceNgsVoicePatchSetVolume(SceNgsHPatch hPatchHandle, const s32 nOutputChannel, const s32 nInputChannel, const float fVol)
//{
//	throw __FUNCTION__;
//}

s32 sceNgsVoicePatchSetVolumes(SceNgsHPatch hPatchHandle, const s32 nOutputChannel, vm::psv::ptr<const float> pVolumes, const s32 nVols)
{
	throw __FUNCTION__;
}

s32 sceNgsVoicePatchSetVolumesMatrix(SceNgsHPatch hPatchHandle, vm::psv::ptr<const SceNgsVolumeMatrix> pMatrix)
{
	throw __FUNCTION__;
}

s32 sceNgsModuleGetNumPresets(SceNgsHSynSystem hSystemHandle, const SceNgsModuleID uModuleID, vm::psv::ptr<u32> puNumPresets)
{
	throw __FUNCTION__;
}

s32 sceNgsModuleGetPreset(SceNgsHSynSystem hSystemHandle, const SceNgsModuleID uModuleID, const u32 uPresetIndex, vm::psv::ptr<SceNgsBufferInfo> pParamsBuffer)
{
	throw __FUNCTION__;
}

vm::psv::ptr<const SceNgsVoiceDefinition> sceNgsVoiceDefGetCompressorBuss()
{
	throw __FUNCTION__;
}

vm::psv::ptr<const SceNgsVoiceDefinition> sceNgsVoiceDefGetCompressorSideChainBuss()
{
	throw __FUNCTION__;
}

vm::psv::ptr<const SceNgsVoiceDefinition> sceNgsVoiceDefGetDelayBuss()
{
	throw __FUNCTION__;
}

vm::psv::ptr<const SceNgsVoiceDefinition> sceNgsVoiceDefGetDistortionBuss()
{
	throw __FUNCTION__;
}

vm::psv::ptr<const SceNgsVoiceDefinition> sceNgsVoiceDefGetEnvelopeBuss()
{
	throw __FUNCTION__;
}

vm::psv::ptr<const SceNgsVoiceDefinition> sceNgsVoiceDefGetEqBuss()
{
	throw __FUNCTION__;
}

vm::psv::ptr<const SceNgsVoiceDefinition> sceNgsVoiceDefGetMasterBuss()
{
	throw __FUNCTION__;
}

vm::psv::ptr<const SceNgsVoiceDefinition> sceNgsVoiceDefGetMixerBuss()
{
	throw __FUNCTION__;
}

vm::psv::ptr<const SceNgsVoiceDefinition> sceNgsVoiceDefGetPauserBuss()
{
	throw __FUNCTION__;
}

vm::psv::ptr<const SceNgsVoiceDefinition> sceNgsVoiceDefGetReverbBuss()
{
	throw __FUNCTION__;
}

vm::psv::ptr<const SceNgsVoiceDefinition> sceNgsVoiceDefGetSasEmuVoice()
{
	throw __FUNCTION__;
}

vm::psv::ptr<const SceNgsVoiceDefinition> sceNgsVoiceDefGetSimpleVoice()
{
	throw __FUNCTION__;
}

vm::psv::ptr<const SceNgsVoiceDefinition> sceNgsVoiceDefGetTemplate1()
{
	throw __FUNCTION__;
}

vm::psv::ptr<const SceNgsVoiceDefinition> sceNgsVoiceDefGetAtrac9Voice()
{
	throw __FUNCTION__;
}

s32 sceSulphaNgsGetDefaultConfig(vm::psv::ptr<SceSulphaNgsConfig> config)
{
	throw __FUNCTION__;
}

s32 sceSulphaNgsGetNeededMemory(vm::psv::ptr<const SceSulphaNgsConfig> config, vm::psv::ptr<u32> sizeInBytes)
{
	throw __FUNCTION__;
}

s32 sceSulphaNgsInit(vm::psv::ptr<const SceSulphaNgsConfig> config, vm::psv::ptr<void> buffer, u32 sizeInBytes)
{
	throw __FUNCTION__;
}

s32 sceSulphaNgsShutdown()
{
	throw __FUNCTION__;
}

s32 sceSulphaNgsSetSynthName(SceNgsHSynSystem synthHandle, vm::psv::ptr<const char> name)
{
	throw __FUNCTION__;
}

s32 sceSulphaNgsSetRackName(SceNgsHRack rackHandle, vm::psv::ptr<const char> name)
{
	throw __FUNCTION__;
}

s32 sceSulphaNgsSetVoiceName(SceNgsHVoice voiceHandle, vm::psv::ptr<const char> name)
{
	throw __FUNCTION__;
}

s32 sceSulphaNgsSetSampleName(vm::psv::ptr<const void> location, u32 length, vm::psv::ptr<const char> name)
{
	throw __FUNCTION__;
}

s32 sceSulphaNgsTrace(vm::psv::ptr<const char> message)
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func(nid, &sceNgs, #name, name)

psv_log_base sceNgs("SceNgs", []()
{
	sceNgs.on_load = nullptr;
	sceNgs.on_unload = nullptr;
	sceNgs.on_stop = nullptr;

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
