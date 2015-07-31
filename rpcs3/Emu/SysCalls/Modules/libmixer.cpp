#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/Cell/PPUInstrTable.h"

#include "cellAudio.h"
#include "libmixer.h"

extern Module libmixer;

SurMixerConfig g_surmx;

std::vector<SSPlayer> g_ssp;

s32 cellAANAddData(u32 aan_handle, u32 aan_port, u32 offset, vm::ptr<float> addr, u32 samples)
{
	libmixer.Log("cellAANAddData(aan_handle=0x%x, aan_port=0x%x, offset=0x%x, addr=*0x%x, samples=%d)", aan_handle, aan_port, offset, addr, samples);

	u32 type = aan_port >> 16;
	u32 port = aan_port & 0xffff;

	switch (type)
	{
	case CELL_SURMIXER_CHSTRIP_TYPE1A:
		if (port >= g_surmx.ch_strips_1) type = 0; break;
	case CELL_SURMIXER_CHSTRIP_TYPE2A:
		if (port >= g_surmx.ch_strips_2) type = 0; break;
	case CELL_SURMIXER_CHSTRIP_TYPE6A:
		if (port >= g_surmx.ch_strips_6) type = 0; break;
	case CELL_SURMIXER_CHSTRIP_TYPE8A:
		if (port >= g_surmx.ch_strips_8) type = 0; break;
	default:
		type = 0; break;
	}

	if (aan_handle != 0x11111111 || samples != 256 || !type || offset != 0)
	{
		libmixer.Error("cellAANAddData(aan_handle=0x%x, aan_port=0x%x, offset=0x%x, addr=*0x%x, samples=%d): invalid parameters", aan_handle, aan_port, offset, addr, samples);
		return CELL_LIBMIXER_ERROR_INVALID_PARAMATER;
	}

	std::lock_guard<std::mutex> lock(g_surmx.mutex);

	if (type == CELL_SURMIXER_CHSTRIP_TYPE1A)
	{
		// mono upmixing
		for (u32 i = 0; i < samples; i++)
		{
			const float center = addr[i];
			g_surmx.mixdata[i * 8 + 0] += center;
			g_surmx.mixdata[i * 8 + 1] += center;
		}		
	}
	else if (type == CELL_SURMIXER_CHSTRIP_TYPE2A)
	{
		// stereo upmixing
		for (u32 i = 0; i < samples; i++)
		{
			const float left = addr[i * 2 + 0];
			const float right = addr[i * 2 + 1];
			g_surmx.mixdata[i * 8 + 0] += left;
			g_surmx.mixdata[i * 8 + 1] += right;
		}
	}
	else if (type == CELL_SURMIXER_CHSTRIP_TYPE6A)
	{
		// 5.1 upmixing
		for (u32 i = 0; i < samples; i++)
		{
			const float left = addr[i * 6 + 0];
			const float right = addr[i * 6 + 1];
			const float center = addr[i * 6 + 2];
			const float low_freq = addr[i * 6 + 3];
			const float rear_left = addr[i * 6 + 4];
			const float rear_right = addr[i * 6 + 5];
			g_surmx.mixdata[i * 8 + 0] += left;
			g_surmx.mixdata[i * 8 + 1] += right;
			g_surmx.mixdata[i * 8 + 2] += center;
			g_surmx.mixdata[i * 8 + 3] += low_freq;
			g_surmx.mixdata[i * 8 + 4] += rear_left;
			g_surmx.mixdata[i * 8 + 5] += rear_right;
		}
	}
	else if (type == CELL_SURMIXER_CHSTRIP_TYPE8A)
	{
		// 7.1
		for (u32 i = 0; i < samples * 8; i++)
		{
			g_surmx.mixdata[i] += addr[i];
		}
	}

	return CELL_OK; 
}

s32 cellAANConnect(u32 receive, u32 receivePortNo, u32 source, u32 sourcePortNo)
{
	libmixer.Warning("cellAANConnect(receive=0x%x, receivePortNo=0x%x, source=0x%x, sourcePortNo=0x%x)",
		receive, receivePortNo, source, sourcePortNo);

	std::lock_guard<std::mutex> lock(g_surmx.mutex);

	if (source >= g_ssp.size() || !g_ssp[source].m_created)
	{
		libmixer.Error("cellAANConnect(): invalid source (%d)", source);
		return CELL_LIBMIXER_ERROR_INVALID_PARAMATER;
	}

	g_ssp[source].m_connected = true;

	return CELL_OK;
}

s32 cellAANDisconnect(u32 receive, u32 receivePortNo, u32 source, u32 sourcePortNo)
{
	libmixer.Warning("cellAANDisconnect(receive=0x%x, receivePortNo=0x%x, source=0x%x, sourcePortNo=0x%x)",
		receive, receivePortNo, source, sourcePortNo);

	std::lock_guard<std::mutex> lock(g_surmx.mutex);

	if (source >= g_ssp.size() || !g_ssp[source].m_created)
	{
		libmixer.Error("cellAANDisconnect(): invalid source (%d)", source);
		return CELL_LIBMIXER_ERROR_INVALID_PARAMATER;
	}

	g_ssp[source].m_connected = false;

	return CELL_OK;
}
 
