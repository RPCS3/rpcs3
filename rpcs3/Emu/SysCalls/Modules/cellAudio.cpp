#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

<<<<<<< HEAD
														
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


//libaudio datatyps
struct CellAudioPortParam
{ 
u64 nChannel; 
u64 nBlock; 
u64 attr; 
float level; 
=======
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
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
}; 

struct CellAudioPortConfig
{ 
<<<<<<< HEAD
u32 readIndexAddr; 
u32 status; 
u64 nChannel; 
u64 nBlock; 
u32 portSize; 
u32 portAddr; 
=======
	u32 readIndexAddr; 
	u32 status; 
	u64 nChannel; 
	u64 nBlock; 
	u32 portSize; 
	u32 portAddr; 
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
};

CellAudioPortParam current_AudioPortParam;
CellAudioPortConfig current_AudioPortConfig;

//libmixer datatypes
<<<<<<< HEAD

typedef void * CellAANHandle;


struct CellSSPlayerConfig 
{
u32 channels; 
u32 outputMode; 
=======
typedef void * CellAANHandle;

struct CellSSPlayerConfig 
{
	u32 channels; 
	u32 outputMode; 
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
}; 

struct CellSSPlayerWaveParam 
{ 
<<<<<<< HEAD
void *addr; 
int format; 
u32 samples; 
u32 loopStartOffset; 
u32 startOffset; 
};

CellSSPlayerWaveParam current_SSPlayerWaveParam;

struct CellSSPlayerCommonParam 
{ 
u32 loopMode; 
u32 attackMode; 
=======
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
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
};

struct CellSurMixerPosition 
{ 
<<<<<<< HEAD
float x; 
float y; 
float z;
=======
	float x; 
	float y; 
	float z;
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
};

struct CellSSPlayerRuntimeInfo 
{ 
<<<<<<< HEAD
float level; 
float speed; 
CellSurMixerPosition position;
=======
	float level; 
	float speed; 
	CellSurMixerPosition position;
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
};

struct CellSurMixerConfig 
{ 
<<<<<<< HEAD
s32 priority; 
u32 chStrips1; 
u32 chStrips2; 
u32 chStrips6; 
u32 chStrips8; 
=======
	s32 priority; 
	u32 chStrips1; 
	u32 chStrips2; 
	u32 chStrips6; 
	u32 chStrips8; 
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
};

struct CellSurMixerChStripParam 
{ 
<<<<<<< HEAD
u32 param; 
void *attribute; 
int dBSwitch; 
float floatVal; 
int intVal; 
};


//libsnd3 datatypes

struct CellSnd3DataCtx
{ 
s8 system;  //[CELL_SND3_DATA_CTX_SIZE], unknown identifier
=======
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
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
}; 

struct CellSnd3SmfCtx
{ 
<<<<<<< HEAD
s8 system;  //[CELL_SND3_SMF_CTX_SIZE],  unknown identifier
=======
	s8 system;  //[CELL_SND3_SMF_CTX_SIZE],  unknown identifier
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
};

struct CellSnd3KeyOnParam
{ 
<<<<<<< HEAD
u8 vel; 
u8 pan; 
u8 panEx; 
s32 addPitch; 
=======
	u8 vel; 
	u8 pan; 
	u8 panEx; 
	s32 addPitch; 
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
};

struct CellSnd3VoiceBitCtx
{ 
<<<<<<< HEAD
u32 core;  //[CELL_SND3_MAX_CORE],  unknown identifier
=======
	u32 core;  //[CELL_SND3_MAX_CORE],  unknown identifier
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
};

struct CellSnd3RequestQueueCtx
{ 
<<<<<<< HEAD
void *frontQueue; 
u32 frontQueueSize; 
void *rearQueue; 
u32 rearQueueSize; 
=======
	void *frontQueue; 
	u32 frontQueueSize; 
	void *rearQueue; 
	u32 rearQueueSize; 
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
};

//libsynt2 datatypes
struct  CellSoundSynth2EffectAttr
{ 
<<<<<<< HEAD
u16 core; 
u16 mode; 
s16 depth_L; 
s16 depth_R; 
u16 delay; 
u16 feedback; 
};

