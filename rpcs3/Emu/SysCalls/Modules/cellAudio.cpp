#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "Emu/Audio/cellAudio.h"

void cellAudio_init();
void cellAudio_unload();
Module cellAudio(0x0011, cellAudio_init, nullptr, cellAudio_unload);

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
	SMutexGeneral m_mutex;
	bool m_is_audio_port_opened;
	bool m_is_audio_port_started;
	u8 channel;
	u8 block;
	float level;
	u64 attr;
	u64 tag;
	u64 counter; // copy of global counter
};

struct AudioConfig  //custom structure
{
	enum
	{
		AUDIO_PORT_COUNT = 8,
	};
	AudioPortConfig m_ports[AUDIO_PORT_COUNT];
	u32 m_buffer; // 1 MB memory for audio ports
	u32 m_indexes; // current block indexes and other info
	bool m_is_audio_initialized;
	bool m_is_audio_finalized;
	u32 m_port_in_use;
	u64 event_key;
	u64 counter;
	u64 start_time;

	AudioConfig()
		: m_is_audio_initialized(false)
		, m_is_audio_finalized(false)
		, m_port_in_use(0)
		, event_key(0)
		, counter(0)
	{
		memset(&m_ports, 0, sizeof(AudioPortConfig) * AUDIO_PORT_COUNT);
	}

	void Clear()
	{
		memset(&m_ports, 0, sizeof(AudioPortConfig) * AUDIO_PORT_COUNT);
		m_port_in_use = 0;
	}
} m_config;

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
	m_config.counter = 0;

	// alloc memory
	m_config.m_buffer = Memory.Alloc(128 * 1024 * m_config.AUDIO_PORT_COUNT, 1024); 
	memset(Memory + m_config.m_buffer, 0, 128 * 1024 * m_config.AUDIO_PORT_COUNT);
	m_config.m_indexes = Memory.Alloc(sizeof(u64) * m_config.AUDIO_PORT_COUNT, 16);
	memset(Memory + m_config.m_indexes, 0, sizeof(u64) * m_config.AUDIO_PORT_COUNT);

	thread t("Audio Thread", []()
		{
			WAVHeader header(2); // WAV file header (stereo)
			
			static const wxString& output_name = "audio.wav";

			wxFile output; // create output file
			if (Ini.AudioDumpToFile.GetValue() && !output.Open(output_name, wxFile::write))
			{
				ConLog.Error("Audio aborted: cannot create %s", output_name.wx_str());
				return;
			}

			ConLog.Write("Audio started");

			m_config.start_time = get_system_time();

			if (Ini.AudioDumpToFile.GetValue())
				output.Write(&header, sizeof(header)); // write file header

			float buffer[2*256]; // buffer for 2 channels
			be_t<float> buffer2[8*256]; // buffer for 8 channels (max count)
			memset(&buffer, 0, sizeof(buffer));
			memset(&buffer2, 0, sizeof(buffer2));

			while (m_config.m_is_audio_initialized)
			{
				if (Emu.IsStopped())
				{
					ConLog.Warning("Audio aborted");
					goto abort;
				}

				// TODO: send beforemix event (in ~2,6 ms before mixing)

				// Sleep(5); // precise time of sleeping: 5,(3) ms (or 256/48000 sec)
				if (m_config.counter * 256000000 / 48000 >= get_system_time() - m_config.start_time)
				{
					Sleep(1);
					continue;
				}

				m_config.counter++;

				if (Emu.IsPaused())
				{
					continue;
				}

				bool first_mix = true;

				// MIX:
				for (u32 i = 0; i < m_config.AUDIO_PORT_COUNT; i++)
				{
					if (!m_config.m_ports[i].m_is_audio_port_started) continue;

					AudioPortConfig& port = m_config.m_ports[i];
					mem64_t index(m_config.m_indexes + i * sizeof(u64));

					const u32 block_size = port.channel * 256;

					u32 position = port.tag % port.block; // old value

					u32 buf_addr = m_config.m_buffer + (i * 128 * 1024) + (position * block_size * sizeof(float));

					memcpy(buffer2, Memory + buf_addr, block_size * sizeof(float));
					memset(Memory + buf_addr, 0, block_size * sizeof(float));

					{
						SMutexGeneralLocker lock(port.m_mutex);
						port.counter = m_config.counter;
						port.tag++; // absolute index of block that will be read
						index = (position + 1) % port.block; // write new value
					}

					if (first_mix)
					{
						for (u32 i = 0; i < (sizeof(buffer) / sizeof(float)); i++)
						{
							// reverse byte order (TODO: use port.m_param.level)
							buffer[i] = buffer2[i];
						}
						first_mix = false;
					}
					else
					{
						for (u32 i = 0; i < (sizeof(buffer) / sizeof(float)); i++)
						{
							buffer[i] = (buffer[i] + buffer2[i]) * 0.5; // TODO: valid mixing
						}
					}
				}

				// send aftermix event (normal audio event)
				// TODO: check event source
				Emu.GetEventManager().SendEvent(m_config.event_key, 0x10103000e010e07, 0, 0, 0);

				if(Ini.AudioDumpToFile.GetValue())
				{
					if (output.Write(&buffer, sizeof(buffer)) != sizeof(buffer)) // write file data
					{
						ConLog.Error("Port aborted: cannot write %s", output_name.wx_str());
						goto abort;
					}

					header.Size += sizeof(buffer); // update file header
					header.RIFF.Size += sizeof(buffer);
				}
			}
			ConLog.Write("Audio finished");
abort:
			if(Ini.AudioDumpToFile.GetValue())
			{
				output.Seek(0);
				output.Write(&header, sizeof(header)); // write fixed file header

				output.Close();
			}

			m_config.m_is_audio_finalized = true;
		});
	t.detach();

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

	while (!m_config.m_is_audio_finalized)
	{
		Sleep(1);
		if (Emu.IsStopped())
		{
			ConLog.Warning("cellAudioQuit() aborted");
			return CELL_OK;
		}
	}

	Memory.Free(m_config.m_buffer);
	Memory.Free(m_config.m_indexes);
	return CELL_OK;
}