s32 cellSSPlayerCreate(vm::ptr<u32> handle, vm::ptr<CellSSPlayerConfig> config)
{
	libmixer.Warning("cellSSPlayerCreate(handle=*0x%x, config=*0x%x)", handle, config);

	if (config->outputMode != 0 || config->channels - 1 >= 2)
	{
		libmixer.Error("cellSSPlayerCreate(config.outputMode=%d, config.channels=%d): invalid parameters", config->outputMode, config->channels);
		return CELL_LIBMIXER_ERROR_INVALID_PARAMATER;
	}

	std::lock_guard<std::mutex> lock(g_surmx.mutex);

	SSPlayer p;
	p.m_created = true;
	p.m_connected = false;
	p.m_active = false;
	p.m_channels = config->channels;
	
	g_ssp.push_back(p);
	*handle = (u32)g_ssp.size() - 1;
	return CELL_OK;
}

s32 cellSSPlayerRemove(u32 handle)
{
	libmixer.Warning("cellSSPlayerRemove(handle=0x%x)", handle);

	std::lock_guard<std::mutex> lock(g_surmx.mutex);

	if (handle >= g_ssp.size() || !g_ssp[handle].m_created)
	{
		libmixer.Error("cellSSPlayerRemove(): SSPlayer not found (%d)", handle);
		return CELL_LIBMIXER_ERROR_INVALID_PARAMATER;
	}

	g_ssp[handle].m_active = false;
	g_ssp[handle].m_created = false;
	g_ssp[handle].m_connected = false;

	return CELL_OK;
}

s32 cellSSPlayerSetWave(u32 handle, vm::ptr<CellSSPlayerWaveParam> waveInfo, vm::ptr<CellSSPlayerCommonParam> commonInfo)
{
	libmixer.Warning("cellSSPlayerSetWave(handle=0x%x, waveInfo=*0x%x, commonInfo=*0x%x)", handle, waveInfo, commonInfo);

	std::lock_guard<std::mutex> lock(g_surmx.mutex);

	if (handle >= g_ssp.size() || !g_ssp[handle].m_created)
	{
		libmixer.Error("cellSSPlayerSetWave(): SSPlayer not found (%d)", handle);
		return CELL_LIBMIXER_ERROR_INVALID_PARAMATER;
	}

	// TODO: check parameters

	g_ssp[handle].m_addr = waveInfo->addr;
	g_ssp[handle].m_samples = waveInfo->samples;
	g_ssp[handle].m_loop_start = waveInfo->loopStartOffset - 1;
	g_ssp[handle].m_loop_mode = commonInfo ? (u32)commonInfo->loopMode : CELL_SSPLAYER_ONESHOT;
	g_ssp[handle].m_position = waveInfo->startOffset - 1;

	return CELL_OK;
}

s32 cellSSPlayerPlay(u32 handle, vm::ptr<CellSSPlayerRuntimeInfo> info)
{
	libmixer.Warning("cellSSPlayerPlay(handle=0x%x, info=*0x%x)", handle, info);

	std::lock_guard<std::mutex> lock(g_surmx.mutex);

	if (handle >= g_ssp.size() || !g_ssp[handle].m_created)
	{
		libmixer.Error("cellSSPlayerPlay(): SSPlayer not found (%d)", handle);
		return CELL_LIBMIXER_ERROR_INVALID_PARAMATER;
	}

	// TODO: check parameters

	g_ssp[handle].m_active = true;
	g_ssp[handle].m_level = info->level;
	g_ssp[handle].m_speed = info->speed;
	g_ssp[handle].m_x = info->position.x;
	g_ssp[handle].m_y = info->position.y;
	g_ssp[handle].m_z = info->position.z;

	return CELL_OK;
}

s32 cellSSPlayerStop(u32 handle, u32 mode)
{
	libmixer.Warning("cellSSPlayerStop(handle=0x%x, mode=0x%x)", handle, mode);

	std::lock_guard<std::mutex> lock(g_surmx.mutex);

	if (handle >= g_ssp.size() || !g_ssp[handle].m_created)
	{
		libmixer.Error("cellSSPlayerStop(): SSPlayer not found (%d)", handle);
		return CELL_LIBMIXER_ERROR_INVALID_PARAMATER;
	}

	// TODO: transition to stop state

	g_ssp[handle].m_active = false;

	return CELL_OK;
}

