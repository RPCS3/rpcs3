#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_sync.h"

#include "cellAudio.h"
#include "libmixer.h"

#include <cmath>
#include <mutex>

LOG_CHANNEL(libmixer);

template<>
void fmt_class_string<CellLibmixerError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_LIBMIXER_ERROR_NOT_INITIALIZED);
			STR_CASE(CELL_LIBMIXER_ERROR_INVALID_PARAMATER);
			STR_CASE(CELL_LIBMIXER_ERROR_NO_MEMORY);
			STR_CASE(CELL_LIBMIXER_ERROR_ALREADY_EXIST);
			STR_CASE(CELL_LIBMIXER_ERROR_FULL);
			STR_CASE(CELL_LIBMIXER_ERROR_NOT_EXIST);
			STR_CASE(CELL_LIBMIXER_ERROR_TYPE_MISMATCH);
			STR_CASE(CELL_LIBMIXER_ERROR_NOT_FOUND);
		}

		return unknown;
	});
}

struct SurMixerConfig
{
	std::mutex mutex;

	u32 audio_port;
	s32 priority;
	u32 ch_strips_1;
	u32 ch_strips_2;
	u32 ch_strips_6;
	u32 ch_strips_8;

	vm::ptr<CellSurMixerNotifyCallbackFunction> cb;
	vm::ptr<void> cb_arg;

	f32 mixdata[8 * 256];
	u64 mixcount;
};

struct SSPlayer
{
	bool m_created; // SSPlayerCreate/Remove
	bool m_connected; // AANConnect/Disconnect
	bool m_active; // SSPlayerPlay/Stop
	u32 m_channels; // 1 or 2
	u32 m_addr;
	u32 m_samples;
	u32 m_loop_start;
	u32 m_loop_mode;
	u32 m_position;
	float m_level;
	float m_speed;
	float m_x;
	float m_y;
	float m_z;
};

// TODO: use fxm
SurMixerConfig g_surmx;

std::vector<SSPlayer> g_ssp;

s32 cellAANAddData(u32 aan_handle, u32 aan_port, u32 offset, vm::ptr<float> addr, u32 samples)
{
	libmixer.trace("cellAANAddData(aan_handle=0x%x, aan_port=0x%x, offset=0x%x, addr=*0x%x, samples=%d)", aan_handle, aan_port, offset, addr, samples);

	u32 type = aan_port >> 16;
	u32 port = aan_port & 0xffff;

	switch (type)
	{
	case CELL_SURMIXER_CHSTRIP_TYPE1A:
		if (port >= g_surmx.ch_strips_1) type = 0;
		break;
	case CELL_SURMIXER_CHSTRIP_TYPE2A:
		if (port >= g_surmx.ch_strips_2) type = 0;
		break;
	case CELL_SURMIXER_CHSTRIP_TYPE6A:
		if (port >= g_surmx.ch_strips_6) type = 0;
		break;
	case CELL_SURMIXER_CHSTRIP_TYPE8A:
		if (port >= g_surmx.ch_strips_8) type = 0;
		break;
	default:
		type = 0; break;
	}

	if (aan_handle != 0x11111111 || samples != 256 || !type || offset != 0)
	{
		libmixer.error("cellAANAddData(aan_handle=0x%x, aan_port=0x%x, offset=0x%x, addr=*0x%x, samples=%d): invalid parameters", aan_handle, aan_port, offset, addr, samples);
		return CELL_LIBMIXER_ERROR_INVALID_PARAMATER;
	}

	std::lock_guard lock(g_surmx.mutex);

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
	libmixer.warning("cellAANConnect(receive=0x%x, receivePortNo=0x%x, source=0x%x, sourcePortNo=0x%x)",
		receive, receivePortNo, source, sourcePortNo);

	std::lock_guard lock(g_surmx.mutex);

	if (source >= g_ssp.size() || !g_ssp[source].m_created)
	{
		libmixer.error("cellAANConnect(): invalid source (%d)", source);
		return CELL_LIBMIXER_ERROR_INVALID_PARAMATER;
	}

	g_ssp[source].m_connected = true;

	return CELL_OK;
}

s32 cellAANDisconnect(u32 receive, u32 receivePortNo, u32 source, u32 sourcePortNo)
{
	libmixer.warning("cellAANDisconnect(receive=0x%x, receivePortNo=0x%x, source=0x%x, sourcePortNo=0x%x)",
		receive, receivePortNo, source, sourcePortNo);

	std::lock_guard lock(g_surmx.mutex);

	if (source >= g_ssp.size() || !g_ssp[source].m_created)
	{
		libmixer.error("cellAANDisconnect(): invalid source (%d)", source);
		return CELL_LIBMIXER_ERROR_INVALID_PARAMATER;
	}

	g_ssp[source].m_connected = false;

	return CELL_OK;
}

