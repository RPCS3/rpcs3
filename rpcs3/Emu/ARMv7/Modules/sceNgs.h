#pragma once

struct SceNgsVoiceDefinition;

using SceNgsHVoice = vm::ptr<void>;
using SceNgsHPatch = vm::ptr<void>;
using SceNgsHSynSystem = vm::ptr<void>;
using SceNgsHRack = vm::ptr<void>;

struct SceNgsModuleParamHeader
{
	le_t<s32> moduleId;
	le_t<s32> chan;
};

struct SceNgsParamsDescriptor
{
	le_t<u32> id;
	le_t<u32> size;
};

struct SceNgsBufferInfo
{
	vm::lptr<void> data;
	le_t<u32> size;
};

struct SceNgsVoicePreset
{
	le_t<s32> nNameOffset;
	le_t<u32> uNameLength;
	le_t<s32> nPresetDataOffset;
	le_t<u32> uSizePresetData;
	le_t<s32> nBypassFlagsOffset;
	le_t<u32> uNumBypassFlags;
};

struct SceNgsSystemInitParams
{
	le_t<s32> nMaxRacks;
	le_t<s32> nMaxVoices;
	le_t<s32> nGranularity;
	le_t<s32> nSampleRate;
	le_t<s32> nMaxModules;
};

struct SceNgsRackDescription
{
	vm::lcptr<SceNgsVoiceDefinition> pVoiceDefn;
	le_t<s32> nVoices;
	le_t<s32> nChannelsPerVoice;
	le_t<s32> nMaxPatchesPerInput;
	le_t<s32> nPatchesPerOutput;
	vm::lptr<void> pUserReleaseData;
};

struct SceNgsPatchSetupInfo
{
	SceNgsHVoice hVoiceSource;
	le_t<s32> nSourceOutputIndex;
	le_t<s32> nSourceOutputSubIndex;
	SceNgsHVoice hVoiceDestination;
	le_t<s32> nTargetInputIndex;
};

struct SceNgsVolumeMatrix
{
	le_t<float> m[2][2];
};

struct SceNgsPatchRouteInfo
{
	le_t<s32> nOutputChannels;
	le_t<s32> nInputChannels;
	SceNgsVolumeMatrix vols;
};

struct SceNgsVoiceInfo
{
	le_t<u32> uVoiceState;
	le_t<u32> uNumModules;
	le_t<u32> uNumInputs;
	le_t<u32> uNumOutputs;
	le_t<u32> uNumPatchesPerOutput;
};

struct SceNgsCallbackInfo
{
	SceNgsHVoice hVoiceHandle;
	SceNgsHRack hRackHandle;
	le_t<u32> uModuleID;
	le_t<s32> nCallbackData;
	le_t<s32> nCallbackData2;
	vm::lptr<void> pCallbackPtr;
	vm::lptr<void> pUserData;
};

using SceNgsCallbackFunc = func_def<void(vm::cptr<SceNgsCallbackInfo> pCallbackInfo)>;

struct SceSulphaNgsConfig
{
	le_t<u32> maxNamedObjects;
	le_t<u32> maxTraceBufferBytes;
};

extern psv_log_base sceNgs;