														//*libaudio functions*//

bool g_is_audio_initialized = false;
int cellAudioInit (void)
=======
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
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	if(g_is_audio_initialized) return CELL_AUDIO_ERROR_ALREADY_INIT;
	g_is_audio_initialized = true;
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellAudioQuit(void)
=======
int cellAudioQuit()
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	if (g_is_audio_initialized) return CELL_AUDIO_ERROR_NOT_INIT;
	g_is_audio_initialized = false;
	return CELL_OK;
}

// Audio Ports Setting/Operation Functions
<<<<<<< HEAD

bool g_is_audio_port_open = false;
int cellAudioPortOpen () //CellAudioPortParam *audioParam, u32 *portNum
=======
bool g_is_audio_port_open = false;
bool g_is_audio_port_start = false;

int cellAudioPortOpen() //CellAudioPortParam *audioParam, u32 *portNum
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	if(g_is_audio_port_open) return CELL_AUDIO_ERROR_PORT_OPEN;
	g_is_audio_port_open = true;
	return CELL_OK;
}

<<<<<<< HEAD
int cellAudioPortClose (u32 portNum)
=======
int cellAudioPortClose(u32 portNum)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	if(g_is_audio_port_open) return CELL_AUDIO_ERROR_PORT_NOT_OPEN;
	g_is_audio_port_open = false;
	return CELL_OK;
}

<<<<<<< HEAD
bool g_is_audio_port_start = false;
int cellAudioPortStart (u32 portNum)
=======
int cellAudioPortStart(u32 portNum)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	if(g_is_audio_port_start) return CELL_AUDIO_ERROR_PORT_ALREADY_RUN;
	g_is_audio_port_start = true;
	return CELL_OK;
}

<<<<<<< HEAD
int cellAudioPortStop (u32 portNum)
=======
int cellAudioPortStop(u32 portNum)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	if(g_is_audio_port_start) return CELL_AUDIO_ERROR_PORT_NOT_RUN;
	g_is_audio_port_start = false;
	return CELL_OK;
}

<<<<<<< HEAD
int cellAudioGetPortTimestamp () //u32 portNum, u64 tag, u64 *stamp
=======
int cellAudioGetPortTimestamp() //u32 portNum, u64 tag, u64 *stamp
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
int cellAudioGetPortConfig () //u32 portNum, CellAudioPortConfig *portConfig
=======
int cellAudioGetPortConfig() //u32 portNum, CellAudioPortConfig *portConfig
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
int cellAudioGetPortBlockTag () //u32 portNum, u64 blockNo, u64 *tag
=======
int cellAudioGetPortBlockTag() //u32 portNum, u64 blockNo, u64 *tag
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC (cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
int cellAudioSetPortLevel (u32 portNum, float level)
=======
int cellAudioSetPortLevel(u32 portNum, float level)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD

// Utility Functions  
int cellAudioCreateNotifyEventQueue () //u32 *id, u64 *key
=======
// Utility Functions  
int cellAudioCreateNotifyEventQueue() //u32 *id, u64 *key
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
int cellAudioCreateNotifyEventQueueEx (u32 *id, u64 *key, u32 iFlags)
=======
int cellAudioCreateNotifyEventQueueEx(u32 *id, u64 *key, u32 iFlags)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
int cellAudioSetNotifyEventQueue (u64 key)
=======
int cellAudioSetNotifyEventQueue(u64 key)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
int cellAudioSetNotifyEventQueueEx (u64 key, u32 iFlags)
=======
int cellAudioSetNotifyEventQueueEx(u64 key, u32 iFlags)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
int cellAudioRemoveNotifyEventQueue (u64 key)
=======
int cellAudioRemoveNotifyEventQueue(u64 key)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
int cellAudioRemoveNotifyEventQueueEx (u64 key, u32 iFlags)
=======
int cellAudioRemoveNotifyEventQueueEx(u64 key, u32 iFlags)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
int cellAudioAddData () //u32 portNum, float *src, unsigned int samples, float volume
=======
int cellAudioAddData() //u32 portNum, float *src, unsigned int samples, float volume
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
int cellAudioAdd2chData () //u32 portNum, float *src, unsigned int samples, float volume
=======
int cellAudioAdd2chData() //u32 portNum, float *src, unsigned int samples, float volume
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
int cellAudioMiscSetAccessoryVolume (u32 devNum, float volume)
=======
int cellAudioMiscSetAccessoryVolume(u32 devNum, float volume)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
int cellAudioSendAck (u64 data3)
=======
int cellAudioSendAck(u64 data3)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
int cellAudioSetPersonalDevice (int iPersonalStream, int iDevice)
=======
int cellAudioSetPersonalDevice(int iPersonalStream, int iDevice)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
int cellAudioUnsetPersonalDevice (int iPersonalStream)
=======
int cellAudioUnsetPersonalDevice(int iPersonalStream)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
int cellAudioAdd6chData (u32 portNum, float *src, float volume)
=======
int cellAudioAdd6chData(u32 portNum, float *src, float volume)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

//Callback Functions 
typedef int (*CellSurMixerNotifyCallbackFunction)(void *arg, u32 counter, u32 samples); //Currently unused.

<<<<<<< HEAD


                                             //*libmixer Functions, NON active in this moment*//


int cellAANConnect (CellAANHandle receive, u32 receivePortNo, CellAANHandle source, u32 sourcePortNo)
=======
// libmixer Functions, NOT active in this moment
int cellAANConnect(CellAANHandle receive, u32 receivePortNo, CellAANHandle source, u32 sourcePortNo)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

<<<<<<< HEAD
int cellAANDisconnect (CellAANHandle receive, u32 receivePortNo, CellAANHandle source, u32 sourcePortNo)
=======
int cellAANDisconnect(CellAANHandle receive, u32 receivePortNo, CellAANHandle source, u32 sourcePortNo)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

<<<<<<< HEAD
int cellAANAddData (CellAANHandle handle, u32 port, u32 offset, float *addr, u32 samples)
=======
int cellAANAddData(CellAANHandle handle, u32 port, u32 offset, float *addr, u32 samples)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0; 
}
 