s32 cellSSPlayerCreate(vm::ptr<u32> handle, vm::ptr<CellSSPlayerConfig> config)
{
	libmixer.warning("cellSSPlayerCreate(handle=*0x%x, config=*0x%x)", handle, config);

	if (config->outputMode != 0u || config->channels - 1u >= 2u)
	{
		libmixer.error("cellSSPlayerCreate(config.outputMode=%d, config.channels=%d): invalid parameters", config->outputMode, config->channels);
		return CELL_LIBMIXER_ERROR_INVALID_PARAMATER;
	}

	std::lock_guard lock(g_surmx.mutex);

	SSPlayer p;
	p.m_created = true;
	p.m_connected = false;
	p.m_active = false;
	p.m_channels = config->channels;

	g_ssp.push_back(p);
	*handle = ::size32(g_ssp) - 1;
	return CELL_OK;
}

s32 cellSSPlayerRemove(u32 handle)
{
	libmixer.warning("cellSSPlayerRemove(handle=0x%x)", handle);

	std::lock_guard lock(g_surmx.mutex);

	if (handle >= g_ssp.size() || !g_ssp[handle].m_created)
	{
		libmixer.error("cellSSPlayerRemove(): SSPlayer not found (%d)", handle);
		return CELL_LIBMIXER_ERROR_INVALID_PARAMATER;
	}

	g_ssp[handle].m_active = false;
	g_ssp[handle].m_created = false;
	g_ssp[handle].m_connected = false;

	return CELL_OK;
}

s32 cellSSPlayerSetWave(u32 handle, vm::ptr<CellSSPlayerWaveParam> waveInfo, vm::ptr<CellSSPlayerCommonParam> commonInfo)
{
	libmixer.warning("cellSSPlayerSetWave(handle=0x%x, waveInfo=*0x%x, commonInfo=*0x%x)", handle, waveInfo, commonInfo);

	std::lock_guard lock(g_surmx.mutex);

	if (handle >= g_ssp.size() || !g_ssp[handle].m_created)
	{
		libmixer.error("cellSSPlayerSetWave(): SSPlayer not found (%d)", handle);
		return CELL_LIBMIXER_ERROR_INVALID_PARAMATER;
	}

	// TODO: check parameters

	g_ssp[handle].m_addr = waveInfo->addr;
	g_ssp[handle].m_samples = waveInfo->samples;
	g_ssp[handle].m_loop_start = waveInfo->loopStartOffset - 1;
	g_ssp[handle].m_loop_mode = commonInfo ? +commonInfo->loopMode : CELL_SSPLAYER_ONESHOT;
	g_ssp[handle].m_position = waveInfo->startOffset - 1;

	return CELL_OK;
}

