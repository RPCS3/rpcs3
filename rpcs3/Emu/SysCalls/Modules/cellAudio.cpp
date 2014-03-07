#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "Emu/Audio/cellAudio.h"
#include "Emu/Audio/AudioManager.h"
#include "Emu/Audio/AudioDumper.h"

void cellAudio_init();
void cellAudio_load();
void cellAudio_unload();
Module cellAudio(0x0011, cellAudio_init, cellAudio_load, cellAudio_unload);

extern u64 get_system_time();

// libaudio Functions

int cellAudioInit()
{
	cellAudio.Warning("cellAudioInit()");

	if(Ini.AudioOutMode.GetValue() == 1)
		m_audio_out->Init();

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
			AudioDumper m_dump(2); // WAV file header (stereo)
		
			if (Ini.AudioDumpToFile.GetValue() && !m_dump.Init())
			{
				ConLog.Error("Audio aborted: cannot create file!");
				return;
			}

			ConLog.Write("Audio started");

			m_config.start_time = get_system_time();

			if (Ini.AudioDumpToFile.GetValue())
				m_dump.WriteHeader();

			float buffer[2*256]; // buffer for 2 channels
			be_t<float> buffer2[8*256]; // buffer for 8 channels (max count)
			u16 oal_buffer[2*256]; // buffer for OpenAL

			memset(&buffer, 0, sizeof(buffer));
			memset(&buffer2, 0, sizeof(buffer2));
			memset(&oal_buffer, 0, sizeof(oal_buffer));

			if(Ini.AudioOutMode.GetValue() == 1)
				m_audio_out->Open(oal_buffer, sizeof(oal_buffer));

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
						SMutexLocker lock(port.m_mutex);
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

							// convert the data from float to u16
							oal_buffer[i] = (u16)((float)buffer[i] * (1 << 16));
						}
						first_mix = false;
					}
					else
					{
						for (u32 i = 0; i < (sizeof(buffer) / sizeof(float)); i++)
						{
							buffer[i] = (buffer[i] + buffer2[i]) * 0.5; // TODO: valid mixing

							// convert the data from float to u16
							oal_buffer[i] = (u16)((float)buffer[i] * (1 << 16));
						}
					}
				}

				// send aftermix event (normal audio event)
				// TODO: check event source
				Emu.GetEventManager().SendEvent(m_config.event_key, 0x10103000e010e07, 0, 0, 0);

				if(Ini.AudioOutMode.GetValue() == 1)
					m_audio_out->AddData(oal_buffer, sizeof(oal_buffer));

				if(Ini.AudioDumpToFile.GetValue())
				{
					if (m_dump.WriteData(&buffer, sizeof(buffer)) != sizeof(buffer)) // write file data
					{
						ConLog.Error("Port aborted: cannot write file!");
						goto abort;
					}

					m_dump.UpdateHeader(sizeof(buffer));
				}
			}
			ConLog.Write("Audio finished");
abort:
			if(Ini.AudioDumpToFile.GetValue())
				m_dump.Finalize();

			m_config.m_is_audio_finalized = true;
			if(Ini.AudioOutMode.GetValue() == 1)
				m_audio_out->Quit();
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
	if(Ini.AudioOutMode.GetValue() == 1)
		m_audio_out->Quit();

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

	if(Ini.AudioOutMode.GetValue() == 1) 
		m_audio_out->Play();

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

	SMutexLocker lock(port.m_mutex);

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

	SMutexLocker lock(port.m_mutex);

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