<<<<<<< HEAD
int cellSSPlayerCreate (CellAANHandle *handle, CellSSPlayerConfig *config)
=======
int cellSSPlayerCreate(CellAANHandle *handle, CellSSPlayerConfig *config)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

<<<<<<< HEAD
int cellSSPlayerRemove (CellAANHandle handle)
=======
int cellSSPlayerRemove(CellAANHandle handle)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

<<<<<<< HEAD
int cellSSPlayerSetWave () //CellAANHandle handle, CellSSPlayerWaveParam *waveInfo, CellSSPlayerCommonParam *commonInfo  //mem_class_t waveInfo
=======
int cellSSPlayerSetWave() //CellAANHandle handle, CellSSPlayerWaveParam *waveInfo, CellSSPlayerCommonParam *commonInfo  //mem_class_t waveInfo
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

<<<<<<< HEAD
int cellSSPlayerPlay () //CellAANHandle handle, CellSSPlayerRuntimeInfo *info
=======
int cellSSPlayerPlay() //CellAANHandle handle, CellSSPlayerRuntimeInfo *info
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

<<<<<<< HEAD
int cellSSPlayerStop () //CellAANHandle handle, u32 mode
=======
int cellSSPlayerStop() //CellAANHandle handle, u32 mode
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

<<<<<<< HEAD
int cellSSPlayerSetParam () //CellAANHandle handle, CellSSPlayerRuntimeInfo *info
=======
int cellSSPlayerSetParam() //CellAANHandle handle, CellSSPlayerRuntimeInfo *info
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}
 
<<<<<<< HEAD
s32 cellSSPlayerGetState () //CellAANHandle handle
=======
s32 cellSSPlayerGetState() //CellAANHandle handle
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

<<<<<<< HEAD
int cellSurMixerCreate () //const CellSurMixerConfig *config
=======
int cellSurMixerCreate() //const CellSurMixerConfig *config
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

<<<<<<< HEAD
int cellSurMixerGetAANHandle () //CellAANHandle *handle
=======
int cellSurMixerGetAANHandle() //CellAANHandle *handle
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

<<<<<<< HEAD
int cellSurMixerChStripGetAANPortNo () //u32 *port, u32 type, u32 index
=======
int cellSurMixerChStripGetAANPortNo() //u32 *port, u32 type, u32 index
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