s32 cellSSPlayerSetParam(u32 handle, vm::ptr<CellSSPlayerRuntimeInfo> info)
{
	libmixer.Warning("cellSSPlayerSetParam(handle=0x%x, info=*0x%x)", handle, info);

	std::lock_guard<std::mutex> lock(g_surmx.mutex);

	if (handle >= g_ssp.size() || !g_ssp[handle].m_created)
	{
		libmixer.Error("cellSSPlayerSetParam(): SSPlayer not found (%d)", handle);
		return CELL_LIBMIXER_ERROR_INVALID_PARAMATER;
	}

	// TODO: check parameters
	
	g_ssp[handle].m_level = info->level;
	g_ssp[handle].m_speed = info->speed;
	g_ssp[handle].m_x = info->position.x;
	g_ssp[handle].m_y = info->position.y;
	g_ssp[handle].m_z = info->position.z;

	return CELL_OK;
}
 
s32 cellSSPlayerGetState(u32 handle)
{
	libmixer.Warning("cellSSPlayerGetState(handle=0x%x)", handle);

	std::lock_guard<std::mutex> lock(g_surmx.mutex);

	if (handle >= g_ssp.size() || !g_ssp[handle].m_created)
	{
		libmixer.Warning("cellSSPlayerGetState(): SSPlayer not found (%d)", handle);
		return CELL_SSPLAYER_STATE_ERROR;
	}

	if (g_ssp[handle].m_active)
	{
		return CELL_SSPLAYER_STATE_ON;
	}

	return CELL_SSPLAYER_STATE_OFF;
}

s32 cellSurMixerCreate(vm::cptr<CellSurMixerConfig> config)
{
	libmixer.Warning("cellSurMixerCreate(config=*0x%x)", config);

	g_surmx.audio_port = g_audio.open_port();

	if (!~g_surmx.audio_port)
	{
		return CELL_LIBMIXER_ERROR_FULL;
	}

	g_surmx.priority = config->priority;
	g_surmx.ch_strips_1 = config->chStrips1;
	g_surmx.ch_strips_2 = config->chStrips2;
	g_surmx.ch_strips_6 = config->chStrips6;
	g_surmx.ch_strips_8 = config->chStrips8;

	AudioPortConfig& port = g_audio.ports[g_surmx.audio_port];

	port.channel = 8;
	port.block = 16;
	port.attr = 0;
	port.addr = g_audio.buffer + AUDIO_PORT_OFFSET * g_surmx.audio_port;
	port.read_index_addr = g_audio.indexes + sizeof(u64) * g_surmx.audio_port;
	port.size = port.channel * port.block * AUDIO_SAMPLES * sizeof(float);
	port.tag = 0;
	port.level = 1.0f;
	port.level_set.data = { 1.0f, 0.0f };

	libmixer.Warning("*** audio port opened (port=%d)", g_surmx.audio_port);

	g_surmx.mixcount = 0;
	g_surmx.cb = vm::null;

	g_ssp.clear();

	libmixer.Warning("*** surMixer created (ch1=%d, ch2=%d, ch6=%d, ch8=%d)", config->chStrips1, config->chStrips2, config->chStrips6, config->chStrips8);

	const auto ppu = Emu.GetIdManager().make_ptr<PPUThread>("Surmixer Thread");
	ppu->prio = 1001;
	ppu->stack_size = 0x10000;
	ppu->custom_task = [](PPUThread& ppu)
	{
		AudioPortConfig& port = g_audio.ports[g_surmx.audio_port];

		while (port.state.load() != AUDIO_PORT_STATE_CLOSED)
		{
			CHECK_EMU_STATUS;

			if (g_surmx.mixcount > (port.tag + 0)) // adding positive value (1-15): preemptive buffer filling (hack)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
				continue;
			}

			if (port.state.load() == AUDIO_PORT_STATE_STARTED)
			{
				//u64 stamp0 = get_system_time();

				memset(g_surmx.mixdata, 0, sizeof(g_surmx.mixdata));
				if (g_surmx.cb)
				{
					g_surmx.cb(ppu, g_surmx.cb_arg, (u32)g_surmx.mixcount, 256);
				}

				//u64 stamp1 = get_system_time();

				{
					std::lock_guard<std::mutex> lock(g_surmx.mutex);

					for (auto& p : g_ssp) if (p.m_active && p.m_created)
					{
						auto v = vm::ptrl<s16>::make(p.m_addr); // 16-bit LE audio data
						float left = 0.0f;
						float right = 0.0f;
						float speed = fabs(p.m_speed);
						float fpos = 0.0f;
						for (s32 i = 0; i < 256; i++) if (p.m_active)
						{
							u32 pos = p.m_position;
							s32 pos_inc = 0;
							if (p.m_speed > 0.0f) // select direction
							{
								pos_inc = 1;
							}
							else if (p.m_speed < 0.0f)
							{
								pos_inc = -1;
							}
							s32 shift = i - (int)fpos; // change playback speed (simple and rough)
							if (shift > 0)
							{
								// slow playback
								pos_inc = 0; // duplicate one sample at this time
								fpos += 1.0f;
								fpos += speed;
							}
							else if (shift < 0)
							{
								// fast playback
								i--; // mix two sample into one at this time
								fpos -= 1.0f;
							}
							else
							{
								fpos += speed;
							}
							p.m_position += (u32)pos_inc;
							if (p.m_channels == 1) // get mono data
							{
								left = right = (float)v[pos] / 0x8000 * p.m_level;
							}
							else if (p.m_channels == 2) // get stereo data
							{
								left = (float)v[pos * 2 + 0] / 0x8000 * p.m_level;
								right = (float)v[pos * 2 + 1] / 0x8000 * p.m_level;
							}
							if (p.m_connected) // mix
							{
								// TODO: m_x, m_y, m_z ignored
								g_surmx.mixdata[i * 8 + 0] += left;
								g_surmx.mixdata[i * 8 + 1] += right;
							}
							if ((p.m_position == p.m_samples && p.m_speed > 0.0f) ||
								(p.m_position == ~0 && p.m_speed < 0.0f)) // loop or stop
							{
								if (p.m_loop_mode == CELL_SSPLAYER_LOOP_ON)
								{
									p.m_position = p.m_loop_start;
								}
								else if (p.m_loop_mode == CELL_SSPLAYER_ONESHOT_CONT)
								{
									p.m_position -= (u32)pos_inc; // restore position
								}
								else // oneshot
								{
									p.m_active = false;
									p.m_position = p.m_loop_start; // TODO: check value
								}
							}
						}
					}
				}

				//u64 stamp2 = get_system_time();

				auto buf = vm::get_ptr<be_t<float>>(port.addr + (g_surmx.mixcount % port.block) * port.channel * AUDIO_SAMPLES * sizeof(float));

				for (auto& mixdata : g_surmx.mixdata)
				{
					// reverse byte order
					*buf++ = mixdata;
				}

				//u64 stamp3 = get_system_time();

				//ConLog.Write("Libmixer perf: start=%lld (cb=%lld, ssp=%lld, finalize=%lld)", stamp0 - m_config.start_time, stamp1 - stamp0, stamp2 - stamp1, stamp3 - stamp2);
			}

			g_surmx.mixcount++;
		}

		Emu.GetIdManager().remove<PPUThread>(ppu.get_id());
	};

	ppu->run();
	ppu->exec();

	return CELL_OK;
}

