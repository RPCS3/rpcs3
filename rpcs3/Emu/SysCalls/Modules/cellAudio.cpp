#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellAudio_init();
Module cellAudio(0x0011, cellAudio_init);

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
	CELL_AUDIO_ERROR_AUDIOSYSTEM_NOT_FOUND      = 0x8031070e,
	CELL_AUDIO_ERROR_TAG_NOT_FOUND				= 0x8031070f,

	//libmixer Error Codes
	CELL_LIBMIXER_ERROR_NOT_INITIALIZED         = 0x80310002,
	CELL_LIBMIXER_ERROR_INVALID_PARAMATER       = 0x80310003,
	CELL_LIBMIXER_ERROR_NO_MEMORY				= 0x80310005,
	CELL_LIBMIXER_ERROR_ALREADY_EXIST		    = 0x80310006, 
	CELL_LIBMIXER_ERROR_FULL					= 0x80310007,
	CELL_LIBMIXER_ERROR_NOT_EXIST				= 0x80310008, 
	CELL_LIBMIXER_ERROR_TYPE_MISMATCH			= 0x80310009, 
	CELL_LIBMIXER_ERROR_NOT_FOUND				= 0x8031000a, 

	//libsnd3 Error Codes
	CELL_SND3_ERROR_PARAM					    = 0x80310301,
	CELL_SND3_ERROR_CREATE_MUTEX				= 0x80310302, 
	CELL_SND3_ERROR_SYNTH					    = 0x80310303, 
	CELL_SND3_ERROR_ALREADY					    = 0x80310304, 
	CELL_SND3_ERROR_NOTINIT						= 0x80310305,
	CELL_SND3_ERROR_SMFFULL					    = 0x80310306, 
	CELL_SND3_ERROR_HD3ID					    = 0x80310307,
	CELL_SND3_ERROR_SMF						    = 0x80310308,
	CELL_SND3_ERROR_SMFCTX					    = 0x80310309, 
	CELL_SND3_ERROR_FORMAT					    = 0x8031030a,
	CELL_SND3_ERROR_SMFID						= 0x8031030b, 
	CELL_SND3_ERROR_SOUNDDATAFULL				= 0x8031030c,
	CELL_SND3_ERROR_VOICENUM					= 0x8031030d, 
	CELL_SND3_ERROR_RESERVEDVOICE				= 0x8031030e, 
	CELL_SND3_ERROR_REQUESTQUEFULL			    = 0x8031030f, 
	CELL_SND3_ERROR_OUTPUTMODE				    = 0x80310310,

	//libsynt2 Error Codes
	CELL_SOUND_SYNTH2_ERROR_FATAL			    = 0x80310201,
	CELL_SOUND_SYNTH2_ERROR_INVALID_PARAMETER   = 0x80310202, 
	CELL_SOUND_SYNTH2_ERROR_ALREADY_INITIALIZED = 0x80310203, 
};


//libaudio datatypes
struct CellAudioPortParam
{ 
	u64 nChannel; 
	u64 nBlock; 
	u64 attr; 
	float level; 
}; 

struct CellAudioPortConfig
{ 
	u32 readIndexAddr; 
	u32 status; 
	u64 nChannel; 
	u64 nBlock; 
	u32 portSize; 
	u32 portAddr; 
};

CellAudioPortParam current_AudioPortParam;
CellAudioPortConfig current_AudioPortConfig;

//libmixer datatypes
typedef void * CellAANHandle;

struct CellSSPlayerConfig 
{
	u32 channels; 
	u32 outputMode; 
}; 

struct CellSSPlayerWaveParam 
{ 
	void *addr;
	int format; 
	u32 samples; 
	u32 loopStartOffset; 
	u32 startOffset; 
};

struct CellSSPlayerCommonParam 
{ 
	u32 loopMode; 
	u32 attackMode; 
};

struct CellSurMixerPosition 
{ 
	float x; 
	float y; 
	float z;
};

struct CellSSPlayerRuntimeInfo 
{ 
	float level; 
	float speed; 
	CellSurMixerPosition position;
};