int cellAudioPortOpen(mem_ptr_t<CellAudioPortParam> audioParam, mem32_t portNum)
{
	cellAudio.Warning("cellAudioPortOpen(audioParam_addr=0x%x, portNum_addr=0x%x)", audioParam.GetAddr(), portNum.GetAddr());

	if(!audioParam.IsGood() || !portNum.IsGood())
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	if (audioParam->nChannel > 8 || audioParam->nBlock > 16)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	if (m_config.m_port_in_use >= m_config.AUDIO_PORT_COUNT)
	{
		return CELL_AUDIO_ERROR_PORT_FULL;
	}

	for (u32 i = 0; i < m_config.AUDIO_PORT_COUNT; i++)
	{
		if (!m_config.m_ports[i].m_is_audio_port_opened)
		{
			AudioPortConfig& port = m_config.m_ports[i];
	
			port.channel = audioParam->nChannel;
			port.block = audioParam->nBlock;
			port.attr = audioParam->attr;
			port.level = audioParam->level;

			portNum = i;
			cellAudio.Warning("*** audio port opened(nChannel=%d, nBlock=%d, attr=0x%llx, level=%f): port = %d",
				port.channel, port.block, port.attr, port.level, i);
			
			port.m_is_audio_port_opened = true;
			port.m_is_audio_port_started = false;
			port.tag = 0;

			m_config.m_port_in_use++;
			return CELL_OK;
		}
	}

	return CELL_AUDIO_ERROR_PORT_FULL;
}

int cellAudioGetPortConfig(u32 portNum, mem_ptr_t<CellAudioPortConfig> portConfig)
{
	cellAudio.Warning("cellAudioGetPortConfig(portNum=0x%x, portConfig_addr=0x%x)", portNum, portConfig.GetAddr());

	if (!portConfig.IsGood() || portNum >= m_config.AUDIO_PORT_COUNT) 
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	if (!m_config.m_ports[portNum].m_is_audio_port_opened)
	{
		portConfig->status = CELL_AUDIO_STATUS_CLOSE;
	}
	else if (m_config.m_ports[portNum].m_is_audio_port_started)
	{
		portConfig->status = CELL_AUDIO_STATUS_RUN;
	}
	else
	{
		portConfig->status = CELL_AUDIO_STATUS_READY;
	}

	AudioPortConfig& port = m_config.m_ports[portNum];

	portConfig->nChannel = port.channel;
	portConfig->nBlock = port.block;
	portConfig->portSize = port.channel * port.block * 256 * sizeof(float);
	portConfig->portAddr = m_config.m_buffer + (128 * 1024 * portNum); // 0x20020000
	portConfig->readIndexAddr = m_config.m_indexes + (sizeof(u64) * portNum); // 0x20010010 on ps3

	cellAudio.Log("*** port config: nChannel=%d, nBlock=%d, portSize=0x%x, portAddr=0x%x, readIndexAddr=0x%x",
		(u32)portConfig->nChannel, (u32)portConfig->nBlock, (u32)portConfig->portSize, (u32)portConfig->portAddr, (u32)portConfig->readIndexAddr);
	// portAddr - readIndexAddr ==  0xFFF0 on ps3

	return CELL_OK;
}

int cellAudioPortStart(u32 portNum)
{
	cellAudio.Warning("cellAudioPortStart(portNum=0x%x)", portNum);

	if (portNum >= m_config.AUDIO_PORT_COUNT) 
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	if (!m_config.m_ports[portNum].m_is_audio_port_opened)
	{
		return CELL_AUDIO_ERROR_PORT_OPEN;
	}

	if (m_config.m_ports[portNum].m_is_audio_port_started)
	{
		return CELL_AUDIO_ERROR_PORT_ALREADY_RUN;
	}
	
	m_config.m_ports[portNum].m_is_audio_port_started = true;

	return CELL_OK;
}

