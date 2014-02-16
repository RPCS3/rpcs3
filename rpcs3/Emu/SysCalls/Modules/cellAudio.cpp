#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "Emu/Audio/cellAudio.h"

void cellAudio_init();
void cellAudio_load();
void cellAudio_unload();
Module cellAudio(0x0011, cellAudio_init, cellAudio_load, cellAudio_unload);

extern u64 get_system_time();

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

struct WAVHeader
{
	struct RIFFHeader
	{
		u32 ID; // "RIFF"
		u32 Size; // FileSize - 8
		u32 WAVE; // "WAVE"

		RIFFHeader(u32 size)
			: ID(*(u32*)"RIFF")
			, WAVE(*(u32*)"WAVE")
			, Size(size)
		{
		}
	} RIFF;
	struct FMTHeader
	{
		u32 ID; // "fmt "
		u32 Size; // 16
		u16 AudioFormat; // 1 for PCM, 3 for IEEE Floating Point
		u16 NumChannels; // 1, 2, 6, 8
		u32 SampleRate; // 48000
		u32 ByteRate; // SampleRate * NumChannels * BitsPerSample/8
		u16 BlockAlign; // NumChannels * BitsPerSample/8
		u16 BitsPerSample; // sizeof(float) * 8

		FMTHeader(u8 ch)
			: ID(*(u32*)"fmt ")
			, Size(16)
			, AudioFormat(3)
			, NumChannels(ch)
			, SampleRate(48000)
			, ByteRate(SampleRate * ch * sizeof(float))
			, BlockAlign(ch * sizeof(float))
			, BitsPerSample(sizeof(float) * 8)
		{
		}
	} FMT;
	u32 ID; // "data"
	u32 Size; // size of data (256 * NumChannels * sizeof(float))

	WAVHeader(u8 ch)
		: ID(*(u32*)"data")
		, Size(0)
		, FMT(ch)
		, RIFF(sizeof(RIFFHeader) + sizeof(FMTHeader))
	{
	}
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

struct AudioPortConfig
{
	bool m_is_audio_port_started;
	CellAudioPortParam m_param;

	const u32 m_buffer; // 64 KB or 128 KB with 8x16 config
	const u32 m_index;

	AudioPortConfig();

	void finalize();
};

struct AudioConfig  //custom structure
{
	Array<AudioPortConfig*> m_ports;
	bool m_is_audio_initialized;
	u32 m_port_in_use;
	u64 event_key;

	AudioConfig()
		: m_is_audio_initialized(false)
		, m_port_in_use(0)
		, event_key(0)
	{
		m_ports.SetCount(8);
		for (u32 i = 0; i < m_ports.GetCount(); i++)
			m_ports[i] = nullptr;
	}

	void Clear()
	{
		for (u32 i = 0; i < m_ports.GetCount(); i++)
		{
			if (m_ports[i])
			{
				delete m_ports[i];
				m_ports[i] = nullptr;
			}
		}
		m_port_in_use = 0;
	}
} m_config;

AudioPortConfig::AudioPortConfig()
	: m_is_audio_port_started(false)
	, m_buffer(Memory.Alloc(1024 * 128, 1024)) // max 128K size
	, m_index(Memory.Alloc(16, 16)) // allocation for u64 value "read index"
{
	m_config.m_port_in_use++;
	mem64_t index(m_index);
	index = 0;
}

void AudioPortConfig::finalize()
{
	m_config.m_port_in_use--;
	Memory.Free(m_buffer);
	Memory.Free(m_index);
}

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
	if (m_config.m_is_audio_initialized)
	{
		return CELL_AUDIO_ERROR_ALREADY_INIT;
	}

	m_config.m_is_audio_initialized = true;
	return CELL_OK;
}

int cellAudioQuit()
{
	cellAudio.Warning("cellAudioQuit()");

	if (!m_config.m_is_audio_initialized)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	m_config.m_is_audio_initialized = false;
	return CELL_OK;
}

