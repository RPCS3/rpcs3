#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "Emu/Audio/cellAudio.h"

void cellAudio_init();
void cellAudio_unload();
Module cellAudio(0x0011, cellAudio_init, nullptr, cellAudio_unload);

enum
{
	//libaudio Error Codes
	CELL_AUDIO_ERROR_ALREADY_INIT				= 0x80310701,
	CELL_AUDIO_ERROR_AUDIOSYSTEM				= 0x80310702,
	CELL_AUDIO_ERROR_NOT_INIT					= 0x80310703,
	CELL_AUDIO_ERROR_PARAM						= 0x80310704,
	CELL_AUDIO_ERROR_PORT_FULL					= 0x80310705,
	CELL_AUDIO_ERROR_PORT_ALREADY_RUN			= 0x80310706,
	CELL_AUDIO_ERROR_PORT_NOT_OPEN				= 0x80310707,
	CELL_AUDIO_ERROR_PORT_NOT_RUN				= 0x80310708,
	CELL_AUDIO_ERROR_TRANS_EVENT				= 0x80310709,
	CELL_AUDIO_ERROR_PORT_OPEN					= 0x8031070a,
	CELL_AUDIO_ERROR_SHAREDMEMORY				= 0x8031070b,
	CELL_AUDIO_ERROR_MUTEX						= 0x8031070c,
	CELL_AUDIO_ERROR_EVENT_QUEUE				= 0x8031070d,
	CELL_AUDIO_ERROR_AUDIOSYSTEM_NOT_FOUND		= 0x8031070e,
	CELL_AUDIO_ERROR_TAG_NOT_FOUND				= 0x8031070f,

	//libmixer Error Codes
	CELL_LIBMIXER_ERROR_NOT_INITIALIZED			= 0x80310002,
	CELL_LIBMIXER_ERROR_INVALID_PARAMATER		= 0x80310003,
	CELL_LIBMIXER_ERROR_NO_MEMORY				= 0x80310005,
	CELL_LIBMIXER_ERROR_ALREADY_EXIST			= 0x80310006,
	CELL_LIBMIXER_ERROR_FULL					= 0x80310007,
	CELL_LIBMIXER_ERROR_NOT_EXIST				= 0x80310008,
	CELL_LIBMIXER_ERROR_TYPE_MISMATCH			= 0x80310009,
	CELL_LIBMIXER_ERROR_NOT_FOUND				= 0x8031000a,

	//libsnd3 Error Codes
	CELL_SND3_ERROR_PARAM						= 0x80310301,
	CELL_SND3_ERROR_CREATE_MUTEX				= 0x80310302,
	CELL_SND3_ERROR_SYNTH						= 0x80310303,
	CELL_SND3_ERROR_ALREADY						= 0x80310304,
	CELL_SND3_ERROR_NOTINIT						= 0x80310305,
	CELL_SND3_ERROR_SMFFULL						= 0x80310306,
	CELL_SND3_ERROR_HD3ID						= 0x80310307,
	CELL_SND3_ERROR_SMF							= 0x80310308,
	CELL_SND3_ERROR_SMFCTX						= 0x80310309,
	CELL_SND3_ERROR_FORMAT						= 0x8031030a,
	CELL_SND3_ERROR_SMFID						= 0x8031030b,
	CELL_SND3_ERROR_SOUNDDATAFULL				= 0x8031030c,
	CELL_SND3_ERROR_VOICENUM					= 0x8031030d,
	CELL_SND3_ERROR_RESERVEDVOICE				= 0x8031030e,
	CELL_SND3_ERROR_REQUESTQUEFULL				= 0x8031030f,
	CELL_SND3_ERROR_OUTPUTMODE					= 0x80310310,

	//libsynt2 Error Codes
	CELL_SOUND_SYNTH2_ERROR_FATAL				= 0x80310201,
	CELL_SOUND_SYNTH2_ERROR_INVALID_PARAMETER	= 0x80310202,
	CELL_SOUND_SYNTH2_ERROR_ALREADY_INITIALIZED	= 0x80310203,
};


//libaudio datatypes
struct CellAudioPortParam
{ 
	be_t<u64> nChannel;
	be_t<u64> nBlock;
	be_t<u64> attr;
	be_t<float> level;
}; 