s32 cellSurMixerGetAANHandle(vm::ptr<u32> handle)
{
	libmixer.Warning("cellSurMixerGetAANHandle(handle=*0x%x) -> %d", handle, 0x11111111);
	*handle = 0x11111111;
	return CELL_OK;
}

s32 cellSurMixerChStripGetAANPortNo(vm::ptr<u32> port, u32 type, u32 index)
{
	libmixer.Warning("cellSurMixerChStripGetAANPortNo(port=*0x%x, type=0x%x, index=0x%x) -> 0x%x", port, type, index, (type << 16) | index);
	*port = (type << 16) | index;
	return CELL_OK;
}

s32 cellSurMixerSetNotifyCallback(vm::ptr<CellSurMixerNotifyCallbackFunction> func, vm::ptr<void> arg)
{
	libmixer.Warning("cellSurMixerSetNotifyCallback(func=*0x%x, arg=*0x%x)", func, arg);

	if (g_surmx.cb)
	{
		throw EXCEPTION("Callback already set");
	}

	g_surmx.cb = func;
	g_surmx.cb_arg = arg;

	return CELL_OK;
}

s32 cellSurMixerRemoveNotifyCallback(vm::ptr<CellSurMixerNotifyCallbackFunction> func)
{
	libmixer.Warning("cellSurMixerRemoveNotifyCallback(func=*0x%x)", func);

	if (g_surmx.cb != func)
	{
		throw EXCEPTION("Callback not set");
	}

	g_surmx.cb = vm::null;

	return CELL_OK;
}

s32 cellSurMixerStart()
{
	libmixer.Warning("cellSurMixerStart()");

	if (g_surmx.audio_port >= AUDIO_PORT_COUNT)
	{
		return CELL_LIBMIXER_ERROR_NOT_INITIALIZED;
	}

	g_audio.ports[g_surmx.audio_port].state.compare_and_swap(AUDIO_PORT_STATE_OPENED, AUDIO_PORT_STATE_STARTED);

	return CELL_OK;
}

s32 cellSurMixerSetParameter(u32 param, float value)
{
	libmixer.Todo("cellSurMixerSetParameter(param=0x%x, value=%f)", param, value);
	return CELL_OK;
}

s32 cellSurMixerFinalize()
{
	libmixer.Warning("cellSurMixerFinalize()");

	if (g_surmx.audio_port >= AUDIO_PORT_COUNT)
	{
		return CELL_LIBMIXER_ERROR_NOT_INITIALIZED;
	}

	g_audio.ports[g_surmx.audio_port].state.compare_and_swap(AUDIO_PORT_STATE_OPENED, AUDIO_PORT_STATE_CLOSED);

	return CELL_OK;
}