int cellAudioPortOpen(mem_ptr_t<CellAudioPortParam> audioParam, mem32_t portNum)
{
	cellAudio.Warning("cellAudioPortOpen(audioParam_addr=0x%x, portNum_addr=0x%x)", audioParam.GetAddr(), portNum.GetAddr());

	if(!audioParam.IsGood() || !portNum.IsGood())
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	if (m_config.m_port_in_use >= m_config.m_ports.GetCount())
	{
		return CELL_AUDIO_ERROR_PORT_FULL;
	}

	for (u32 i = 0; i < m_config.m_ports.GetCount(); i++)
	{
		if (m_config.m_ports[i] == nullptr)
		{
			CellAudioPortParam& ref = (m_config.m_ports[i] = new AudioPortConfig)->m_param;
	
			ref.nChannel = audioParam->nChannel;
			ref.nBlock = audioParam->nBlock;
			ref.attr = audioParam->attr;
			ref.level = audioParam->level;

			portNum = i;
			cellAudio.Warning("*** audio port opened(nChannel=%lld, nBlock=0x%llx, attr=0x%llx, level=%f): port = %d",
				(u64)ref.nChannel, (u64)ref.nBlock, (u64)ref.attr, (float)ref.level, portNum.GetValue());
			//TODO: implementation of ring buffer
			return CELL_OK;
		}
	}

	return CELL_AUDIO_ERROR_PORT_FULL;
}

int cellAudioGetPortConfig(u32 portNum, mem_ptr_t<CellAudioPortConfig> portConfig)
{
	cellAudio.Warning("cellAudioGetPortConfig(portNum=0x%x, portConfig_addr=0x%x)", portNum, portConfig.GetAddr());

	if (!portConfig.IsGood() || portNum >= m_config.m_ports.GetCount()) 
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	if (!m_config.m_ports[portNum])
	{
		portConfig->status = CELL_AUDIO_STATUS_CLOSE;
	}
	else if (m_config.m_ports[portNum]->m_is_audio_port_started)
	{
		portConfig->status = CELL_AUDIO_STATUS_RUN;
	}
	else
	{
		CellAudioPortParam& ref = m_config.m_ports[portNum]->m_param;

		portConfig->status = CELL_AUDIO_STATUS_READY;
		portConfig->nChannel = ref.nChannel;
		portConfig->nBlock = ref.nBlock;
		portConfig->portSize = ref.nChannel * ref.nBlock * 256 * sizeof(float);
		portConfig->portAddr = m_config.m_ports[portNum]->m_buffer; // 0x20020000
		portConfig->readIndexAddr = m_config.m_ports[portNum]->m_index; // 0x20010010 on ps3

		ConLog.Write("*** nChannel=%d, nBlock=%d, portSize=0x%x, portAddr=0x%x, readIndexAddr=0x%x",
			(u32)portConfig->nChannel, (u32)portConfig->nBlock, (u32)portConfig->portSize, (u32)portConfig->portAddr, (u32)portConfig->readIndexAddr);
		// portAddr - readIndexAddr ==  0xFFF0 on ps3
	}

	return CELL_OK;
}