<<<<<<< HEAD
int cellSurMixerSetNotifyCallback () //CellSurMixerNotifyCallbackFunction callback, void *arg
=======
int cellSurMixerSetNotifyCallback() //CellSurMixerNotifyCallbackFunction callback, void *arg
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

<<<<<<< HEAD
int cellSurMixerRemoveNotifyCallback () //CellSurMixerNotifyCallbackFunction callback
=======
int cellSurMixerRemoveNotifyCallback() //CellSurMixerNotifyCallbackFunction callback
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

<<<<<<< HEAD
int cellSurMixerStart(void)
=======
int cellSurMixerStart()
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

<<<<<<< HEAD
int cellSurMixerSurBusAddData () //u32 busNo, u32 offset, float *addr, u32 samples
=======
int cellSurMixerSurBusAddData() //u32 busNo, u32 offset, float *addr, u32 samples
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

<<<<<<< HEAD
int cellSurMixerSetParameter () //u32 param, float value
=======
int cellSurMixerSetParameter() //u32 param, float value
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

<<<<<<< HEAD
int cellSurMixerChStripSetParameter () //u32 type, u32 index, CellSurMixerChStripParam *param
=======
int cellSurMixerChStripSetParameter() //u32 type, u32 index, CellSurMixerChStripParam *param
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

int cellSurMixerPause() //u32 switch
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

<<<<<<< HEAD
int cellSurMixerGetCurrentBlockTag () //u64 *tag
=======
int cellSurMixerGetCurrentBlockTag() //u64 *tag
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

<<<<<<< HEAD
int cellSurMixerGetTimestamp () //u64 tag, u64 *stamp
=======
int cellSurMixerGetTimestamp() //u64 tag, u64 *stamp
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

<<<<<<< HEAD
void cellSurMixerBeep (); //void *arg

float cellSurMixerUtilGetLevelFromDB () //float dB
=======
void cellSurMixerBeep(); //void *arg

float cellSurMixerUtilGetLevelFromDB() //float dB
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO;
}

<<<<<<< HEAD
float cellSurMixerUtilGetLevelFromDBIndex () //int index
=======
float cellSurMixerUtilGetLevelFromDBIndex() //int index
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO;
}

<<<<<<< HEAD
float cellSurMixerUtilNoteToRatio () //unsigned char refNote, unsigned char note
=======
float cellSurMixerUtilNoteToRatio() //unsigned char refNote, unsigned char note
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

<<<<<<< HEAD
int cellSurMixerFinalize(void); //Currently unused. Returns 0 (in the current release).


											//*libsnd3 Functions, NON active in this moment*//

s32 cellSnd3Init () //u32 maxVoice, u32 samples, CellSnd3RequestQueueCtx *queue
=======
int cellSurMixerFinalize(); //Currently unused. Returns 0 (in the current release).

//*libsnd3 Functions, NOT active in this moment
s32 cellSnd3Init() //u32 maxVoice, u32 samples, CellSnd3RequestQueueCtx *queue
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3Exit (void)
=======
s32 cellSnd3Exit()
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3SetOutputMode () //u32 mode
=======
s32 cellSnd3SetOutputMode() //u32 mode
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3Synthesis () //float *pOutL, float *pOutR
=======
s32 cellSnd3Synthesis() //float *pOutL, float *pOutR
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3SynthesisEx () //float *pOutL, float *pOutR, float *pOutRL, float *pOutRR
=======
s32 cellSnd3SynthesisEx() //float *pOutL, float *pOutR, float *pOutRL, float *pOutRR
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3VoiceSetReserveMode () //u32 voiceNum, u32 reserveMode
=======
s32 cellSnd3VoiceSetReserveMode() //u32 voiceNum, u32 reserveMode
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3BindSoundData () //CellSnd3DataCtx *snd3Ctx, void *hd3, u32 synthMemOffset
=======
s32 cellSnd3BindSoundData() //CellSnd3DataCtx *snd3Ctx, void *hd3, u32 synthMemOffset
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3UnbindSoundData (u32 hd3ID)
=======
s32 cellSnd3UnbindSoundData(u32 hd3ID)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3NoteOnByTone () //u32 hd3ID, u32 toneIndex, u32 note, u32 keyOnID, CellSnd3KeyOnParam *keyOnParam
=======
s32 cellSnd3NoteOnByTone() //u32 hd3ID, u32 toneIndex, u32 note, u32 keyOnID, CellSnd3KeyOnParam *keyOnParam
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