s32 cellSurMixerSurBusAddData(u32 busNo, u32 offset, vm::ptr<float> addr, u32 samples)
{
	if (busNo < 8 && samples == 256 && offset == 0)
	{
		libmixer.Log("cellSurMixerSurBusAddData(busNo=%d, offset=0x%x, addr=0x%x, samples=%d)", busNo, offset, addr, samples);
	}
	else
	{
		libmixer.Todo("cellSurMixerSurBusAddData(busNo=%d, offset=0x%x, addr=0x%x, samples=%d)", busNo, offset, addr, samples);
		return CELL_OK;
	}

	std::lock_guard<std::mutex> lock(g_surmx.mutex);

	for (u32 i = 0; i < samples; i++)
	{
		// reverse byte order and mix
		g_surmx.mixdata[i * 8 + busNo] += addr[i];
	}

	return CELL_OK;
}

s32 cellSurMixerChStripSetParameter(u32 type, u32 index, vm::ptr<CellSurMixerChStripParam> param)
{
	libmixer.Todo("cellSurMixerChStripSetParameter(type=%d, index=%d, param=*0x%x)", type, index, param);
	return CELL_OK;
}

s32 cellSurMixerPause(u32 type)
{
	libmixer.Warning("cellSurMixerPause(type=%d)", type);

	if (g_surmx.audio_port >= AUDIO_PORT_COUNT)
	{
		return CELL_LIBMIXER_ERROR_NOT_INITIALIZED;
	}

	g_audio.ports[g_surmx.audio_port].state.compare_and_swap(AUDIO_PORT_STATE_STARTED, AUDIO_PORT_STATE_OPENED);

	return CELL_OK;
}

s32 cellSurMixerGetCurrentBlockTag(vm::ptr<u64> tag)
{
	libmixer.Log("cellSurMixerGetCurrentBlockTag(tag=*0x%x)", tag);

	*tag = g_surmx.mixcount;
	return CELL_OK;
}

s32 cellSurMixerGetTimestamp(u64 tag, vm::ptr<u64> stamp)
{
	libmixer.Log("cellSurMixerGetTimestamp(tag=0x%llx, stamp=*0x%x)", tag, stamp);

	*stamp = g_audio.start_time + (tag) * 256000000 / 48000; // ???
	return CELL_OK;
}

void cellSurMixerBeep(u32 arg)
{
	libmixer.Todo("cellSurMixerBeep(arg=%d)", arg);
	return;
}

float cellSurMixerUtilGetLevelFromDB(float dB)
{
	libmixer.Todo("cellSurMixerUtilGetLevelFromDB(dB=%f)", dB);
	throw EXCEPTION("");
}

float cellSurMixerUtilGetLevelFromDBIndex(s32 index)
{
	libmixer.Todo("cellSurMixerUtilGetLevelFromDBIndex(index=%d)", index);
	throw EXCEPTION("");
}

float cellSurMixerUtilNoteToRatio(u8 refNote, u8 note)
{
	libmixer.Todo("cellSurMixerUtilNoteToRatio(refNote=%d, note=%d)", refNote, note);
	throw EXCEPTION("");
}