int cellAudioPortStart(u32 portNum)
{
	cellAudio.Warning("cellAudioPortStart(portNum=0x%x)", portNum);

	if (portNum >= m_config.m_ports.GetCount()) 
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	if (!m_config.m_ports[portNum])
	{
		return CELL_AUDIO_ERROR_PORT_OPEN;
	}

	if (m_config.m_ports[portNum]->m_is_audio_port_started)
	{
		return CELL_AUDIO_ERROR_PORT_ALREADY_RUN;
	}
	
	m_config.m_ports[portNum]->m_is_audio_port_started = true;

	std::string t_name = "AudioPort0";
	t_name[9] += portNum;

	thread t(t_name, [portNum]()
		{
			AudioPortConfig& port = *m_config.m_ports[portNum];
			mem64_t index(port.m_index); // index storage

			if (port.m_param.nChannel > 8)
			{
				ConLog.Error("Port aborted: invalid channel count (%d)", port.m_param.nChannel);
				return;
			}

			WAVHeader header(port.m_param.nChannel); // WAV file header

			wxString output_name = "audioport0.wav";
			output_name[9] = '0' + portNum;

			wxFile output(output_name, wxFile::write); // create output file
			if (!output.IsOpened())
			{
				ConLog.Error("Port aborted: cannot write %s", output_name.wx_str());
				return;
			}

			ConLog.Write("Port started");

			u64 start_time = get_system_time();
			u32 counter = 0;

			output.Write(&header, sizeof(header)); // write file header

			const u32 block_size = port.m_param.nChannel * 256 * sizeof(float);

			float buffer[32*256]; // buffer for max channel count (8)

			while (port.m_is_audio_port_started)
			{
				// Sleep(5); // precise time of sleeping: 5,(3) ms (or 256/48000 sec)
				if ((u64)counter * 256000000 / 48000 >= get_system_time() - start_time)
				{
					Sleep(1);
					continue;
				}

				counter++;

				u32 position = index.GetValue(); // get old value
				
				memcpy(buffer, Memory + port.m_buffer + position * block_size, block_size);

				index = (position + 1) % port.m_param.nBlock; // write new value

				for (u32 i = 0; i < block_size; i++)
				{
					// reverse byte order (TODO: use port.m_param.level)
					buffer[i] = re(buffer[i]);
				}

				output.Write(&buffer, block_size); // write file data
				header.Size += block_size; // update file header
				header.RIFF.Size += block_size;
				
				if (Emu.IsStopped())
				{
					ConLog.Write("Port aborted");
					goto abort;
				}
			}
			ConLog.Write("Port finished");
		abort:
			output.Seek(0);
			output.Write(&header, sizeof(header)); // write fixed file header

			output.Close();
		});
	t.detach();

	return CELL_OK;
}

int cellAudioPortClose(u32 portNum)
{
	cellAudio.Warning("cellAudioPortClose(portNum=0x%x)", portNum);

	if (portNum >= m_config.m_ports.GetCount()) 
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	if (!m_config.m_ports[portNum])
	{
		return CELL_AUDIO_ERROR_PORT_NOT_OPEN;
	}

	m_config.m_ports[portNum]->finalize();
	safe_delete(m_config.m_ports[portNum]);
	return CELL_OK;
}

int cellAudioPortStop(u32 portNum)
{
	cellAudio.Warning("cellAudioPortStop(portNum=0x%x)",portNum);
	
	if (portNum >= m_config.m_ports.GetCount()) 
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	if (!m_config.m_ports[portNum])
	{
		return CELL_AUDIO_ERROR_PORT_NOT_OPEN;
	}

	if (!m_config.m_ports[portNum]->m_is_audio_port_started)
	{
		return CELL_AUDIO_ERROR_PORT_NOT_RUN;
	}
	
	m_config.m_ports[portNum]->m_is_audio_port_started = false;	
	return CELL_OK;
}

int cellAudioGetPortTimestamp(u32 portNum, u64 tag, mem64_t stamp)
{
	cellAudio.Error("cellAudioGetPortTimestamp(portNum=0x%x, tag=0x%llx, stamp_addr=0x%x)", portNum, tag, stamp.GetAddr());
	return CELL_OK;
}

int cellAudioGetPortBlockTag(u32 portNum, u64 blockNo, mem64_t tag)
{
	cellAudio.Error("cellAudioGetPortBlockTag(portNum=0x%x, blockNo=0x%llx, tag_addr=0x%x)", portNum, blockNo, tag.GetAddr());
	return CELL_OK;
}

int cellAudioSetPortLevel(u32 portNum, float level)
{
	cellAudio.Error("cellAudioSetPortLevel(portNum=0x%x, level=%f)", portNum, level);
	return CELL_OK;
}