<<<<<<< HEAD
s32 cellSnd3KeyOnByTone () //u32 hd3ID, u32 toneIndex,  u32 pitch,u32 keyOnID,CellSnd3KeyOnParam *keyOnParam
=======
s32 cellSnd3KeyOnByTone() //u32 hd3ID, u32 toneIndex,  u32 pitch,u32 keyOnID,CellSnd3KeyOnParam *keyOnParam
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

<<<<<<< HEAD
s32 cellSnd3VoiceNoteOnByTone () //u32 hd3ID, u32 voiceNum, u32 toneIndex, u32 note, u32 keyOnID, CellSnd3KeyOnParam *keyOnParam
=======
s32 cellSnd3VoiceNoteOnByTone() //u32 hd3ID, u32 voiceNum, u32 toneIndex, u32 note, u32 keyOnID, CellSnd3KeyOnParam *keyOnParam
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

<<<<<<< HEAD
s32 cellSnd3VoiceKeyOnByTone () //u32 hd3ID, u32 voiceNum, u32 toneIndex, u32 pitch, u32 keyOnID, CellSnd3KeyOnParam *keyOnParam
=======
s32 cellSnd3VoiceKeyOnByTone() //u32 hd3ID, u32 voiceNum, u32 toneIndex, u32 pitch, u32 keyOnID, CellSnd3KeyOnParam *keyOnParam
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

<<<<<<< HEAD
s32 cellSnd3VoiceSetSustainHold (u32 voiceNum, u32 sustainHold)
=======
s32 cellSnd3VoiceSetSustainHold(u32 voiceNum, u32 sustainHold)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3VoiceKeyOff (u32 voiceNum)
=======
s32 cellSnd3VoiceKeyOff(u32 voiceNum)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

s32 cellSnd3VoiceSetPitch(u32 voiceNum, s32 addPitch)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3VoiceSetVelocity (u32 voiceNum, u32 velocity)
=======
s32 cellSnd3VoiceSetVelocity(u32 voiceNum, u32 velocity)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3VoiceSetPanpot (u32 voiceNum, u32 panpot)
=======
s32 cellSnd3VoiceSetPanpot(u32 voiceNum, u32 panpot)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3VoiceSetPanpotEx (u32 voiceNum, u32 panpotEx)
=======
s32 cellSnd3VoiceSetPanpotEx(u32 voiceNum, u32 panpotEx)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3VoiceSetPitchBend (u32 voiceNum, u32 bendValue)
=======
s32 cellSnd3VoiceSetPitchBend(u32 voiceNum, u32 bendValue)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3VoiceAllKeyOff (void)
=======
s32 cellSnd3VoiceAllKeyOff()
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3VoiceGetEnvelope (u32 voiceNum)
=======
s32 cellSnd3VoiceGetEnvelope(u32 voiceNum)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3VoiceGetStatus ()  //u32 voiceNum
=======
s32 cellSnd3VoiceGetStatus()  //u32 voiceNum
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

<<<<<<< HEAD
u32 cellSnd3KeyOffByID (u32 keyOnID)
=======
u32 cellSnd3KeyOffByID(u32 keyOnID)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3GetVoice () //u32 midiChannel, u32 keyOnID, CellSnd3VoiceBitCtx *voiceBit
=======
s32 cellSnd3GetVoice() //u32 midiChannel, u32 keyOnID, CellSnd3VoiceBitCtx *voiceBit
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3GetVoiceByID () //u32 keyOnID, CellSnd3VoiceBitCtx *voiceBit
=======
s32 cellSnd3GetVoiceByID() //u32 keyOnID, CellSnd3VoiceBitCtx *voiceBit
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3NoteOn () //u32 hd3ID, u32 midiChannel, u32 midiProgram, u32 midiNote, u32 sustain,CellSnd3KeyOnParam *keyOnParam, u32 keyOnID
=======
s32 cellSnd3NoteOn() //u32 hd3ID, u32 midiChannel, u32 midiProgram, u32 midiNote, u32 sustain,CellSnd3KeyOnParam *keyOnParam, u32 keyOnID
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3NoteOff (u32 midiChannel, u32 midiNote, u32 keyOnID)
=======
s32 cellSnd3NoteOff(u32 midiChannel, u32 midiNote, u32 keyOnID)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3SetSustainHold (u32 midiChannel, u32 sustainHold, u32 ID)
=======
s32 cellSnd3SetSustainHold(u32 midiChannel, u32 sustainHold, u32 ID)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3SetEffectType (u16 effectType, s16 returnVol, u16 delay, u16 feedback)
=======
s32 cellSnd3SetEffectType(u16 effectType, s16 returnVol, u16 delay, u16 feedback)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
u16 cellSnd3Note2Pitch ()  //u16 center_note, u16 center_fine, u16 note, s16 fine
=======
u16 cellSnd3Note2Pitch() //u16 center_note, u16 center_fine, u16 note, s16 fine
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