struct CellSurMixerConfig 
{ 
	s32 priority; 
	u32 chStrips1; 
	u32 chStrips2; 
	u32 chStrips6; 
	u32 chStrips8; 
};

struct CellSurMixerChStripParam 
{ 
	u32 param; 
	void *attribute; 
	int dBSwitch; 
	float floatVal; 
	int intVal; 
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
	s32 addPitch; 
};

struct CellSnd3VoiceBitCtx
{ 
	u32 core;  //[CELL_SND3_MAX_CORE],  unknown identifier
};

struct CellSnd3RequestQueueCtx
{ 
	void *frontQueue; 
	u32 frontQueueSize; 
	void *rearQueue; 
	u32 rearQueueSize; 
};

//libsynt2 datatypes
struct  CellSoundSynth2EffectAttr
{ 
	u16 core; 
	u16 mode; 
	s16 depth_L; 
	s16 depth_R; 
	u16 delay; 
	u16 feedback; 
};

// libaudio Functions
bool g_is_audio_initialized = false;

int cellAudioInit()
{
	UNIMPLEMENTED_FUNC(cellAudio);
	if(g_is_audio_initialized) return CELL_AUDIO_ERROR_ALREADY_INIT;
	g_is_audio_initialized = true;
	return CELL_OK;
}

int cellAudioQuit()
{
	UNIMPLEMENTED_FUNC(cellAudio);
	if (g_is_audio_initialized) return CELL_AUDIO_ERROR_NOT_INIT;
	g_is_audio_initialized = false;
	return CELL_OK;
}

// Audio Ports Setting/Operation Functions
bool g_is_audio_port_open = false;
bool g_is_audio_port_start = false;

int cellAudioPortOpen() //CellAudioPortParam *audioParam, u32 *portNum
{
	UNIMPLEMENTED_FUNC(cellAudio);
	if(g_is_audio_port_open) return CELL_AUDIO_ERROR_PORT_OPEN;
	g_is_audio_port_open = true;
	return CELL_OK;
}

int cellAudioPortClose(u32 portNum)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	if(g_is_audio_port_open) return CELL_AUDIO_ERROR_PORT_NOT_OPEN;
	g_is_audio_port_open = false;
	return CELL_OK;
}

int cellAudioPortStart(u32 portNum)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	if(g_is_audio_port_start) return CELL_AUDIO_ERROR_PORT_ALREADY_RUN;
	g_is_audio_port_start = true;
	return CELL_OK;
}

int cellAudioPortStop(u32 portNum)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	if(g_is_audio_port_start) return CELL_AUDIO_ERROR_PORT_NOT_RUN;
	g_is_audio_port_start = false;
	return CELL_OK;
}

int cellAudioGetPortTimestamp() //u32 portNum, u64 tag, u64 *stamp
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioGetPortConfig() //u32 portNum, CellAudioPortConfig *portConfig
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioGetPortBlockTag() //u32 portNum, u64 blockNo, u64 *tag
{
	UNIMPLEMENTED_FUNC (cellAudio);
	return CELL_OK;
}

int cellAudioSetPortLevel(u32 portNum, float level)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

// Utility Functions  
int cellAudioCreateNotifyEventQueue() //u32 *id, u64 *key
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioCreateNotifyEventQueueEx(u32 *id, u64 *key, u32 iFlags)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioSetNotifyEventQueue(u64 key)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioSetNotifyEventQueueEx(u64 key, u32 iFlags)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioRemoveNotifyEventQueue(u64 key)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioRemoveNotifyEventQueueEx(u64 key, u32 iFlags)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioAddData() //u32 portNum, float *src, unsigned int samples, float volume
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioAdd2chData() //u32 portNum, float *src, unsigned int samples, float volume
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioMiscSetAccessoryVolume(u32 devNum, float volume)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioSendAck(u64 data3)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioSetPersonalDevice(int iPersonalStream, int iDevice)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioUnsetPersonalDevice(int iPersonalStream)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioAdd6chData(u32 portNum, float *src, float volume)
{
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
