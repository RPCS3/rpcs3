#pragma once

struct SceAudioencInitStreamParam
{
	le_t<u32> size;
	le_t<u32> totalStreams;
};

struct SceAudioencInfoCelp
{
	le_t<u32> size;
	le_t<u32> excitationMode;
	le_t<u32> samplingRate;
	le_t<u32> bitRate;
};

struct SceAudioencOptInfoCelp
{
	le_t<u32> size;
	u8 header[32];
	le_t<u32> headerSize;
	le_t<u32> encoderVersion;
};


union SceAudioencInitParam
{
	le_t<u32> size;
	SceAudioencInitStreamParam celp;
};

union SceAudioencInfo
{
	le_t<u32> size;
	SceAudioencInfoCelp celp;
};

union SceAudioencOptInfo
{
	le_t<u32> size;
	SceAudioencOptInfoCelp celp;
};

struct SceAudioencCtrl
{
	le_t<u32> size;
	le_t<s32> handle;
	vm::lptr<u8> pInputPcm;
	le_t<u32> inputPcmSize;
	le_t<u32> maxPcmSize;
	vm::lptr<void> pOutputEs;
	le_t<u32> outputEsSize;
	le_t<u32> maxEsSize;
	le_t<u32> wordLength;
	vm::lptr<SceAudioencInfo> pInfo;
	vm::lptr<SceAudioencOptInfo> pOptInfo;
};

extern psv_log_base sceAudioenc;