<<<<<<< HEAD
u16 cellSnd3Pitch2Note ()  //u16 center_note, u16 center_fine, u16 pitch
=======
u16 cellSnd3Pitch2Note() //u16 center_note, u16 center_fine, u16 pitch
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

<<<<<<< HEAD
s32 cellSnd3SMFBind ()  //CellSnd3SmfCtx *smfCtx, void *smf, u32 hd3ID
=======
s32 cellSnd3SMFBind() //CellSnd3SmfCtx *smfCtx, void *smf, u32 hd3ID
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

<<<<<<< HEAD
s32 cellSnd3SMFUnbind () //u32 smfID
=======
s32 cellSnd3SMFUnbind() //u32 smfID
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

<<<<<<< HEAD
s32 cellSnd3SMFPlay (u32 smfID, u32 playVelocity, u32 playPan, u32 playCount)
=======
s32 cellSnd3SMFPlay(u32 smfID, u32 playVelocity, u32 playPan, u32 playCount)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3SMFPlayEx (u32 smfID, u32 playVelocity, u32 playPan, u32 playPanEx, u32 playCount)
=======
s32 cellSnd3SMFPlayEx(u32 smfID, u32 playVelocity, u32 playPan, u32 playPanEx, u32 playCount)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3SMFPause (u32 smfID)
=======
s32 cellSnd3SMFPause(u32 smfID)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3SMFResume (u32 smfID)
=======
s32 cellSnd3SMFResume(u32 smfID)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3SMFStop (u32 smfID)
=======
s32 cellSnd3SMFStop(u32 smfID)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3SMFAddTempo (u32 smfID, s32 addTempo)
=======
s32 cellSnd3SMFAddTempo(u32 smfID, s32 addTempo)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3SMFGetTempo () //u32 smfID
=======
s32 cellSnd3SMFGetTempo() //u32 smfID
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

<<<<<<< HEAD
s32 cellSnd3SMFSetPlayVelocity (u32 smfID, u32 playVelocity)
=======
s32 cellSnd3SMFSetPlayVelocity(u32 smfID, u32 playVelocity)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3SMFGetPlayVelocity ()  //u32 smfID
=======
s32 cellSnd3SMFGetPlayVelocity()  //u32 smfID
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

<<<<<<< HEAD
s32 cellSnd3SMFSetPlayPanpot (u32 smfID, u32 playPanpot)
=======
s32 cellSnd3SMFSetPlayPanpot(u32 smfID, u32 playPanpot)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3SMFSetPlayPanpotEx (u32 smfID, u32 playPanpotEx)
=======
s32 cellSnd3SMFSetPlayPanpotEx(u32 smfID, u32 playPanpotEx)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3SMFGetPlayPanpot () //u32 smfID
=======
s32 cellSnd3SMFGetPlayPanpot() //u32 smfID
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

<<<<<<< HEAD
s32 cellSnd3SMFGetPlayPanpotEx () //u32 smfID
=======
s32 cellSnd3SMFGetPlayPanpotEx() //u32 smfID
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

<<<<<<< HEAD
s32 cellSnd3SMFGetPlayStatus ()  //u32 smfID
=======
s32 cellSnd3SMFGetPlayStatus()  //u32 smfID
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

