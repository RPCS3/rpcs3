#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

														
// Initialization and Termination Functions;

void cellAudio_init ();
Module cellAudio (0x0011, cellAudio_init);

enum 
{
//libaudio ERROR_CODES
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

//libmixer ERROR_CODES
CELL_LIBMIXER_ERROR_NOT_INITIALIZED         = 0x80310002,
CELL_LIBMIXER_ERROR_INVALID_PARAMATER       = 0x80310003,
CELL_LIBMIXER_ERROR_NO_MEMORY				= 0x80310005,
CELL_LIBMIXER_ERROR_ALREADY_EXIST		    = 0x80310006, 
CELL_LIBMIXER_ERROR_FULL					= 0x80310007,
CELL_LIBMIXER_ERROR_NOT_EXIST				= 0x80310008, 
CELL_LIBMIXER_ERROR_TYPE_MISMATCH			= 0x80310009, 
CELL_LIBMIXER_ERROR_NOT_FOUND				= 0x8031000a, 

//libsnd3 ERROR_CODES
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

//libsynt2 EROR_CODES
CELL_SOUND_SYNTH2_ERROR_FATAL			    = 0x80310201,
CELL_SOUND_SYNTH2_ERROR_INVALID_PARAMETER   = 0x80310202, 
CELL_SOUND_SYNTH2_ERROR_ALREADY_INITIALIZED = 0x80310203, 
};

#define __CSTD
typedef __CSTD uint32_t sys_timer_t;
typedef __CSTD int64_t system_time_t;    
typedef __CSTD uint64_t usecond_t;       
typedef __CSTD uint32_t second_t;
typedef __CSTD uint32_t sys_event_queue_t;
typedef __CSTD uint32_t sys_event_port_t;
typedef __CSTD uint32_t sys_event_type_t;
typedef __CSTD uint64_t sys_ipc_key_t; 
typedef __CSTD uintptr_t lparaddr_t;
typedef __CSTD uintptr_t sys_addr_t;
typedef __CSTD uint32_t sys_memory_t;
typedef __CSTD uint32_t sys_memory_container_t;
#undef __CSTD

//libaudio datatyps
struct CellAudioPortParam
{ 
uint64_t nChannel; 
uint64_t nBlock; 
uint64_t attr; 
float level; 
}; 

struct CellAudioPortConfig
{ 
sys_addr_t readIndexAddr; 
uint32_t status; 
uint64_t nChannel; 
uint64_t nBlock; 
uint32_t portSize; 
sys_addr_t portAddr; 
};

//libmixer datatypes

typedef void * CellAANHandle;


struct CellSSPlayerConfig 
{
uint32_t channels; 
uint32_t outputMode; 
}; 

struct CellSSPlayerWaveParam 
{ 
void *addr; 
int format; 
uint32_t samples; 
uint32_t loopStartOffset; 
uint32_t startOffset; 
};

struct CellSSPlayerCommonParam 
{ 
uint32_t loopMode; 
uint32_t attackMode; 
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
int32_t priority; 
uint32_t chStrips1; 
uint32_t chStrips2; 
uint32_t chStrips6; 
uint32_t chStrips8; 
};

struct CellSurMixerChStripParam 
{ 
uint32_t param; 
void *attribute; 
int dBSwitch; 
float floatVal; 
int intVal; 
};


//libsnd3 datatypes

struct CellSnd3DataCtx
{ 
int8_t system;  //[CELL_SND3_DATA_CTX_SIZE], unknown identifier
}; 

struct CellSnd3SmfCtx
{ 
int8_t system;  //[CELL_SND3_SMF_CTX_SIZE],  unknown identifier
};

struct CellSnd3KeyOnParam
{ 
uint8_t vel; 
uint8_t pan; 
uint8_t panEx; 
int32_t addPitch; 
};

struct CellSnd3VoiceBitCtx
{ 
uint32_t core;  //[CELL_SND3_MAX_CORE],  unknown identifier
};

struct CellSnd3RequestQueueCtx
{ 
void *frontQueue; 
uint32_t frontQueueSize; 
void *rearQueue; 
uint32_t rearQueueSize; 
};

//libsynt2 datatypes
struct  CellSoundSynth2EffectAttr
{ 
uint16_t core; 
uint16_t mode; 
int16_t depth_L; 
int16_t depth_R; 
uint16_t delay; 
uint16_t feedback; 
};


														//*libaudio functions*//