Module libmixer("libmixer", []()
{
	g_surmx.audio_port = ~0;

	libmixer.on_stop = []()
	{
		g_ssp.clear();
	};

	using namespace PPU_instr;

	REG_SUB(libmixer,, cellAANAddData,
		{ SPET_MASKED_OPCODE, 0x7c691b78, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0802a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf821ff91, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf8010080, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c802378, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7caa2b78, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x81690000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c050378, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7cc43378, 0xffffffff },
		{ SPET_OPTIONAL_MASKED_OPCODE, 0x78630020, 0xffffffff }, // clrldi r3,r3,32
		{ SPET_MASKED_OPCODE, 0x7d465378, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x812b0030, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x80090000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf8410028, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0903a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x80490004, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4e800421, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xe8410028, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xe8010080, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c6307b4, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0803a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x38210070, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4e800020, 0xffffffff },
	);

	REG_SUB(libmixer,, cellAANConnect,
		{ SPET_MASKED_OPCODE, 0xf821ff71, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0802a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2f830000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf80100a0, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x3c008031, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c691b78, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c8a2378, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x60000003, 0xffffffff },
		SP_LABEL_BR(BNE(cr7, XXX), 0x24),
		SET_LABEL(0x24),
		{ SPET_MASKED_OPCODE, 0x7c0307b4, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xe80100a0, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x38210090, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0803a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4e800020, 0xffffffff },
		SET_LABEL(0x38),
		{ SPET_MASKED_OPCODE, 0x2f850000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x78630020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x38810070, 0xffffffff },
		SP_LABEL_BR(BEQ(cr7, XXX), 0x38),
		{ SPET_MASKED_OPCODE, 0x81690000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x38000001, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x91210074, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x90a10070, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x90c10078, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x9141007c, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x812b0018, 0xffffffff }, //
		{ SPET_MASKED_OPCODE, 0x90010080, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x80090000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf8410028, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0903a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x80490004, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4e800421, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xe8410028, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c601b78, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0307b4, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xe80100a0, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x38210090, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0803a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4e800020, 0xffffffff },
	);

	REG_SUB(libmixer,, cellAANDisconnect,
		{ SPET_MASKED_OPCODE, 0xf821ff71, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0802a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2f830000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf80100a0, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x3c008031, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c691b78, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c8a2378, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x60000003, 0xffffffff },
		SP_LABEL_BR(BNE(cr7, XXX), 0x24),
		SET_LABEL(0x24),
		{ SPET_MASKED_OPCODE, 0x7c0307b4, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xe80100a0, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x38210090, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0803a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4e800020, 0xffffffff },
		SET_LABEL(0x38),
		{ SPET_MASKED_OPCODE, 0x2f850000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x78630020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x38810070, 0xffffffff },
		SP_LABEL_BR(BEQ(cr7, XXX), 0x38),
		{ SPET_MASKED_OPCODE, 0x81690000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x38000001, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x91210074, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x90a10070, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x90c10078, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x9141007c, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x812b001c, 0xffffffff }, //
		{ SPET_MASKED_OPCODE, 0x90010080, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x80090000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf8410028, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0903a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x80490004, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4e800421, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xe8410028, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c601b78, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0307b4, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xe80100a0, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x38210090, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0803a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4e800020, 0xffffffff },
	);

	REG_SUB(libmixer,, cellSurMixerCreate,
		{ SPET_MASKED_OPCODE, 0x2f830000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0802a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf821ff51, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfbc100a0, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfb210078, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfb410080, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfb610088, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfb810090, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfba10098, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfbe100a8, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf80100c0, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c7e1b78, 0xffffffff },
		SP_LABEL_BR(BNE(cr7, XXX), 0x6c),
		{ SPET_MASKED_OPCODE, 0x3fe08031, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x63ff0003, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xe80100c0, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7fe307b4, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xeb210078, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xeb410080, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0803a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xeb610088, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xeb810090, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xeba10098, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xebc100a0, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xebe100a8, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x382100b0, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4e800020, 0xffffffff },
		SET_LABEL(0x6c),
	);

	REG_SUB(libmixer,, cellSurMixerGetAANHandle,
		SP_I(LWZ(r10, r2, XXX)),
		{ SPET_MASKED_OPCODE, 0x3d607fce, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x616bfffe, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x812a0018, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7d2afe70, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x91230000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7d404a78, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c005050, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c00fe70, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c035838, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x3c638031, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x38630002, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c6307b4, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4e800020, 0xffffffff },
	);

	REG_SUB(libmixer,, cellSurMixerChStripGetAANPortNo,
		SP_I(LWZ(r9, r2, XXX)),
		{ SPET_MASKED_OPCODE, 0x7c661b78, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x3c608031, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x78c60020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x78840020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x60630002, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x80090018, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x78a50020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2f800000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4d9e0020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x78030020, 0xffffffff },
		SP_I(B(XXX, 0, 0)),
	);

	REG_SUB(libmixer,, cellSurMixerSetNotifyCallback,
		SP_I(LWZ(r10, r2, XXX)),
		{ SPET_MASKED_OPCODE, 0x7c0802a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf821ff81, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf8010090, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c6b1b78, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x3c608031, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x812a0018, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c882378, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x60630003, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2f890000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2f0b0000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x409e0000, 0xffffff00 }, // bne
		{ SPET_MASKED_OPCODE, 0x3c608031, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x60630002, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xe8010090, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c6307b4, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x38210080, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0803a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4e800020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x419aff00, 0xffffff00 }, // beq
		SP_I(LWZ(r0, r10, XXX)),
		{ SPET_MASKED_OPCODE, 0x79290020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x38810070, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2f800000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7d234b78, 0xffffffff }
	);

	REG_SUB(libmixer,, cellSurMixerRemoveNotifyCallback,
		SP_I(LWZ(r11, r2, XXX)),
		{ SPET_MASKED_OPCODE, 0x7c0802a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf821ff81, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf8010090, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c6a1b78, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x3d208031, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x806b0018, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x61290002, 0xffffffff }, // ori
		{ SPET_MASKED_OPCODE, 0x2f830000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x409e0000, 0xffff0000 }, // bne
		{ SPET_MASKED_OPCODE, 0xe8010090, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7d2307b4, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x38210080, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0803a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4e800020, 0xffffffff }
	);

	REG_SUB(libmixer,, cellSurMixerStart,
		{ SPET_MASKED_OPCODE, 0xf821ff71, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0802a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfbc10080, 0xffffffff },
		SP_I(LWZ(r30, r2, XXX)),
		{ SPET_MASKED_OPCODE, 0xf80100a0, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfba10078, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfbe10088, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x801e0018, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2f800000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x40000000, 0xf0000000 }, // bne
		{ SPET_MASKED_OPCODE, 0x3fe08031, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x63ff0002, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xe80100a0, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7fe307b4, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xeba10078, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xebc10080, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0803a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xebe10088, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x38210090, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4e800020, 0xffffffff }
	);

	REG_SUB(libmixer,, cellSurMixerSetParameter,
		{ SPET_MASKED_OPCODE, 0xf821ff81, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0802a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfbc10070, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfc000890, 0xffffffff },
		SP_I(LWZ(r30, r2, XXX)),
		{ SPET_MASKED_OPCODE, 0x3d208031, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf8010090, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfbe10078, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x61290002, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c7f1b78, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x801e0018, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2f800000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x409e0000, 0xffff0000 }, // bne
		{ SPET_MASKED_OPCODE, 0xe8010090, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7d2307b4, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xebc10070, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xebe10078, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0803a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x38210080, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4e800020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x801e001c, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2b03001f, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2f800000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x419c0000, 0xffff0000 }, // blt
		{ SPET_MASKED_OPCODE, 0x2b83002b, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x40990000, 0xffff0000 }, // ble
		{ SPET_MASKED_OPCODE, 0x409d0000, 0xffff0000 }, // ble
	);

	REG_SUB(libmixer,, cellSurMixerFinalize,
		{ SPET_MASKED_OPCODE, 0xf821ff91, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0802a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf8010080, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4bfffd00, 0xffffff00 }, // bl
		{ SPET_MASKED_OPCODE, 0xe8010080, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x38600000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x38210070, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0803a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4e800020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf821ff71, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0802a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfba10078, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x80000000, 0xf0000000 }, // lwz
		{ SPET_MASKED_OPCODE, 0xf80100a0, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x817d0018, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7d635b78, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x812b0000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x81490000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x800a0000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf8410028, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0903a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x804a0004, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4e800421, 0xffffffff }
	);

	REG_SUB(libmixer,, cellSurMixerSurBusAddData,
		SP_I(LWZ(r10, r2, XXX)),
		{ SPET_MASKED_OPCODE, 0x7c0802a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf821ff91, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf8010080, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c601b78, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x3d208031, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x806a0018, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c8b2378, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7cc73378, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2f830000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x61290002, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x409e0000, 0xffff0000 }, // bne
		{ SPET_MASKED_OPCODE, 0xe8010080, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7d2307b4, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x38210070, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0803a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4e800020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x78a40020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x78050020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x800a001c, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x78680020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2f800000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7d034378, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x79660020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x78e70020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x419c0000, 0xffff0000 } // blt
	);

	REG_SUB(libmixer,, cellSurMixerChStripSetParameter,
		SP_I(LWZ(r8, r2, XXX)),
		{ SPET_MASKED_OPCODE, 0x7c6b1b78, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x3c608031, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c8a2378, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7ca62b78, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x60630002, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x81280018, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2f890000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4d9e0000, 0xffff0000 }, // beqlr
		{ SPET_MASKED_OPCODE, 0x8008001c, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x79640020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x79450020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2f800000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x78c60020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4d9c0020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x79230020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x40000000, 0xf0000000 } // b
	);

	REG_SUB(libmixer,, cellSurMixerPause,
		SP_I(LWZ(r10, r2, XXX)),
		{ SPET_MASKED_OPCODE, 0x7c0802a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf821ff81, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf8010090, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x3d208031, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfbe10078, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x800a0018, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c7f1b78, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfbc10070, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2f800000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x61290002, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x409e0000, 0xffff0000 }, // bne
		{ SPET_MASKED_OPCODE, 0xe8010090, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7d2307b4, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xebc10070, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xebe10078, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0803a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x38210080, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4e800020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x800a001c, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2b030002, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2f800000, 0xffffffff }
	);

	REG_SUB(libmixer,, cellSurMixerGetCurrentBlockTag,
		SP_I(LWZ(r11, r2, XXX)),
		{ SPET_MASKED_OPCODE, 0x3d208031, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x61290002, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x880b0020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2f800000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x419e0000, 0xffff0000 }, // beq
		{ SPET_MASKED_OPCODE, 0xe80b0028, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x39200000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf8030000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7d2307b4, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4e800020, 0xffffffff }
	);

	REG_SUB(libmixer,, cellSurMixerGetTimestamp,
		SP_I(LWZ(r11, r2, XXX)),
		{ SPET_MASKED_OPCODE, 0x7c0802a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf821ff91, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf8010080, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c852378, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x3d208031, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x880b0020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c641b78, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x78a50020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2f800000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x61290002, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x40de0000, 0xffff0000 }, // bne-
		{ SPET_MASKED_OPCODE, 0xe8010080, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7d2307b4, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x38210070, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0803a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4e800020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x806b04d8, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x40000000, 0xf0000000 } // bl
	);

	REG_SUB(libmixer,, cellSurMixerBeep,
		SP_I(LWZ(r9, r2, XXX)),
		{ SPET_MASKED_OPCODE, 0x7c641b78, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x80690018, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2f830000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4d9e0000, 0xffff0000 }, // beqlr
		{ SPET_MASKED_OPCODE, 0x8009001c, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x78630020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x78840020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2f800000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4d9c0020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x40000000, 0xf0000000 } // b
	);

	REG_SUB(libmixer,, cellSSPlayerCreate,
		{ SPET_MASKED_OPCODE, 0xf821ff51, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0802a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2f840000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf80100c0, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x3c008031, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfb210078, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfb410080, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfb610088, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfb810090, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfba10098, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfbc100a0, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfbe100a8, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c9a2378, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c791b78, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x60000003, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x419e0000, 0xffff0000 }, // beq
		{ SPET_MASKED_OPCODE, 0x83000000, 0xff000000 }, // lwz
		{ SPET_MASKED_OPCODE, 0x3b800000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x381b0064, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x901b0018, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x5780103a, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x38800010, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0007b4, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x38a01c70, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7fc0da14, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x38c00000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x83be0024, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2f9d0000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7ba30020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x41de0000, 0xffff0000 }, // beq-
		{ SPET_MASKED_OPCODE, 0x40000000, 0xf0000000 } // bl
	);

	REG_SUB(libmixer,, cellSSPlayerRemove,
		{ SPET_MASKED_OPCODE, 0x7c641b78, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0802a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x3c608031, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2f840000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf821ff51, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfb010070, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfb210078, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfb410080, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfb610088, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfb810090, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfba10098, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfbc100a0, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfbe100a8, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf80100c0, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x60630003, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x419e0000, 0xffff0000 }, // beq
		{ SPET_MASKED_OPCODE, 0x81240000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x78830020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x83440004, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x83240008, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7b5b0020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x81690000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x800b0000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf8410028, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0903a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x804b0004, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4e800421, 0xffffffff }
	);

	REG_SUB(libmixer,, cellSSPlayerSetWave,
		{ SPET_MASKED_OPCODE, 0x7c601b78, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x78840020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2f800000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x3c608031, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x78a50020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x419e0000, 0xffff0000 }, // beq
		{ SPET_MASKED_OPCODE, 0x78030020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x40000000, 0xf0000000 }, // b
		{ SPET_MASKED_OPCODE, 0x60630003, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4e800020, 0xffffffff }
	);

	REG_SUB(libmixer,, cellSSPlayerPlay,
		{ SPET_MASKED_OPCODE, 0x7c601b78, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x3c608031, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2f800000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x60630003, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x78840020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4d9e0020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x78030020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x40000000, 0xf0000000 }, // b
		{ SPET_MASKED_OPCODE, 0xf821ff81, 0xffffffff }, // next func
		{ SPET_MASKED_OPCODE, 0x7c0802a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfbe10078, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c7f1b78, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x81000000, 0xff000000 }, // lwz
		{ SPET_MASKED_OPCODE, 0xf8010090, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x39400000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x38630010, 0xffffffff }
	);

	REG_SUB(libmixer,, cellSSPlayerStop,
		{ SPET_MASKED_OPCODE, 0xf821ff91, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0802a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2f830000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf8010080, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x3c008031, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x78630020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x60000003, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x419e0000, 0xffff0000 }, // beq
		{ SPET_MASKED_OPCODE, 0x78840020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x40000000, 0xf0000000 }, // bl
		{ SPET_MASKED_OPCODE, 0x38000000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0307b4, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xe8010080, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x38210070, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c0803a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4e800020, 0xffffffff }
	);

	REG_SUB(libmixer,, cellSSPlayerSetParam,
		{ SPET_MASKED_OPCODE, 0x7c601b78, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x3c608031, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2f800000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x60630003, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x78840020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4d9e0020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x78030020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x40000000, 0xf0000000 }, // b
		{ SPET_MASKED_OPCODE, 0xf821ff71, 0xffffffff }, // next func
		{ SPET_MASKED_OPCODE, 0x7c0802a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x3d608031, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf80100a0, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x80030068, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x616b0002, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xfbc10080, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2f800000, 0xffffffff }
	);

	REG_SUB(libmixer,, cellSSPlayerGetState,
		{ SPET_MASKED_OPCODE, 0x7c601b78, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x3c608031, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2f800000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x60630003, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4d9e0020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x78030020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x40000000, 0xf0000000 }, // b
	);

	REG_SUB(libmixer,, cellSurMixerUtilGetLevelFromDB);
	REG_SUB(libmixer,, cellSurMixerUtilGetLevelFromDBIndex);
	REG_SUB(libmixer,, cellSurMixerUtilNoteToRatio);
});