// Utility Functions  
int cellAudioCreateNotifyEventQueue(mem32_t id, mem64_t key)
{
	cellAudio.Error("cellAudioCreateNotifyEventQueue(id_addr=0x%x, key_addr=0x%x)", id.GetAddr(), key.GetAddr());
	key = 0x123456789ABCDEF0;
	return CELL_OK;
}

int cellAudioCreateNotifyEventQueueEx(mem32_t id, mem64_t key, u32 iFlags)
{
	cellAudio.Error("cellAudioCreateNotifyEventQueueEx(id_addr=0x%x, key_addr=0x%x, iFlags=0x%x)", id.GetAddr(), key.GetAddr(), iFlags);
	return CELL_OK;
}

int cellAudioSetNotifyEventQueue(u64 key)
{
	cellAudio.Warning("cellAudioSetNotifyEventQueue(key=0x%llx)", key);

	m_config.event_key = key;

	EventQueue* eq;
	if (!Emu.GetEventManager().GetEventQueue(key, eq))
	{
		//return CELL_AUDIO_ERROR_PARAM;
		return CELL_OK;
	}

	// eq->events.push(0, 0, 0, 0);

	return CELL_OK;
}

int cellAudioSetNotifyEventQueueEx(u64 key, u32 iFlags)
{
	cellAudio.Error("cellAudioSetNotifyEventQueueEx(key=0x%llx, iFlags=0x%x)", key, iFlags);
	return CELL_OK;
}

int cellAudioRemoveNotifyEventQueue(u64 key)
{
	cellAudio.Error("cellAudioRemoveNotifyEventQueue(key=0x%llx)", key);
	return CELL_OK;
}

int cellAudioRemoveNotifyEventQueueEx(u64 key, u32 iFlags)
{
	cellAudio.Error("cellAudioRemoveNotifyEventQueueEx(key=0x%llx, iFlags=0x%x)", key, iFlags);
	return CELL_OK;
}

int cellAudioAddData(u32 portNum, mem32_t src, u32 samples, float volume)
{
	cellAudio.Error("cellAudioAddData(portNum=0x%x, src_addr=0x%x, samples=%d, volume=%f)", portNum, src.GetAddr(), samples, volume);
	return CELL_OK;
}

int cellAudioAdd2chData(u32 portNum, mem32_t src, u32 samples, float volume) 
{
	cellAudio.Error("cellAudioAdd2chData(portNum=0x%x, src_addr=0x%x, samples=%d, volume=%f)", portNum, src.GetAddr(), samples, volume);
	return CELL_OK;
}

int cellAudioAdd6chData(u32 portNum, mem32_t src, float volume)
{
	cellAudio.Error("cellAudioAdd6chData(portNum=0x%x, src_addr=0x%x, volume=%f)", portNum, src.GetAddr(), volume);
	return CELL_OK;
}

int cellAudioMiscSetAccessoryVolume(u32 devNum, float volume)
{
	cellAudio.Error("cellAudioMiscSetAccessoryVolume(devNum=0x%x, volume=%f)", devNum, volume);
	return CELL_OK;
}

int cellAudioSendAck(u64 data3)
{
	cellAudio.Error("cellAudioSendAck(data3=0x%llx)", data3);
	return CELL_OK;
}

int cellAudioSetPersonalDevice(int iPersonalStream, int iDevice)
{
	cellAudio.Error("cellAudioSetPersonalDevice(iPersonalStream=0x%x, iDevice=0x%x)", iPersonalStream, iDevice);
	return CELL_OK;
}

int cellAudioUnsetPersonalDevice(int iPersonalStream)
{
	cellAudio.Error("cellAudioUnsetPersonalDevice(iPersonalStream=0x%x)", iPersonalStream);
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

void cellAudio_load()
{
	m_config.m_is_audio_initialized = false;
	m_config.Clear();
}

void cellAudio_unload()
{
	m_config.m_is_audio_initialized = false;
	m_config.Clear();
}