int cellAudioInit (void)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellAudioQuit(void)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

// Audio Ports Setting/Operation Functions
int cellAudioPortOpen () //CellAudioPortParam *audioParam, uint32_t *portNum
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioPortStart (uint32_t portNum)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioPortClose (uint32_t portNum)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioPortStop (uint32_t portNum)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioGetPortConfig () //uint32_t portNum, CellAudioPortConfig *portConfig
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioGetPortBlockTag () //uint32_t portNum, uint64_t blockNo, uint64_t *tag
{
	UNIMPLEMENTED_FUNC (cellAudio);
	return CELL_OK;
}

int cellAudioGetPortTimestamp () //uint32_t portNum, uint64_t tag, usecond_t *stamp
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioSetPortLevel (uint32_t portNum, float level)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}


// Utility Functions  
int cellAudioCreateNotifyEventQueue () //sys_event_queue_t *id, sys_ipc_key_t *key
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioCreateNotifyEventQueueEx (sys_event_queue_t *id, sys_ipc_key_t *key, uint32_t iFlags)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioSetNotifyEventQueue (sys_ipc_key_t key)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioSetNotifyEventQueueEx (sys_ipc_key_t key, uint32_t iFlags)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioRemoveNotifyEventQueue (sys_ipc_key_t key)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioRemoveNotifyEventQueueEx (sys_ipc_key_t key, uint32_t iFlags)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioAddData () //uint32_t portNum, float *src, unsigned int samples, float volume
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioAdd2chData () //uint32_t portNum, float *src, unsigned int samples, float volume
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioMiscSetAccessoryVolume (uint32_t devNum, float volume)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioSendAck (uint64_t data3)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioSetPersonalDevice (int iPersonalStream, int iDevice)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioUnsetPersonalDevice (int iPersonalStream)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int cellAudioAdd6chData (uint32_t portNum, float *src, float volume)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

//Callback Functions 
typedef int (*CellSurMixerNotifyCallbackFunction)(void *arg, uint32_t counter, uint32_t samples); //Currently unused.


                                             //*libmixer Functions, NON active in this moment*//


int cellAANConnect (CellAANHandle receive, uint32_t receivePortNo, CellAANHandle source, uint32_t sourcePortNo)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellAANDisconnect (CellAANHandle receive, uint32_t receivePortNo, CellAANHandle source, uint32_t sourcePortNo)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellAANAddData (CellAANHandle handle, uint32_t port, uint32_t offset, float *addr, uint32_t samples)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0; 
}
 
int cellSSPlayerCreate (CellAANHandle *handle, CellSSPlayerConfig *config)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSSPlayerRemove (CellAANHandle handle)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSSPlayerSetWave (CellAANHandle handle, CellSSPlayerWaveParam *waveInfo, CellSSPlayerCommonParam *commonInfo)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSSPlayerPlay (CellAANHandle handle, CellSSPlayerRuntimeInfo *info)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSSPlayerStop (CellAANHandle handle, uint32_t mode)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSSPlayerSetParam (CellAANHandle handle, CellSSPlayerRuntimeInfo *info)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}
 
int32_t cellSSPlayerGetState (CellAANHandle handle)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSurMixerCreate (const CellSurMixerConfig *config)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSurMixerGetAANHandle (CellAANHandle *handle)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSurMixerChStripGetAANPortNo (uint32_t *port, uint32_t type, uint32_t index)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSurMixerSetNotifyCallback (CellSurMixerNotifyCallbackFunction callback, void *arg)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSurMixerRemoveNotifyCallback (CellSurMixerNotifyCallbackFunction callback)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSurMixerStart(void)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSurMixerSurBusAddData (uint32_t busNo, uint32_t offset, float *addr, uint32_t samples)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSurMixerSetParameter (uint32_t param, float value)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSurMixerChStripSetParameter (uint32_t type, uint32_t index, CellSurMixerChStripParam *param)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSurMixerPause() //uint32_t switch
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSurMixerGetCurrentBlockTag (uint64_t *tag)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSurMixerGetTimestamp (uint64_t tag, usecond_t *stamp)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

void cellSurMixerBeep (void *arg);

float cellSurMixerUtilGetLevelFromDB () //float dB
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO;
}

float cellSurMixerUtilGetLevelFromDBIndex () //int index
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO;
}