<<<<<<< HEAD
s32 cellSnd3SMFSetPlayChannel (u32 smfID, u32 playChannelBit)
=======
s32 cellSnd3SMFSetPlayChannel(u32 smfID, u32 playChannelBit)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3SMFGetPlayChannel () //u32 smfID, u32 *playChannelBit
=======
s32 cellSnd3SMFGetPlayChannel() //u32 smfID, u32 *playChannelBit
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

<<<<<<< HEAD
s32 cellSnd3SMFGetKeyOnID () //u32 smfID, u32 midiChannel, u32 *keyOnID
=======
s32 cellSnd3SMFGetKeyOnID() //u32 smfID, u32 midiChannel, u32 *keyOnID
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK;
}

                                           //*libsynth2 Functions, NON active in this moment*//

<<<<<<< HEAD
s32 cellSoundSynth2Config (s16 param, s32 value)
=======
s32 cellSoundSynth2Config(s16 param, s32 value)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

s32 cellSoundSynth2Init( s16 flag)
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

<<<<<<< HEAD
s32 cellSoundSynth2Exit(void)
=======
s32 cellSoundSynth2Exit()
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

<<<<<<< HEAD
void cellSoundSynth2SetParam (u16 register, u16 value)
=======
void cellSoundSynth2SetParam(u16 register, u16 value)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	//TODO
}

<<<<<<< HEAD
u16 cellSoundSynth2GetParam () //u16 register
=======
u16 cellSoundSynth2GetParam() //u16 register
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

<<<<<<< HEAD
void cellSoundSynth2SetSwitch (u16 register, u32 value)
=======
void cellSoundSynth2SetSwitch(u16 register, u32 value)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	//TODO
}

<<<<<<< HEAD
u32 cellSoundSynth2GetSwitch () //u16 register
=======
u32 cellSoundSynth2GetSwitch() //u16 register
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

<<<<<<< HEAD
u32 cellSoundSynth2SetAddr (u16 register, u32 value)
=======
u32 cellSoundSynth2SetAddr(u16 register, u32 value)
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

<<<<<<< HEAD
u32 cellSoundSynth2GetAddr () //u16 register
=======
u32 cellSoundSynth2GetAddr() //u16 register
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

<<<<<<< HEAD
s32 cellSoundSynth2SetEffectAttr () //s16 bus, CellSoundSynth2EffectAttr *attr
=======
s32 cellSoundSynth2SetEffectAttr() //s16 bus, CellSoundSynth2EffectAttr *attr
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

<<<<<<< HEAD
s32 cellSoundSynth2SetEffectMode () //s16 bus, CellSoundSynth2EffectAttr *attr
=======
s32 cellSoundSynth2SetEffectMode() //s16 bus, CellSoundSynth2EffectAttr *attr
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

<<<<<<< HEAD
void cellSoundSynth2SetCoreAttr (u16 entry, u16 value) 
=======
void cellSoundSynth2SetCoreAttr(u16 entry, u16 value) 
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	//TODO
}

<<<<<<< HEAD
s32 cellSoundSynth2Generate () //u16 samples, float *left_buffer, float *right_buffer, float *left_rear, float *right_rear
=======
s32 cellSoundSynth2Generate() //u16 samples, float *left_buffer, float *right_buffer, float *left_rear, float *right_rear
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

<<<<<<< HEAD
s32 cellSoundSynth2VoiceTrans () //s16 channel, u16 mode, u8 *m_addr, u32 s_addr, u32 size
=======
s32 cellSoundSynth2VoiceTrans() //s16 channel, u16 mode, u8 *m_addr, u32 s_addr, u32 size
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return 0;
}

<<<<<<< HEAD
u16 cellSoundSynth2Note2Pitch () //u16 center_note, u16 center_fine, u16 note, s16 fine
=======
u16 cellSoundSynth2Note2Pitch() //u16 center_note, u16 center_fine, u16 note, s16 fine
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}

<<<<<<< HEAD
u16 cellSoundSynth2Pitch2Note () //u16 center_note, u16 center_fine, u16 pitch
=======
u16 cellSoundSynth2Pitch2Note() //u16 center_note, u16 center_fine, u16 pitch
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
{
	UNIMPLEMENTED_FUNC(cellAudio);
	return CELL_OK; //it's NOT real value
	//TODO
}


<<<<<<< HEAD
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
=======
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
>>>>>>> 3dd9683b472b89358a697210798c89df5b0e5baa
}