struct CellAudioPortConfig
{ 
	be_t<u32> readIndexAddr;
	be_t<u32> status;
	be_t<u64> nChannel;
	be_t<u64> nBlock;
	be_t<u32> portSize;
	be_t<u32> portAddr;
};

struct CellAudioConfig  //custom structure
{
	bool m_is_audio_initialized;
	bool m_is_audio_port_open;
	bool m_is_audio_port_started;
};

CellAudioPortParam* m_param = new CellAudioPortParam;

CellAudioConfig* m_config = new CellAudioConfig;

//libmixer datatypes
typedef void * CellAANHandle;

struct CellSSPlayerConfig
{
	be_t<u32> channels;
	be_t<u32> outputMode;
}; 

struct CellSSPlayerWaveParam 
{ 
	void *addr;
	be_t<s32> format;
	be_t<u32> samples;
	be_t<u32> loopStartOffset;
	be_t<u32> startOffset;
};

struct CellSSPlayerCommonParam 
{ 
	be_t<u32> loopMode;
	be_t<u32> attackMode;
};

struct CellSurMixerPosition 
{ 
	be_t<float> x;
	be_t<float> y;
	be_t<float> z;
};

struct CellSSPlayerRuntimeInfo 
{ 
	be_t<float> level;
	be_t<float> speed;
	CellSurMixerPosition position;
};

struct CellSurMixerConfig 
{ 
	be_t<s32> priority;
	be_t<u32> chStrips1;
	be_t<u32> chStrips2;
	be_t<u32> chStrips6;
	be_t<u32> chStrips8;
};

struct CellSurMixerChStripParam 
{ 
	be_t<u32> param;
	void *attribute;
	be_t<s32> dBSwitch;
	be_t<float> floatVal;
	be_t<s32> intVal;
};

CellSSPlayerWaveParam current_SSPlayerWaveParam;

//libsnd3 datatypes
struct CellSnd3DataCtx
{ 
	s8 system;  //[CELL_SND3_DATA_CTX_SIZE], unknown identifier
}; 

struct CellSnd3SmfCtx
{ 
	s8 system;  //[CELL_SND3_SMF_CTX_SIZE],  unknown identifier
};

struct CellSnd3KeyOnParam
{ 
	u8 vel;
	u8 pan;
	u8 panEx;
	be_t<s32> addPitch;
};

struct CellSnd3VoiceBitCtx
{ 
	be_t<u32> core;  //[CELL_SND3_MAX_CORE],  unknown identifier
};

struct CellSnd3RequestQueueCtx
{ 
	void *frontQueue;
	be_t<u32> frontQueueSize;
	void *rearQueue;
	be_t<u32> rearQueueSize;
};

//libsynt2 datatypes
struct CellSoundSynth2EffectAttr
{ 
	be_t<u16> core;
	be_t<u16> mode;
	be_t<s16> depth_L;
	be_t<s16> depth_R;
	be_t<u16> delay;
	be_t<u16> feedback;
};

// libaudio Functions

int cellAudioInit()
{
	cellAudio.Warning("cellAudioInit()");
	if(m_config->m_is_audio_initialized == true) return CELL_AUDIO_ERROR_ALREADY_INIT;
	m_config->m_is_audio_initialized = true;
	return CELL_OK;
}

int cellAudioQuit()
{
	cellAudio.Warning("cellAudioQuit()");
	if (m_config->m_is_audio_initialized == false) return CELL_AUDIO_ERROR_NOT_INIT;
	m_config->m_is_audio_initialized = false;

	return CELL_OK;
}

int cellAudioPortOpen(mem_ptr_t<CellAudioPortParam> audioParam, mem32_t portNum)
{
	cellAudio.Warning("cellAudioPortOpen(audioParam_addr=0x%x,portNum_addr=0x%x)",audioParam.GetAddr(),portNum.GetAddr());

	if(!audioParam.IsGood() || !portNum.IsGood()) return CELL_AUDIO_ERROR_PORT_OPEN;
	m_config->m_is_audio_port_open = true;
	
	m_param->nChannel = audioParam->nChannel;
	m_param->nBlock = audioParam->nBlock;
	m_param->attr = audioParam->attr;
	m_param->level = audioParam->level;

	//TODO: implementation of ring buffer
	return CELL_OK;
}