int cellAudioPortClose(u32 portNum)
{
	cellAudio.Warning("cellAudioPortClose(portNum=0x%x)", portNum);

	if (portNum >= m_config.AUDIO_PORT_COUNT) 
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	if (!m_config.m_ports[portNum].m_is_audio_port_opened)
	{
		return CELL_AUDIO_ERROR_PORT_NOT_OPEN;
	}

	m_config.m_ports[portNum].m_is_audio_port_started = false;
	m_config.m_port_in_use--;
	return CELL_OK;
}

int cellAudioPortStop(u32 portNum)
{
	cellAudio.Warning("cellAudioPortStop(portNum=0x%x)",portNum);
	
	if (portNum >= m_config.AUDIO_PORT_COUNT) 
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	if (!m_config.m_ports[portNum].m_is_audio_port_opened)
	{
		return CELL_AUDIO_ERROR_PORT_NOT_OPEN;
	}

	if (!m_config.m_ports[portNum].m_is_audio_port_started)
	{
		return CELL_AUDIO_ERROR_PORT_NOT_RUN;
	}
	
	m_config.m_ports[portNum].m_is_audio_port_started = false;
	return CELL_OK;
}

int cellAudioGetPortTimestamp(u32 portNum, u64 tag, mem64_t stamp)
{
	cellAudio.Log("cellAudioGetPortTimestamp(portNum=0x%x, tag=0x%llx, stamp_addr=0x%x)", portNum, tag, stamp.GetAddr());

	if (portNum >= m_config.AUDIO_PORT_COUNT) 
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	if (!m_config.m_ports[portNum].m_is_audio_port_opened)
	{
		return CELL_AUDIO_ERROR_PORT_NOT_OPEN;
	}

	if (!m_config.m_ports[portNum].m_is_audio_port_started)
	{
		return CELL_AUDIO_ERROR_PORT_NOT_RUN;
	}

	AudioPortConfig& port = m_config.m_ports[portNum];

	SMutexGeneralLocker lock(port.m_mutex);

	stamp = m_config.start_time + (port.counter + (tag - port.tag)) * 256000000 / 48000;

	return CELL_OK;
}

int cellAudioGetPortBlockTag(u32 portNum, u64 blockNo, mem64_t tag)
{
	cellAudio.Log("cellAudioGetPortBlockTag(portNum=0x%x, blockNo=0x%llx, tag_addr=0x%x)", portNum, blockNo, tag.GetAddr());

	if (portNum >= m_config.AUDIO_PORT_COUNT) 
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	if (!m_config.m_ports[portNum].m_is_audio_port_opened)
	{
		return CELL_AUDIO_ERROR_PORT_NOT_OPEN;
	}

	if (!m_config.m_ports[portNum].m_is_audio_port_started)
	{
		return CELL_AUDIO_ERROR_PORT_NOT_RUN;
	}

	AudioPortConfig& port = m_config.m_ports[portNum];

	if (blockNo >= port.block)
	{
		cellAudio.Error("cellAudioGetPortBlockTag: wrong blockNo(%lld)", blockNo);
		return CELL_AUDIO_ERROR_PARAM;
	}

	SMutexGeneralLocker lock(port.m_mutex);

	u64 tag_base = port.tag;
	if (tag_base % port.block > blockNo)
	{
		tag_base &= ~(port.block-1);
		tag_base += port.block;
	}
	else
	{
		tag_base &= ~(port.block-1);
	}
	tag = tag_base + blockNo;

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
	cellAudio.Warning("cellAudioCreateNotifyEventQueue(id_addr=0x%x, key_addr=0x%x)", id.GetAddr(), key.GetAddr());

	if (Emu.GetEventManager().CheckKey(0x80004d494f323221))
	{
		return CELL_AUDIO_ERROR_EVENT_QUEUE;
	}

	EventQueue* eq = new EventQueue(SYS_SYNC_FIFO, SYS_PPU_QUEUE, 0x80004d494f323221, 0x80004d494f323221, 32);

	if (!Emu.GetEventManager().RegisterKey(eq, 0x80004d494f323221))
	{
		delete eq;
		return CELL_AUDIO_ERROR_EVENT_QUEUE;
	}

	id = cellAudio.GetNewId(eq);
	key = 0x80004d494f323221;

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

	/*EventQueue* eq;
	if (!Emu.GetEventManager().GetEventQueue(key, eq))
	{
		return CELL_AUDIO_ERROR_PARAM;
	}*/

	// TODO: connect port (?????)

	return CELL_OK;
}

int cellAudioSetNotifyEventQueueEx(u64 key, u32 iFlags)
{
	cellAudio.Error("cellAudioSetNotifyEventQueueEx(key=0x%llx, iFlags=0x%x)", key, iFlags);
	return CELL_OK;
}

int cellAudioRemoveNotifyEventQueue(u64 key)
{
	cellAudio.Warning("cellAudioRemoveNotifyEventQueue(key=0x%llx)", key);

	EventQueue* eq;
	if (!Emu.GetEventManager().GetEventQueue(key, eq))
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	m_config.event_key = 0;

	// TODO: disconnect port

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
	//StaticFinalize();
}