s32 cellSSPlayerPlay(u32 handle, vm::ptr<CellSSPlayerRuntimeInfo> info)
{
	libmixer.warning("cellSSPlayerPlay(handle=0x%x, info=*0x%x)", handle, info);

	std::lock_guard lock(g_surmx.mutex);

	if (handle >= g_ssp.size() || !g_ssp[handle].m_created)
	{
		libmixer.error("cellSSPlayerPlay(): SSPlayer not found (%d)", handle);
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
	libmixer.warning("cellSSPlayerStop(handle=0x%x, mode=0x%x)", handle, mode);

	std::lock_guard lock(g_surmx.mutex);

	if (handle >= g_ssp.size() || !g_ssp[handle].m_created)
	{
		libmixer.error("cellSSPlayerStop(): SSPlayer not found (%d)", handle);
		return CELL_LIBMIXER_ERROR_INVALID_PARAMATER;
	}

	// TODO: transition to stop state

	g_ssp[handle].m_active = false;

	return CELL_OK;
}

s32 cellSSPlayerSetParam(u32 handle, vm::ptr<CellSSPlayerRuntimeInfo> info)
{
	libmixer.warning("cellSSPlayerSetParam(handle=0x%x, info=*0x%x)", handle, info);

	std::lock_guard lock(g_surmx.mutex);

	if (handle >= g_ssp.size() || !g_ssp[handle].m_created)
	{
		libmixer.error("cellSSPlayerSetParam(): SSPlayer not found (%d)", handle);
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
	libmixer.warning("cellSSPlayerGetState(handle=0x%x)", handle);

	std::lock_guard lock(g_surmx.mutex);

	if (handle >= g_ssp.size() || !g_ssp[handle].m_created)
	{
		libmixer.warning("cellSSPlayerGetState(): SSPlayer not found (%d)", handle);
		return CELL_SSPLAYER_STATE_ERROR;
	}

	if (g_ssp[handle].m_active)
	{
		return CELL_SSPLAYER_STATE_ON;
	}

	return CELL_SSPLAYER_STATE_OFF;
}

struct surmixer_thread : ppu_thread
{
	using ppu_thread::ppu_thread;

	void non_task()
	{
		auto& g_audio = g_fxo->get<cell_audio>();

		audio_port& port = g_audio.ports[g_surmx.audio_port];

		while (port.state != audio_port_state::closed)
		{
			if (g_surmx.mixcount > (port.active_counter + 0)) // adding positive value (1-15): preemptive buffer filling (hack)
			{
				thread_ctrl::wait_for(1000); // hack
				continue;
			}

			if (port.state == audio_port_state::started)
			{
				//u64 stamp0 = get_guest_system_time();

				memset(g_surmx.mixdata, 0, sizeof(g_surmx.mixdata));
				if (g_surmx.cb)
				{
					g_surmx.cb(*this, g_surmx.cb_arg, static_cast<u32>(g_surmx.mixcount), 256);
					lv2_obj::sleep(*this);
				}

				//u64 stamp1 = get_guest_system_time();

				{
					std::lock_guard lock(g_surmx.mutex);

					for (auto& p : g_ssp) if (p.m_active && p.m_created)
					{
						auto v = vm::ptrl<s16>::make(p.m_addr); // 16-bit LE audio data
						float left = 0.0f;
						float right = 0.0f;
						float speed = std::fabs(p.m_speed);
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
							s32 shift = i - static_cast<s32>(fpos); // change playback speed (simple and rough)
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
							p.m_position += pos_inc;
							if (p.m_channels == 1) // get mono data
							{
								left = right = v[pos] / 32768.f * p.m_level;
							}
							else if (p.m_channels == 2) // get stereo data
							{
								left = v[pos * 2 + 0] / 32768.f * p.m_level;
								right = v[pos * 2 + 1] / 32768.f * p.m_level;
							}
							if (p.m_connected) // mix
							{
								// TODO: m_x, m_y, m_z ignored
								g_surmx.mixdata[i * 8 + 0] += left;
								g_surmx.mixdata[i * 8 + 1] += right;
							}
							if ((p.m_position == p.m_samples && p.m_speed > 0.0f) ||
								(p.m_position == umax && p.m_speed < 0.0f)) // loop or stop
							{
								if (p.m_loop_mode == CELL_SSPLAYER_LOOP_ON)
								{
									p.m_position = p.m_loop_start;
								}
								else if (p.m_loop_mode == CELL_SSPLAYER_ONESHOT_CONT)
								{
									p.m_position -= pos_inc; // restore position
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

				//u64 stamp2 = get_guest_system_time();

				auto buf = vm::_ptr<f32>(port.addr.addr() + (g_surmx.mixcount % port.num_blocks) * port.num_channels * AUDIO_BUFFER_SAMPLES * sizeof(float));

				for (auto& mixdata : g_surmx.mixdata)
				{
					// reverse byte order
					*buf++ = mixdata;
				}

				//u64 stamp3 = get_guest_system_time();

				//ConLog.Write("Libmixer perf: start=%lld (cb=%lld, ssp=%lld, finalize=%lld)", stamp0 - m_config.start_time, stamp1 - stamp0, stamp2 - stamp1, stamp3 - stamp2);
			}

			g_surmx.mixcount++;
		}

		idm::remove<ppu_thread>(id);
	}
};

s32 cellSurMixerCreate(vm::cptr<CellSurMixerConfig> config)
{
	libmixer.warning("cellSurMixerCreate(config=*0x%x)", config);

	auto& g_audio = g_fxo->get<cell_audio>();

	const auto port = g_audio.open_port();

	if (!port)
	{
		return CELL_LIBMIXER_ERROR_FULL;
	}

	g_surmx.audio_port = port->number;
	g_surmx.priority = config->priority;
	g_surmx.ch_strips_1 = config->chStrips1;
	g_surmx.ch_strips_2 = config->chStrips2;
	g_surmx.ch_strips_6 = config->chStrips6;
	g_surmx.ch_strips_8 = config->chStrips8;

	port->num_channels = 8;
	port->num_blocks = 16;
	port->attr = 0;
	port->size = port->num_channels * port->num_blocks * AUDIO_BUFFER_SAMPLES * sizeof(float);
	port->level = 1.0f;
	port->level_set.store({ 1.0f, 0.0f });

	libmixer.warning("*** audio port opened (port=%d)", g_surmx.audio_port);

	g_surmx.mixcount = 0;
	g_surmx.cb = vm::null;

	g_ssp.clear();

	libmixer.warning("*** surMixer created (ch1=%d, ch2=%d, ch6=%d, ch8=%d)", config->chStrips1, config->chStrips2, config->chStrips6, config->chStrips8);

	//auto thread = idm::make_ptr<named_thread<ppu_thread>>("Surmixer Thread");

	return CELL_OK;
}

s32 cellSurMixerGetAANHandle(vm::ptr<u32> handle)
{
	libmixer.warning("cellSurMixerGetAANHandle(handle=*0x%x) -> %d", handle, 0x11111111);
	*handle = 0x11111111;
	return CELL_OK;
}

s32 cellSurMixerChStripGetAANPortNo(vm::ptr<u32> port, u32 type, u32 index)
{
	libmixer.warning("cellSurMixerChStripGetAANPortNo(port=*0x%x, type=0x%x, index=0x%x) -> 0x%x", port, type, index, (type << 16) | index);
	*port = (type << 16) | index;
	return CELL_OK;
}

s32 cellSurMixerSetNotifyCallback(vm::ptr<CellSurMixerNotifyCallbackFunction> func, vm::ptr<void> arg)
{
	libmixer.warning("cellSurMixerSetNotifyCallback(func=*0x%x, arg=*0x%x)", func, arg);

	if (g_surmx.cb)
	{
		fmt::throw_exception("Callback already set");
	}

	g_surmx.cb = func;
	g_surmx.cb_arg = arg;

	return CELL_OK;
}

s32 cellSurMixerRemoveNotifyCallback(vm::ptr<CellSurMixerNotifyCallbackFunction> func)
{
	libmixer.warning("cellSurMixerRemoveNotifyCallback(func=*0x%x)", func);

	if (g_surmx.cb != func)
	{
		fmt::throw_exception("Callback not set");
	}

	g_surmx.cb = vm::null;

	return CELL_OK;
}

s32 cellSurMixerStart()
{
	libmixer.warning("cellSurMixerStart()");

	auto& g_audio = g_fxo->get<cell_audio>();

	if (g_surmx.audio_port >= AUDIO_PORT_COUNT)
	{
		return CELL_LIBMIXER_ERROR_NOT_INITIALIZED;
	}

	g_audio.ports[g_surmx.audio_port].state.compare_and_swap(audio_port_state::opened, audio_port_state::started);

	return CELL_OK;
}

s32 cellSurMixerSetParameter(u32 param, float value)
{
	libmixer.todo("cellSurMixerSetParameter(param=0x%x, value=%f)", param, value);
	return CELL_OK;
}

s32 cellSurMixerFinalize()
{
	libmixer.warning("cellSurMixerFinalize()");

	auto& g_audio = g_fxo->get<cell_audio>();

	if (g_surmx.audio_port >= AUDIO_PORT_COUNT)
	{
		return CELL_LIBMIXER_ERROR_NOT_INITIALIZED;
	}

	g_audio.ports[g_surmx.audio_port].state.compare_and_swap(audio_port_state::opened, audio_port_state::closed);

	return CELL_OK;
}

s32 cellSurMixerSurBusAddData(u32 busNo, u32 offset, vm::ptr<float> addr, u32 samples)
{
	if (busNo < 8 && samples == 256 && offset == 0)
	{
		libmixer.trace("cellSurMixerSurBusAddData(busNo=%d, offset=0x%x, addr=0x%x, samples=%d)", busNo, offset, addr, samples);
	}
	else
	{
		libmixer.todo("cellSurMixerSurBusAddData(busNo=%d, offset=0x%x, addr=0x%x, samples=%d)", busNo, offset, addr, samples);
		return CELL_OK;
	}

	std::lock_guard lock(g_surmx.mutex);

	for (u32 i = 0; i < samples; i++)
	{
		// reverse byte order and mix
		g_surmx.mixdata[i * 8 + busNo] += addr[i];
	}

	return CELL_OK;
}

s32 cellSurMixerChStripSetParameter(u32 type, u32 index, vm::ptr<CellSurMixerChStripParam> param)
{
	libmixer.todo("cellSurMixerChStripSetParameter(type=%d, index=%d, param=*0x%x)", type, index, param);
	return CELL_OK;
}

s32 cellSurMixerPause(u32 type)
{
	libmixer.warning("cellSurMixerPause(type=%d)", type);

	auto& g_audio = g_fxo->get<cell_audio>();

	if (g_surmx.audio_port >= AUDIO_PORT_COUNT)
	{
		return CELL_LIBMIXER_ERROR_NOT_INITIALIZED;
	}

	g_audio.ports[g_surmx.audio_port].state.compare_and_swap(audio_port_state::started, audio_port_state::opened);

	return CELL_OK;
}

s32 cellSurMixerGetCurrentBlockTag(vm::ptr<u64> tag)
{
	libmixer.trace("cellSurMixerGetCurrentBlockTag(tag=*0x%x)", tag);

	*tag = g_surmx.mixcount;
	return CELL_OK;
}

s32 cellSurMixerGetTimestamp(u64 tag, vm::ptr<u64> stamp)
{
	libmixer.error("cellSurMixerGetTimestamp(tag=0x%llx, stamp=*0x%x)", tag, stamp);

	auto& g_audio = g_fxo->get<cell_audio>();

	*stamp = g_audio.m_start_time + tag * AUDIO_BUFFER_SAMPLES * 1'000'000 / g_audio.cfg.audio_sampling_rate;

	return CELL_OK;
}

void cellSurMixerBeep(u32 arg)
{
	libmixer.todo("cellSurMixerBeep(arg=%d)", arg);
	return;
}

f32 cellSurMixerUtilGetLevelFromDB(f32 dB)
{
	libmixer.fatal("cellSurMixerUtilGetLevelFromDB(dB=%f)", dB);
	return 0;
}

f32 cellSurMixerUtilGetLevelFromDBIndex(s32 index)
{
	libmixer.fatal("cellSurMixerUtilGetLevelFromDBIndex(index=%d)", index);
	return 0;
}

f32 cellSurMixerUtilNoteToRatio(u8 refNote, u8 note)
{
	libmixer.fatal("cellSurMixerUtilNoteToRatio(refNote=%d, note=%d)", refNote, note);
	return 0;
}

DECLARE(ppu_module_manager::libmixer)("libmixer", []()
{
	REG_FUNC(libmixer, cellAANAddData);
	REG_FUNC(libmixer, cellAANConnect);
	REG_FUNC(libmixer, cellAANDisconnect);

	REG_FUNC(libmixer, cellSurMixerCreate);
	REG_FUNC(libmixer, cellSurMixerGetAANHandle);
	REG_FUNC(libmixer, cellSurMixerChStripGetAANPortNo);
	REG_FUNC(libmixer, cellSurMixerSetNotifyCallback);
	REG_FUNC(libmixer, cellSurMixerRemoveNotifyCallback);
	REG_FUNC(libmixer, cellSurMixerStart);
	REG_FUNC(libmixer, cellSurMixerSetParameter);
	REG_FUNC(libmixer, cellSurMixerFinalize);
	REG_FUNC(libmixer, cellSurMixerSurBusAddData);
	REG_FUNC(libmixer, cellSurMixerChStripSetParameter);
	REG_FUNC(libmixer, cellSurMixerPause);
	REG_FUNC(libmixer, cellSurMixerGetCurrentBlockTag);
	REG_FUNC(libmixer, cellSurMixerGetTimestamp);
	REG_FUNC(libmixer, cellSurMixerBeep);

	REG_FUNC(libmixer, cellSSPlayerCreate);
	REG_FUNC(libmixer, cellSSPlayerRemove);
	REG_FUNC(libmixer, cellSSPlayerSetWave);
	REG_FUNC(libmixer, cellSSPlayerPlay);
	REG_FUNC(libmixer, cellSSPlayerStop);
	REG_FUNC(libmixer, cellSSPlayerSetParam);
	REG_FUNC(libmixer, cellSSPlayerGetState);

	REG_FUNC(libmixer, cellSurMixerUtilGetLevelFromDB);
	REG_FUNC(libmixer, cellSurMixerUtilGetLevelFromDBIndex);
	REG_FUNC(libmixer, cellSurMixerUtilNoteToRatio);
});