float cellSurMixerUtilNoteToRatio () //unsigned char refNote, unsigned char note
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

int cellSurMixerFinalize(void); //Currently unused. Returns 0 (in the current release).


											//*libsnd3 Functions, NON active in this moment*//

int32_t cellSnd3Init (uint32_t maxVoice, uint32_t samples, CellSnd3RequestQueueCtx *queue)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3Exit (void)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3SetOutputMode (uint32_t mode)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3Synthesis (float *pOutL, float *pOutR)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3SynthesisEx (float *pOutL, float *pOutR, float *pOutRL, float *pOutRR)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3VoiceSetReserveMode (uint32_t voiceNum, uint32_t reserveMode)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3BindSoundData (CellSnd3DataCtx *snd3Ctx, void *hd3, uint32_t synthMemOffset)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3UnbindSoundData (uint32_t hd3ID)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3NoteOnByTone () //uint32_t hd3ID, uint32_t toneIndex, uint32_t note, uint32_t keyOnID, CellSnd3KeyOnParam *keyOnParam
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

int32_t cellSnd3KeyOnByTone () //uint32_t hd3ID, uint32_t toneIndex,  uint32_t pitch,uint32_t keyOnID,CellSnd3KeyOnParam *keyOnParam
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

int32_t cellSnd3VoiceNoteOnByTone () //uint32_t hd3ID, uint32_t voiceNum, uint32_t toneIndex, uint32_t note, uint32_t keyOnID, CellSnd3KeyOnParam *keyOnParam
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

int32_t cellSnd3VoiceKeyOnByTone () //uint32_t hd3ID, uint32_t voiceNum, uint32_t toneIndex, uint32_t pitch, uint32_t keyOnID, CellSnd3KeyOnParam *keyOnParam
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

int32_t cellSnd3VoiceSetSustainHold (uint32_t voiceNum, uint32_t sustainHold)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3VoiceKeyOff (uint32_t voiceNum)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3VoiceSetPitch(uint32_t voiceNum, int32_t addPitch)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3VoiceSetVelocity (uint32_t voiceNum, uint32_t velocity)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3VoiceSetPanpot (uint32_t voiceNum, uint32_t panpot)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3VoiceSetPanpotEx (uint32_t voiceNum, uint32_t panpotEx)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3VoiceSetPitchBend (uint32_t voiceNum, uint32_t bendValue)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3VoiceAllKeyOff (void)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3VoiceGetEnvelope (uint32_t voiceNum)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3VoiceGetStatus ()  //uint32_t voiceNum
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

uint32_t cellSnd3KeyOffByID (uint32_t keyOnID)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3GetVoice (uint32_t midiChannel, uint32_t keyOnID, CellSnd3VoiceBitCtx *voiceBit)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3GetVoiceByID (uint32_t keyOnID, CellSnd3VoiceBitCtx *voiceBit)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3NoteOn (uint32_t hd3ID, uint32_t midiChannel, uint32_t midiProgram, uint32_t midiNote, uint32_t sustain,CellSnd3KeyOnParam *keyOnParam, uint32_t keyOnID)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3NoteOff (uint32_t midiChannel, uint32_t midiNote, uint32_t keyOnID)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3SetSustainHold (uint32_t midiChannel, uint32_t sustainHold, uint32_t ID)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3SetEffectType (uint16_t effectType, int16_t returnVol, uint16_t delay, uint16_t feedback)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

uint16_t cellSnd3Note2Pitch ()  //uint16_t center_note, uint16_t center_fine, uint16_t note, int16_t fine
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

uint16_t cellSnd3Pitch2Note ()  //uint16_t center_note, uint16_t center_fine, uint16_t pitch
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

int32_t cellSnd3SMFBind ()  //CellSnd3SmfCtx *smfCtx, void *smf, uint32_t hd3ID
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

int32_t cellSnd3SMFUnbind () //uint32_t smfID
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

int32_t cellSnd3SMFPlay (uint32_t smfID, uint32_t playVelocity, uint32_t playPan, uint32_t playCount)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3SMFPlayEx (uint32_t smfID, uint32_t playVelocity, uint32_t playPan, uint32_t playPanEx, uint32_t playCount)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3SMFPause (uint32_t smfID)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3SMFResume (uint32_t smfID)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3SMFStop (uint32_t smfID)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3SMFAddTempo (uint32_t smfID, int32_t addTempo)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3SMFGetTempo () //uint32_t smfID
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