int cellAudioGetPortConfig(u32 portNum, mem_ptr_t<CellAudioPortConfig> portConfig)
{
	cellAudio.Warning("cellAudioGetPortConfig(portNum=0x%x,portConfig_addr=0x%x)",portNum,portConfig.GetAddr());

	if(!portConfig.IsGood()) 
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	//if(portNum > 7) return CELL_AUDIO_ERROR_PORT_FULL;

	if(m_config->m_is_audio_port_open == false)
	{
		portConfig->status = CELL_AUDIO_STATUS_CLOSE;
		return CELL_OK;
	}

	if(m_config->m_is_audio_port_started == true)
	{
		portConfig->status = CELL_AUDIO_STATUS_RUN;
	}
	else
	{
		portConfig->status = CELL_AUDIO_STATUS_READY;
		portConfig->nChannel = m_param->nChannel;
		portConfig->nBlock = m_param->nBlock;
		portConfig->portSize = sizeof(float)*256*(m_param->nChannel)*(m_param->nBlock);
		portConfig->portAddr = Memory.Alloc(portConfig->portSize, 4);	// 0x20020000			WARNING: Memory leak.
		portConfig->readIndexAddr = Memory.Alloc(m_param->nBlock, 4);	// 0x20010010 on ps3	WARNING: Memory leak.

		// portAddr - readIndexAddr ==  0xFFF0 on ps3
		Memory.Write64(portConfig->readIndexAddr, 1);
	}

	return CELL_OK;
}

int cellAudioPortStart(u32 portNum)
{
	cellAudio.Warning("cellAudioPortStart(portNum=0x%x)",portNum);

	if (m_config->m_is_audio_port_open == true)
		return CELL_AUDIO_ERROR_PORT_OPEN;
	
	m_config->m_is_audio_port_started = true;
	return CELL_OK;
}

int cellAudioPortClose(u32 portNum)
{
	cellAudio.Warning("cellAudioPortClose(portNum=0x%x)",portNum);
	
	if (m_config->m_is_audio_port_open == false)
		return CELL_AUDIO_ERROR_PORT_NOT_OPEN;
	
	m_config->m_is_audio_port_open = false;
	return CELL_OK;
}

int cellAudioPortStop(u32 portNum)
{
	cellAudio.Warning("cellAudioPortStop(portNum=0x%x)",portNum);
	
	if (m_config->m_is_audio_port_started == false)
		return CELL_AUDIO_ERROR_PORT_NOT_OPEN;
	
	m_config->m_is_audio_port_started = false;
	return CELL_OK;
}

