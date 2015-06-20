#pragma once

struct SceAudiodecInitStreamParam
{
	le_t<u32> size;
	le_t<u32> totalStreams;
};

struct SceAudiodecInitChParam
{
	le_t<u32> size;
	le_t<u32> totalCh;
};

union SceAudiodecInitParam
{
	le_t<u32> size;
	SceAudiodecInitChParam     at9;
	SceAudiodecInitStreamParam mp3;
	SceAudiodecInitStreamParam aac;
	SceAudiodecInitStreamParam celp;
};

struct SceAudiodecInfoAt9
{
	le_t<u32> size;
	u8 configData[4];
	le_t<u32> ch;
	le_t<u32> bitRate;
	le_t<u32> samplingRate;
	le_t<u32> superFrameSize;
	le_t<u32> framesInSuperFrame;
};

struct SceAudiodecInfoMp3
{
	le_t<u32> size;
	le_t<u32> ch;
	le_t<u32> version;
};

struct SceAudiodecInfoAac
{
	le_t<u32> size;
	le_t<u32> isAdts;
	le_t<u32> ch;
	le_t<u32> samplingRate;
	le_t<u32> isSbr;
};

struct SceAudiodecInfoCelp
{
	le_t<u32> size;
	le_t<u32> excitationMode;
	le_t<u32> samplingRate;
	le_t<u32> bitRate;
	le_t<u32> lostCount;
};

union SceAudiodecInfo
{
	le_t<u32> size;
	SceAudiodecInfoAt9  at9;
	SceAudiodecInfoMp3  mp3;
	SceAudiodecInfoAac  aac;
	SceAudiodecInfoCelp celp;
};

struct SceAudiodecCtrl
{
	le_t<u32> size;
	le_t<s32> handle;
	vm::lptr<u8> pEs;
	le_t<u32> inputEsSize;
	le_t<u32> maxEsSize;
	vm::lptr<void> pPcm;
	le_t<u32> outputPcmSize;
	le_t<u32> maxPcmSize;
	le_t<u32> wordLength;
	vm::lptr<SceAudiodecInfo> pInfo;
};

extern psv_log_base sceAudiodec;