int32_t cellSnd3SMFSetPlayVelocity (uint32_t smfID, uint32_t playVelocity)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3SMFGetPlayVelocity ()  //uint32_t smfID
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

int32_t cellSnd3SMFSetPlayPanpot (uint32_t smfID, uint32_t playPanpot)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3SMFSetPlayPanpotEx (uint32_t smfID, uint32_t playPanpotEx)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3SMFGetPlayPanpot () //uint32_t smfID
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

int32_t cellSnd3SMFGetPlayPanpotEx () //uint32_t smfID
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

int32_t cellSnd3SMFGetPlayStatus ()  //uint32_t smfID
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

int32_t cellSnd3SMFSetPlayChannel (uint32_t smfID, uint32_t playChannelBit)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3SMFGetPlayChannel (uint32_t smfID, uint32_t *playChannelBit)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

int32_t cellSnd3SMFGetKeyOnID (uint32_t smfID, uint32_t midiChannel, uint32_t *keyOnID)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

                                           //*libsynth2 Functions, NON active in this moment*//

int32_t cellSoundSynth2Config (int16_t param, int32_t value)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int32_t cellSoundSynth2Init( int16_t flag)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int32_t cellSoundSynth2Exit(void)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

void cellSoundSynth2SetParam (uint16_t register, uint16_t value)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	//TODO
}

uint16_t cellSoundSynth2GetParam () //uint16_t register
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

void cellSoundSynth2SetSwitch (uint16_t register, uint32_t value)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	//TODO
}

uint32_t cellSoundSynth2GetSwitch () //uint16_t register
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

uint32_t cellSoundSynth2SetAddr (uint16_t register, uint32_t value)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

uint32_t cellSoundSynth2GetAddr () //uint16_t register
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

int32_t cellSoundSynth2SetEffectAttr (int16_t bus, CellSoundSynth2EffectAttr *attr)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int32_t cellSoundSynth2SetEffectMode (int16_t bus, CellSoundSynth2EffectAttr *attr)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

void cellSoundSynth2SetCoreAttr (uint16_t entry, uint16_t value) 
{
	UNIMPLEMENTED_FUNC(cellAudio);
	//TODO
}

int32_t cellSoundSynth2Generate (uint16_t samples, float *left_buffer, float *right_buffer, float *left_rear, float *right_rear)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int32_t cellSoundSynth2VoiceTrans () //int16_t channel, uint16_t mode, uint8_t *m_addr, uint32_t s_addr, uint32_t size
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

uint16_t cellSoundSynth2Note2Pitch () //uint16_t center_note, uint16_t center_fine, uint16_t note, int16_t fine
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

uint16_t cellSoundSynth2Pitch2Note () //uint16_t center_note, uint16_t center_fine, uint16_t pitch
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}



void cellAudio_init ()
{
cellAudio.AddFunc (0x0b168f92, cellAudioInit);
cellAudio.AddFunc (0x4129fe2d, cellAudioPortClose);
cellAudio.AddFunc (0x5b1e2c73, cellAudioPortStop);
cellAudio.AddFunc (0x74a66af0, cellAudioGetPortConfig);
cellAudio.AddFunc (0x89be28f2, cellAudioPortStart);
cellAudio.AddFunc (0xca5ac370, cellAudioQuit);
cellAudio.AddFunc (0xcd7bc431, cellAudioPortOpen);
cellAudio.AddFunc (0x56dfe179, cellAudioSetPortLevel);
cellAudio.AddFunc (0x04af134e, cellAudioCreateNotifyEventQueue);
cellAudio.AddFunc (0x31211f6b, cellAudioMiscSetAccessoryVolume);
cellAudio.AddFunc (0x377e0cd9, cellAudioSetNotifyEventQueue);
cellAudio.AddFunc (0x4109d08c, cellAudioGetPortTimestamp);
cellAudio.AddFunc (0x9e4b1db8, cellAudioAdd2chData);
cellAudio.AddFunc (0xdab029aa, cellAudioAddData);
cellAudio.AddFunc (0xe4046afe, cellAudioGetPortBlockTag);
cellAudio.AddFunc (0xff3626fd, cellAudioRemoveNotifyEventQueue);
//TODO: Find addresses for libmixer, libsnd3 and libsynth2 functions
}