int cellAudioGetPortTimestamp(u32 portNum, u64 tag, mem64_t stamp)
{
	cellAudio.Warning("cellAudioGetPortTimestamp(portNum=0x%x,tag=0x%x,stamp=0x%x)",portNum,tag,stamp.GetAddr());
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioGetPortBlockTag(u32 portNum, u64 blockNo, mem64_t tag)
{
	cellAudio.Warning("cellAudioGetPortBlockTag(portNum=0x%x,blockNo=0x%x,tag=0x%x)",portNum,blockNo,tag.GetAddr());
	UNIMPLEMENTED_FUNC (cellAudio);
	return CELL_OK;
}

int cellAudioSetPortLevel(u32 portNum, float level)
{
	cellAudio.Warning("cellAudioSetPortLevel(portNum=0x%x,level=0x%x)",portNum,level);
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

// Utility Functions  
int cellAudioCreateNotifyEventQueue(mem32_t id, mem64_t key)
{
	cellAudio.Warning("cellAudioCreateNotifyEventQueue(id=0x%x,key=0x%x)",id.GetAddr(),key.GetAddr());
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioCreateNotifyEventQueueEx(mem32_t id, mem64_t key, u32 iFlags)
{
	cellAudio.Warning("cellAudioCreateNotifyEventQueueEx(id=0x%x,key=0x%x,iFlags=0x%x)",id.GetAddr(),key.GetAddr(),iFlags);
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioSetNotifyEventQueue(u64 key)
{
	cellAudio.Warning("cellAudioSetNotifyEventQueue(key=0x%x)",key);
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioSetNotifyEventQueueEx(u64 key, u32 iFlags)
{
	cellAudio.Warning("cellAudioSetNotifyEventQueueEx(key=0x%x,iFlags=0x%x)",key,iFlags);
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioRemoveNotifyEventQueue(u64 key)
{
	cellAudio.Warning("cellAudioRemoveNotifyEventQueue(key=0x%x)",key);
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioRemoveNotifyEventQueueEx(u64 key, u32 iFlags)
{
	cellAudio.Warning("cellAudioRemoveNotifyEventQueueEx(key=0x%x,iFlags=0x%x)",key,iFlags);
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioAddData(u32 portNum, mem32_t src, uint samples, float volume)
{
	cellAudio.Warning("cellAudioAddData(portNum=0x%x,src=0x%x,samples=0x%x,volume=0x%x)",portNum,src.GetAddr(),samples,volume);
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioAdd2chData(u32 portNum, mem32_t src, uint samples, float volume) 
{
	cellAudio.Warning("cellAudioAdd2chData(portNum=0x%x,src=0x%x,samples=0x%x,volume=0x%x)",portNum,src.GetAddr(),samples,volume);
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioAdd6chData(u32 portNum, mem32_t src, float volume)
{
	cellAudio.Warning("cellAudioAdd6chData(portNum=0x%x,src=0x%x,volume=0x%x)",portNum,src.GetAddr(),volume);
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioMiscSetAccessoryVolume(u32 devNum, float volume)
{
	cellAudio.Warning("cellAudioMiscSetAccessoryVolume(devNum=0x%x,volume=0x%x)",devNum,volume);
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioSendAck(u64 data3)
{
	cellAudio.Warning("cellAudioSendAck(data3=0x%x)",data3);
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioSetPersonalDevice(int iPersonalStream, int iDevice)
{
	cellAudio.Warning("cellAudioSetPersonalDevice(iPersonalStream=0x%x,iDevice=0x%x)",iPersonalStream,iDevice);
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioUnsetPersonalDevice(int iPersonalStream)
{
	cellAudio.Warning("cellAudioUnsetPersonalDevice(iPersonalStream=0x%x)",iPersonalStream);
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}


//Callback Functions 
typedef int (*CellSurMixerNotifyCallbackFunction)(void *arg, u32 counter, u32 samples); //Currently unused.

// libmixer Functions, NOT active in this moment
int cellAANConnect(CellAANHandle receive, u32 receivePortNo, CellAANHandle source, u32 sourcePortNo)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellAANDisconnect(CellAANHandle receive, u32 receivePortNo, CellAANHandle source, u32 sourcePortNo)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellAANAddData(CellAANHandle handle, u32 port, u32 offset, float *addr, u32 samples)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0; 
}
 
int cellSSPlayerCreate(CellAANHandle *handle, CellSSPlayerConfig *config)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSSPlayerRemove(CellAANHandle handle)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSSPlayerSetWave() //CellAANHandle handle, CellSSPlayerWaveParam *waveInfo, CellSSPlayerCommonParam *commonInfo  //mem_class_t waveInfo
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSSPlayerPlay() //CellAANHandle handle, CellSSPlayerRuntimeInfo *info
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSSPlayerStop() //CellAANHandle handle, u32 mode
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSSPlayerSetParam() //CellAANHandle handle, CellSSPlayerRuntimeInfo *info
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}
 
s32 cellSSPlayerGetState() //CellAANHandle handle
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSurMixerCreate() //const CellSurMixerConfig *config
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSurMixerGetAANHandle() //CellAANHandle *handle
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSurMixerChStripGetAANPortNo() //u32 *port, u32 type, u32 index
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSurMixerSetNotifyCallback() //CellSurMixerNotifyCallbackFunction callback, void *arg
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSurMixerRemoveNotifyCallback() //CellSurMixerNotifyCallbackFunction callback
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSurMixerStart()
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSurMixerSurBusAddData() //u32 busNo, u32 offset, float *addr, u32 samples
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSurMixerSetParameter() //u32 param, float value
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSurMixerChStripSetParameter() //u32 type, u32 index, CellSurMixerChStripParam *param
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSurMixerPause() //u32 switch
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSurMixerGetCurrentBlockTag() //u64 *tag
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSurMixerGetTimestamp() //u64 tag, u64 *stamp
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

void cellSurMixerBeep(); //void *arg

float cellSurMixerUtilGetLevelFromDB() //float dB
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO;
}

float cellSurMixerUtilGetLevelFromDBIndex() //int index
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO;
}

float cellSurMixerUtilNoteToRatio() //unsigned char refNote, unsigned char note
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

int cellSurMixerFinalize(); //Currently unused. Returns 0 (in the current release).

//*libsnd3 Functions, NOT active in this moment
s32 cellSnd3Init() //u32 maxVoice, u32 samples, CellSnd3RequestQueueCtx *queue
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3Exit()
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3SetOutputMode() //u32 mode
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3Synthesis() //float *pOutL, float *pOutR
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3SynthesisEx() //float *pOutL, float *pOutR, float *pOutRL, float *pOutRR
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3VoiceSetReserveMode() //u32 voiceNum, u32 reserveMode
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3BindSoundData() //CellSnd3DataCtx *snd3Ctx, void *hd3, u32 synthMemOffset
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3UnbindSoundData(u32 hd3ID)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3NoteOnByTone() //u32 hd3ID, u32 toneIndex, u32 note, u32 keyOnID, CellSnd3KeyOnParam *keyOnParam
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

s32 cellSnd3KeyOnByTone() //u32 hd3ID, u32 toneIndex,  u32 pitch,u32 keyOnID,CellSnd3KeyOnParam *keyOnParam
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

s32 cellSnd3VoiceNoteOnByTone() //u32 hd3ID, u32 voiceNum, u32 toneIndex, u32 note, u32 keyOnID, CellSnd3KeyOnParam *keyOnParam
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

s32 cellSnd3VoiceKeyOnByTone() //u32 hd3ID, u32 voiceNum, u32 toneIndex, u32 pitch, u32 keyOnID, CellSnd3KeyOnParam *keyOnParam
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

s32 cellSnd3VoiceSetSustainHold(u32 voiceNum, u32 sustainHold)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3VoiceKeyOff(u32 voiceNum)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3VoiceSetPitch(u32 voiceNum, s32 addPitch)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3VoiceSetVelocity(u32 voiceNum, u32 velocity)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3VoiceSetPanpot(u32 voiceNum, u32 panpot)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3VoiceSetPanpotEx(u32 voiceNum, u32 panpotEx)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3VoiceSetPitchBend(u32 voiceNum, u32 bendValue)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3VoiceAllKeyOff()
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3VoiceGetEnvelope(u32 voiceNum)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3VoiceGetStatus()  //u32 voiceNum
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

u32 cellSnd3KeyOffByID(u32 keyOnID)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3GetVoice() //u32 midiChannel, u32 keyOnID, CellSnd3VoiceBitCtx *voiceBit
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3GetVoiceByID() //u32 keyOnID, CellSnd3VoiceBitCtx *voiceBit
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3NoteOn() //u32 hd3ID, u32 midiChannel, u32 midiProgram, u32 midiNote, u32 sustain,CellSnd3KeyOnParam *keyOnParam, u32 keyOnID
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3NoteOff(u32 midiChannel, u32 midiNote, u32 keyOnID)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3SetSustainHold(u32 midiChannel, u32 sustainHold, u32 ID)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3SetEffectType(u16 effectType, s16 returnVol, u16 delay, u16 feedback)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

u16 cellSnd3Note2Pitch() //u16 center_note, u16 center_fine, u16 note, s16 fine
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

u16 cellSnd3Pitch2Note() //u16 center_note, u16 center_fine, u16 pitch
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

s32 cellSnd3SMFBind() //CellSnd3SmfCtx *smfCtx, void *smf, u32 hd3ID
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

s32 cellSnd3SMFUnbind() //u32 smfID
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

s32 cellSnd3SMFPlay(u32 smfID, u32 playVelocity, u32 playPan, u32 playCount)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3SMFPlayEx(u32 smfID, u32 playVelocity, u32 playPan, u32 playPanEx, u32 playCount)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3SMFPause(u32 smfID)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3SMFResume(u32 smfID)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3SMFStop(u32 smfID)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3SMFAddTempo(u32 smfID, s32 addTempo)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3SMFGetTempo() //u32 smfID
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

s32 cellSnd3SMFSetPlayVelocity(u32 smfID, u32 playVelocity)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3SMFGetPlayVelocity()  //u32 smfID
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

s32 cellSnd3SMFSetPlayPanpot(u32 smfID, u32 playPanpot)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3SMFSetPlayPanpotEx(u32 smfID, u32 playPanpotEx)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3SMFGetPlayPanpot() //u32 smfID
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

s32 cellSnd3SMFGetPlayPanpotEx() //u32 smfID
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

s32 cellSnd3SMFGetPlayStatus()  //u32 smfID
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

s32 cellSnd3SMFSetPlayChannel(u32 smfID, u32 playChannelBit)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3SMFGetPlayChannel() //u32 smfID, u32 *playChannelBit
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3SMFGetKeyOnID() //u32 smfID, u32 midiChannel, u32 *keyOnID
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

                                           //*libsynth2 Functions, NON active in this moment*//

s32 cellSoundSynth2Config(s16 param, s32 value)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

s32 cellSoundSynth2Init( s16 flag)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

s32 cellSoundSynth2Exit()
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

void cellSoundSynth2SetParam(u16 register, u16 value)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	//TODO
}

u16 cellSoundSynth2GetParam() //u16 register
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

void cellSoundSynth2SetSwitch(u16 register, u32 value)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	//TODO
}

u32 cellSoundSynth2GetSwitch() //u16 register
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

u32 cellSoundSynth2SetAddr(u16 register, u32 value)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

u32 cellSoundSynth2GetAddr() //u16 register
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

s32 cellSoundSynth2SetEffectAttr() //s16 bus, CellSoundSynth2EffectAttr *attr
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

s32 cellSoundSynth2SetEffectMode() //s16 bus, CellSoundSynth2EffectAttr *attr
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

void cellSoundSynth2SetCoreAttr(u16 entry, u16 value) 
{
	UNIMPLEMENTED_FUNC(cellAudio);
	//TODO
}

s32 cellSoundSynth2Generate() //u16 samples, float *left_buffer, float *right_buffer, float *left_rear, float *right_rear
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

s32 cellSoundSynth2VoiceTrans() //s16 channel, u16 mode, u8 *m_addr, u32 s_addr, u32 size
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

u16 cellSoundSynth2Note2Pitch() //u16 center_note, u16 center_fine, u16 note, s16 fine
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

u16 cellSoundSynth2Pitch2Note() //u16 center_note, u16 center_fine, u16 pitch
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}


void cellAudio_init()
{
	cellAudio.AddFunc(0x0b168f92, cellAudioInit);
	cellAudio.AddFunc(0x4129fe2d, cellAudioPortClose);
	cellAudio.AddFunc(0x5b1e2c73, cellAudioPortStop);
	cellAudio.AddFunc(0x74a66af0, cellAudioGetPortConfig);
	cellAudio.AddFunc(0x89be28f2, cellAudioPortStart);
	cellAudio.AddFunc(0xca5ac370, cellAudioQuit);
	cellAudio.AddFunc(0xcd7bc431, cellAudioPortOpen);
	cellAudio.AddFunc(0x56dfe179, cellAudioSetPortLevel);
	cellAudio.AddFunc(0x04af134e, cellAudioCreateNotifyEventQueue);
	cellAudio.AddFunc(0x31211f6b, cellAudioMiscSetAccessoryVolume);
	cellAudio.AddFunc(0x377e0cd9, cellAudioSetNotifyEventQueue);
	cellAudio.AddFunc(0x4109d08c, cellAudioGetPortTimestamp);
	cellAudio.AddFunc(0x9e4b1db8, cellAudioAdd2chData);
	cellAudio.AddFunc(0xdab029aa, cellAudioAddData);
	cellAudio.AddFunc(0xe4046afe, cellAudioGetPortBlockTag);
	cellAudio.AddFunc(0xff3626fd, cellAudioRemoveNotifyEventQueue);

	//TODO: Find addresses for libmixer, libsnd3 and libsynth2 functions
}

void cellAudio_unload()
{
	m_config->m_is_audio_initialized = false;
	m_config->m_is_audio_port_open = false;
	m_config->m_is_audio_port_started = false;
}