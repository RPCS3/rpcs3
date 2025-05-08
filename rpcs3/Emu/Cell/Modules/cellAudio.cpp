#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/Audio/audio_utils.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/timers.hpp"
#include "Emu/Cell/lv2/sys_process.h"
#include "Emu/Cell/lv2/sys_event.h"
#include "cellAudio.h"
#include "util/video_provider.h"

#include <cmath>

LOG_CHANNEL(cellAudio);

extern atomic_t<recording_mode> g_recording_mode;

extern void lv2_sleep(u64 timeout, ppu_thread* ppu = nullptr);

vm::gvar<char, AUDIO_PORT_OFFSET * AUDIO_PORT_COUNT> g_audio_buffer;

struct alignas(16) aligned_index_t
{
	be_t<u64> index;
	u8 pad[8];
};

vm::gvar<aligned_index_t, AUDIO_PORT_COUNT> g_audio_indices;

template <>
void fmt_class_string<CellAudioError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](CellAudioError value)
	{
		switch (value)
		{
		STR_CASE(CELL_AUDIO_ERROR_ALREADY_INIT);
		STR_CASE(CELL_AUDIO_ERROR_AUDIOSYSTEM);
		STR_CASE(CELL_AUDIO_ERROR_NOT_INIT);
		STR_CASE(CELL_AUDIO_ERROR_PARAM);
		STR_CASE(CELL_AUDIO_ERROR_PORT_FULL);
		STR_CASE(CELL_AUDIO_ERROR_PORT_ALREADY_RUN);
		STR_CASE(CELL_AUDIO_ERROR_PORT_NOT_OPEN);
		STR_CASE(CELL_AUDIO_ERROR_PORT_NOT_RUN);
		STR_CASE(CELL_AUDIO_ERROR_TRANS_EVENT);
		STR_CASE(CELL_AUDIO_ERROR_PORT_OPEN);
		STR_CASE(CELL_AUDIO_ERROR_SHAREDMEMORY);
		STR_CASE(CELL_AUDIO_ERROR_MUTEX);
		STR_CASE(CELL_AUDIO_ERROR_EVENT_QUEUE);
		STR_CASE(CELL_AUDIO_ERROR_AUDIOSYSTEM_NOT_FOUND);
		STR_CASE(CELL_AUDIO_ERROR_TAG_NOT_FOUND);
		}

		return unknown;
	});
}


cell_audio_config::cell_audio_config()
{
	raw = audio::get_raw_config();
}

void cell_audio_config::reset(bool backend_changed)
{
	if (!backend || backend_changed)
	{
		backend.reset();
		backend = Emu.GetCallbacks().get_audio();
	}

	cellAudio.notice("cellAudio initializing. Backend: %s", backend->GetName());

	const AudioFreq freq = AudioFreq::FREQ_48K;
	const AudioSampleSize sample_size = raw.convert_to_s16 ? AudioSampleSize::S16 : AudioSampleSize::FLOAT;

	const auto& [req_ch_cnt, downmix] = AudioBackend::get_channel_count_and_downmixer(0); // CELL_AUDIO_OUT_PRIMARY
	f64 cb_frame_len = 0.0;
	u32 ch_cnt = 2;
	audio_channel_layout ch_layout = audio_channel_layout::stereo;

	if (backend->Open(raw.audio_device, freq, sample_size, req_ch_cnt, raw.channel_layout))
	{
		cb_frame_len = backend->GetCallbackFrameLen();
		ch_cnt = backend->get_channels();
		ch_layout = backend->get_channel_layout();
		cellAudio.notice("Opened audio backend (sampling_rate=%d, sample_size=%d, channels=%d, layout=%s)", backend->get_sampling_rate(), backend->get_sample_size(), backend->get_channels(), backend->get_channel_layout());
	}
	else
	{
		cellAudio.error("Failed to open audio backend. Make sure that no other application is running that might block audio access (e.g. Netflix).");
	}

	audio_downmix = downmix;
	backend_ch_cnt = ch_cnt;
	backend_channel_layout = ch_layout;
	audio_channels = static_cast<u32>(req_ch_cnt);
	audio_sampling_rate = static_cast<u32>(freq);
	audio_block_period = AUDIO_BUFFER_SAMPLES * 1'000'000 / audio_sampling_rate;
	audio_sample_size = static_cast<u32>(sample_size);
	audio_min_buffer_duration = cb_frame_len + u32{AUDIO_BUFFER_SAMPLES} * 2.0 / audio_sampling_rate; // Add 2 blocks to allow jitter compensation

	audio_buffer_length = AUDIO_BUFFER_SAMPLES * audio_channels;

	desired_buffer_duration = std::max(static_cast<s64>(audio_min_buffer_duration * 1000), raw.desired_buffer_duration) * 1000llu;
	buffering_enabled = raw.buffering_enabled && raw.renderer != audio_renderer::null;

	minimum_block_period = audio_block_period / 2;
	maximum_block_period = (6 * audio_block_period) / 5;

	desired_full_buffers = buffering_enabled ? static_cast<u32>(desired_buffer_duration / audio_block_period) + 3 : 2;
	num_allocated_buffers = desired_full_buffers + EXTRA_AUDIO_BUFFERS;

	fully_untouched_timeout = static_cast<u64>(audio_block_period) * 2;
	partially_untouched_timeout = static_cast<u64>(audio_block_period) * 4;

	const bool raw_time_stretching_enabled = buffering_enabled && raw.enable_time_stretching && (raw.time_stretching_threshold > 0);

	time_stretching_enabled = raw_time_stretching_enabled;
	time_stretching_threshold = raw.time_stretching_threshold / 100.0f;

	// Warn if audio backend does not support all requested features
	if (raw.buffering_enabled && !buffering_enabled)
	{
		cellAudio.error("Audio backend %s does not support buffering, this option will be ignored.", backend->GetName());

		if (raw.enable_time_stretching)
		{
			cellAudio.error("Audio backend %s does not support time stretching, this option will be ignored.", backend->GetName());
		}
	}
}

audio_ringbuffer::audio_ringbuffer(cell_audio_config& _cfg)
	: backend(_cfg.backend)
	, cfg(_cfg)
	, buf_sz(AUDIO_BUFFER_SAMPLES * _cfg.audio_channels)
{
	// Initialize buffers
	if (cfg.num_allocated_buffers > MAX_AUDIO_BUFFERS)
	{
		fmt::throw_exception("MAX_AUDIO_BUFFERS is too small");
	}

	for (u32 i = 0; i < cfg.num_allocated_buffers; i++)
	{
		buffer[i].reset(new float[buf_sz]{});
	}

	// Init audio dumper if enabled
	if (cfg.raw.dump_to_file)
	{
		m_dump.Open(static_cast<AudioChannelCnt>(cfg.audio_channels), static_cast<AudioFreq>(cfg.audio_sampling_rate), AudioSampleSize::FLOAT);
	}

	// Configure resampler
	resampler.set_params(static_cast<AudioChannelCnt>(cfg.audio_channels), static_cast<AudioFreq>(cfg.audio_sampling_rate));
	resampler.set_tempo(RESAMPLER_MAX_FREQ_VAL);

	const f64 buffer_dur_mult = [&]()
	{
		if (cfg.buffering_enabled)
		{
			return cfg.desired_buffer_duration / 1'000'000.0 + 0.02; // Add 20ms to buffer to keep buffering algorithm happy
		}

		return cfg.audio_min_buffer_duration;
	}();

	cb_ringbuf.set_buf_size(static_cast<u32>(cfg.backend_ch_cnt * cfg.audio_sampling_rate * cfg.audio_sample_size * buffer_dur_mult));
	backend->SetWriteCallback(std::bind(&audio_ringbuffer::backend_write_callback, this, std::placeholders::_1, std::placeholders::_2));
	backend->SetStateCallback(std::bind(&audio_ringbuffer::backend_state_callback, this, std::placeholders::_1));
}

audio_ringbuffer::~audio_ringbuffer()
{
	if (get_backend_playing())
	{
		flush();
	}

	backend->Close();
}

f32 audio_ringbuffer::set_frequency_ratio(f32 new_ratio)
{
	frequency_ratio = static_cast<f32>(resampler.set_tempo(new_ratio));

	return frequency_ratio;
}

float* audio_ringbuffer::get_buffer(u32 num) const
{
	AUDIT(num < cfg.num_allocated_buffers);
	AUDIT(buffer[num]);
	return buffer[num].get();
}

u32 audio_ringbuffer::backend_write_callback(u32 size, void *buf)
{
	if (!backend_active.observe()) backend_active = true;

	return static_cast<u32>(cb_ringbuf.pop(buf, size, true));
}

void audio_ringbuffer::backend_state_callback(AudioStateEvent event)
{
	if (event == AudioStateEvent::DEFAULT_DEVICE_MAYBE_CHANGED)
	{
		backend_device_changed = true;
	}
}

u64 audio_ringbuffer::get_timestamp()
{
	return get_system_time();
}

float* audio_ringbuffer::get_current_buffer() const
{
	return get_buffer(cur_pos);
}

u64 audio_ringbuffer::get_enqueued_samples() const
{
	AUDIT(cfg.buffering_enabled);
	const u64 ringbuf_samples = cb_ringbuf.get_used_size() / (cfg.audio_sample_size * cfg.backend_ch_cnt);

	if (cfg.time_stretching_enabled)
	{
		return ringbuf_samples + resampler.samples_available();
	}

	return ringbuf_samples;
}

u64 audio_ringbuffer::get_enqueued_playtime() const
{
	AUDIT(cfg.buffering_enabled);

	return get_enqueued_samples() * 1'000'000 / cfg.audio_sampling_rate;
}

void audio_ringbuffer::enqueue(bool enqueue_silence, bool force)
{
	AUDIT(cur_pos < cfg.num_allocated_buffers);

	// Prepare buffer
	static float silence_buffer[u32{AUDIO_MAX_CHANNELS_COUNT} * u32{AUDIO_BUFFER_SAMPLES}]{};
	float* buf = silence_buffer;

	if (!enqueue_silence)
	{
		buf = buffer[cur_pos].get();
		cur_pos = (cur_pos + 1) % cfg.num_allocated_buffers;
	}

	if (!backend_active.observe() && !force)
	{
		// backend is not ready yet
		return;
	}

	// Enqueue audio
	if (cfg.time_stretching_enabled)
	{
		resampler.put_samples(buf, AUDIO_BUFFER_SAMPLES);
	}
	else
	{
		// Since time stretching step is skipped, we can commit to buffer directly
		commit_data(buf, AUDIO_BUFFER_SAMPLES);
	}
}

void audio_ringbuffer::enqueue_silence(u32 buf_count, bool force)
{
	for (u32 i = 0; i < buf_count; i++)
	{
		enqueue(true, force);
	}
}

void audio_ringbuffer::process_resampled_data()
{
	if (!cfg.time_stretching_enabled) return;

	const auto& [buffer, samples] = resampler.get_samples(static_cast<u32>(cb_ringbuf.get_free_size() / (cfg.audio_sample_size * cfg.backend_ch_cnt)));
	commit_data(buffer, samples);
}

void audio_ringbuffer::commit_data(f32* buf, u32 sample_cnt)
{
	const u32 sample_cnt_in = sample_cnt * cfg.audio_channels;
	const u32 sample_cnt_out = sample_cnt * cfg.backend_ch_cnt;

	// Dump audio if enabled
	m_dump.WriteData(buf, sample_cnt_in * static_cast<u32>(AudioSampleSize::FLOAT));

	// Record audio if enabled
	if (g_recording_mode != recording_mode::stopped)
	{
		utils::video_provider& provider = g_fxo->get<utils::video_provider>();
		provider.present_samples(reinterpret_cast<const u8*>(buf), sample_cnt, cfg.audio_channels);
	}

	// Downmix if necessary
	AudioBackend::downmix(sample_cnt_in, cfg.audio_channels, cfg.backend_channel_layout, buf, buf);

	if (cfg.backend->get_convert_to_s16())
	{
		AudioBackend::convert_to_s16(sample_cnt_out, buf, buf);
	}

	cb_ringbuf.push(buf, sample_cnt_out * cfg.audio_sample_size);
}

void audio_ringbuffer::play()
{
	if (playing)
	{
		return;
	}

	playing = true;
	play_timestamp = get_timestamp();
	backend->Play();
}

void audio_ringbuffer::flush()
{
	backend->Pause();
	cb_ringbuf.writer_flush();
	resampler.flush();
	backend_active = false;
	playing = false;

	if (frequency_ratio != RESAMPLER_MAX_FREQ_VAL)
	{
		frequency_ratio = set_frequency_ratio(RESAMPLER_MAX_FREQ_VAL);
	}
}

u64 audio_ringbuffer::update(bool emu_is_paused)
{
	// Check emulator pause state
	if (emu_is_paused)
	{
		// Emulator paused
		if (playing)
		{
			flush();
		}
	}
	else
	{
		// Emulator unpaused
		play();
	}

	// Prepare timestamp
	const u64 timestamp = get_timestamp();

	// Store and return timestamp
	update_timestamp = timestamp;
	return timestamp;
}

void audio_port::tag(s32 offset)
{
	auto port_buf = get_vm_ptr(offset);

	// This tag will be used to make sure that the game has finished writing the audio for the next audio period
	// We use -0.0f in case games check if the buffer is empty. -0.0f == 0.0f evaluates to true, but std::signbit can be used to distinguish them
	const f32 tag = -0.0f;

	const u32 tag_first_pos = num_channels == 2 ? PORT_BUFFER_TAG_FIRST_2CH : PORT_BUFFER_TAG_FIRST_8CH;
	const u32 tag_delta = num_channels == 2 ? PORT_BUFFER_TAG_DELTA_2CH : PORT_BUFFER_TAG_DELTA_8CH;

	for (u32 tag_pos = tag_first_pos, tag_nr = 0; tag_nr < PORT_BUFFER_TAG_COUNT; tag_pos += tag_delta, tag_nr++)
	{
		port_buf[tag_pos] = tag;
		last_tag_value[tag_nr] = -0.0f;
	}

	prev_touched_tag_nr = -1;
}

cell_audio_thread::cell_audio_thread(utils::serial& ar)
	: cell_audio_thread()
{
	ar(init);

	if (!init)
	{
		return;
	}

	ar(key_count, event_period);

	keys.resize(ar);

	for (key_info& k : keys)
	{
		ar(k.start_period, k.flags, k.source);
		k.port = lv2_event_queue::load_ptr(ar, k.port, "audio");
	}

	ar(ports);
}

void cell_audio_thread::save(utils::serial& ar)
{
	ar(init);

	if (!init)
	{
		return;
	}

	USING_SERIALIZATION_VERSION(cellAudio);

	ar(key_count, event_period);
	ar(keys.size());

	for (const key_info& k : keys)
	{
		ar(k.start_period, k.flags, k.source);
		lv2_event_queue::save_ptr(ar, k.port.get());
	}

	ar(ports);
}

std::tuple<u32, u32, u32, u32> cell_audio_thread::count_port_buffer_tags()
{
	AUDIT(cfg.buffering_enabled);

	u32 active = 0;
	u32 in_progress = 0;
	u32 untouched = 0;
	u32 incomplete = 0;

	for (audio_port& port : ports)
	{
		if (port.state != audio_port_state::started) continue;
		active++;

		auto port_buf = port.get_vm_ptr();

		// Find the last tag that has been touched
		const u32 tag_first_pos = port.num_channels == 2 ? PORT_BUFFER_TAG_FIRST_2CH : PORT_BUFFER_TAG_FIRST_8CH;
		const u32 tag_delta = port.num_channels == 2 ? PORT_BUFFER_TAG_DELTA_2CH : PORT_BUFFER_TAG_DELTA_8CH;

		u32 last_touched_tag_nr = port.prev_touched_tag_nr;
		bool retouched = false;
		for (u32 tag_pos = tag_first_pos, tag_nr = 0; tag_nr < PORT_BUFFER_TAG_COUNT; tag_pos += tag_delta, tag_nr++)
		{
			const f32 val = port_buf[tag_pos];
			f32& last_val = port.last_tag_value[tag_nr];

			if (val != last_val || (last_val == -0.0f && std::signbit(last_val) && !std::signbit(val)))
			{
				last_val = val;

				retouched |= (tag_nr <= port.prev_touched_tag_nr) && port.prev_touched_tag_nr != umax;
				last_touched_tag_nr = tag_nr;
			}
		}

		// Decide whether the buffer is untouched, in progress, incomplete, or complete
		if (last_touched_tag_nr == umax)
		{
			// no tag has been touched yet
			untouched++;
		}
		else if (last_touched_tag_nr == PORT_BUFFER_TAG_COUNT - 1)
		{
			if (retouched)
			{
				// we retouched, so wait at least once more to make sure no more tags get touched
				in_progress++;
			}

			// buffer has been completely filled
			port.prev_touched_tag_nr = last_touched_tag_nr;
		}
		else if (last_touched_tag_nr == port.prev_touched_tag_nr)
		{
			if (retouched)
			{
				// we retouched, so wait at least once more to make sure no more tags get touched
				in_progress++;
			}
			else
			{
				// hasn't been touched since the last call
				incomplete++;
			}
		}
		else
		{
			// the touched tag changed since the last call
			in_progress++;
			port.prev_touched_tag_nr = last_touched_tag_nr;
		}
	}

	return std::make_tuple(active, in_progress, untouched, incomplete);
}

void cell_audio_thread::reset_ports(s32 offset)
{
	// Memset buffer to 0 and tag
	for (audio_port& port : ports)
	{
		if (port.state != audio_port_state::started) continue;

		memset(port.get_vm_ptr(offset), 0, port.block_size() * sizeof(float));

		if (cfg.buffering_enabled)
		{
			port.tag(offset);
		}
	}
}

void cell_audio_thread::advance(u64 timestamp)
{
	ringbuffer->process_resampled_data();

	std::unique_lock lock(mutex);

	// update ports
	reset_ports(0);

	for (audio_port& port : ports)
	{
		if (port.state != audio_port_state::started) continue;

		port.global_counter = m_counter;
		port.active_counter++;
		port.timestamp = timestamp;

		port.cur_pos = port.position(1);
		*port.index = port.cur_pos;
	}

	if (cfg.buffering_enabled)
	{
		// Calculate rolling average of enqueued playtime
		m_average_playtime = cfg.period_average_alpha * ringbuffer->get_enqueued_playtime() + (1.0f - cfg.period_average_alpha) * m_average_playtime;
	}

	m_counter++;
	m_last_period_end = timestamp;
	m_dynamic_period = 0;

	// send aftermix event (normal audio event)
	std::array<shared_ptr<lv2_event_queue>, MAX_AUDIO_EVENT_QUEUES> queues;
	u32 queue_count = 0;

	event_period++;

	for (const key_info& key_inf : keys)
	{
		if (key_inf.flags & CELL_AUDIO_EVENTFLAG_NOMIX)
		{
			continue;
		}

		if ((queues[queue_count] = key_inf.port))
		{
			u32 periods = 1;

			if (key_inf.flags & CELL_AUDIO_EVENTFLAG_DECIMATE_2)
			{
				periods *= 2;
			}

			if (key_inf.flags & CELL_AUDIO_EVENTFLAG_DECIMATE_4)
			{
				// If both flags are set periods is set to x8
				periods *= 4;
			}

			if ((event_period ^ key_inf.start_period) & (periods - 1))
			{
				// The time has not come for this key to receive event
				queues[queue_count].reset();
				continue;
			}

			event_sources[queue_count] = key_inf.source;
			event_data3[queue_count] = (key_inf.flags & CELL_AUDIO_EVENTFLAG_BEFOREMIX) ? key_inf.source : 0;
			queue_count++;
		}
	}

	lock.unlock();

	for (u32 i = 0; i < queue_count; i++)
	{
		lv2_obj::notify_all_t notify;
		queues[i]->send(event_sources[i], CELL_AUDIO_EVENT_MIX, 0, event_data3[i]);
	}
}

namespace audio
{
	cell_audio_config::raw_config get_raw_config()
	{
		return
		{
			.audio_device = g_cfg.audio.audio_device,
			.buffering_enabled = static_cast<bool>(g_cfg.audio.enable_buffering),
			.desired_buffer_duration = g_cfg.audio.desired_buffer_duration,
			.enable_time_stretching = static_cast<bool>(g_cfg.audio.enable_time_stretching),
			.time_stretching_threshold = g_cfg.audio.time_stretching_threshold,
			.convert_to_s16 = static_cast<bool>(g_cfg.audio.convert_to_s16),
			.dump_to_file = static_cast<bool>(g_cfg.audio.dump_to_file),
			.channel_layout = g_cfg.audio.channel_layout,
			.renderer = g_cfg.audio.renderer,
			.provider = g_cfg.audio.provider
		};
	}

	void configure_audio(bool force_reset)
	{
		if (g_cfg.audio.provider != audio_provider::cell_audio)
		{
			return;
		}

		if (auto& g_audio = g_fxo->get<cell_audio>(); g_fxo->is_init<cell_audio>())
		{
			// Only reboot the audio renderer if a relevant setting changed
			const cell_audio_config::raw_config new_raw = get_raw_config();

			if (const cell_audio_config::raw_config raw = g_audio.cfg.raw;
				force_reset ||
				raw.audio_device != new_raw.audio_device ||
				raw.desired_buffer_duration != new_raw.desired_buffer_duration ||
				raw.buffering_enabled != new_raw.buffering_enabled ||
				raw.time_stretching_threshold != new_raw.time_stretching_threshold ||
				raw.enable_time_stretching != new_raw.enable_time_stretching ||
				raw.convert_to_s16 != new_raw.convert_to_s16 ||
				raw.renderer != new_raw.renderer ||
				raw.dump_to_file != new_raw.dump_to_file)
			{
				std::lock_guard lock{g_audio.emu_cfg_upd_m};
				g_audio.cfg.new_raw = new_raw;
				g_audio.m_update_configuration = raw.renderer != new_raw.renderer ? audio_backend_update::ALL : audio_backend_update::PARAM;
			}
		}
	}
}

void cell_audio_thread::update_config(bool backend_changed)
{
	std::lock_guard lock(mutex);

	// Clear ringbuffer
	ringbuffer.reset();

	// Reload config
	cfg.reset(backend_changed);

	// Allocate ringbuffer
	ringbuffer.reset(new audio_ringbuffer(cfg));

	// Reset thread state
	reset_counters();
}

void cell_audio_thread::reset_counters()
{
	m_counter = 0;
	m_start_time = ringbuffer->get_timestamp();
	m_last_period_end = m_start_time;
	m_dynamic_period = 0;
	m_audio_should_restart = true;
}

cell_audio_thread::cell_audio_thread()
{
}

void cell_audio_thread::operator()()
{
	// Initialize loop variables (regardless of provider in order to initialize timestamps)
	reset_counters();

	if (cfg.raw.provider != audio_provider::cell_audio)
	{
		return;
	}

	// Init audio config
	cfg.reset();

	// Allocate ringbuffer
	ringbuffer.reset(new audio_ringbuffer(cfg));

	thread_ctrl::scoped_priority high_prio(+1);

	while (Emu.IsPausedOrReady())
	{
		thread_ctrl::wait_for(5000);
	}

	u32 untouched_expected = 0;

	u32 loop_count = 0;

	// Main cellAudio loop
	while (thread_ctrl::state() != thread_state::aborting)
	{
		loop_count++;

		const audio_backend_update update_req = m_update_configuration.observe();
		if (update_req != audio_backend_update::NONE)
		{
			cellAudio.warning("Updating cell_audio_thread configuration");
			{
				std::lock_guard lock{emu_cfg_upd_m};
				cfg.raw = cfg.new_raw;
				m_update_configuration = audio_backend_update::NONE;
			}
			update_config(update_req == audio_backend_update::ALL);
		}

		const bool emu_paused = Emu.IsPaused();
		const u64 timestamp = ringbuffer->update(emu_paused || m_backend_failed);

		if (emu_paused)
		{
			m_audio_should_restart = true;
			ringbuffer->flush();
			thread_ctrl::wait_for(10000);
			continue;
		}

		// TODO: (no idea how much of this is already implemented)
		// The hardware heartbeat interval of libaudio is ~5.3ms.
		// As soon as one interval starts, libaudio waits for ~2.6ms (half of the interval) before it mixes the audio.
		// There are 2 different types of games:
		// - Normal games:
		//     Once the audio was mixed, we send the CELL_AUDIO_EVENT_MIX event and the game can process audio.
		// - Latency sensitive games:
		//     If CELL_AUDIO_EVENTFLAG_BEFOREMIX is specified, we immediately send the CELL_AUDIO_EVENT_MIX event and the game can process audio.
		//     We then have to wait for a maximum of ~2.6ms for cellAudioSendAck and then mix immediately.

		const u64 time_since_last_period = timestamp - m_last_period_end;

		// Handle audio restart
		if (m_audio_should_restart)
		{
			// align to 5.(3)ms on global clock - some games seem to prefer this
			const s64 audio_period_alignment_delta = (timestamp - m_start_time) % cfg.audio_block_period;
			if (audio_period_alignment_delta > cfg.period_comparison_margin)
			{
				thread_ctrl::wait_for(audio_period_alignment_delta - cfg.period_comparison_margin);
			}

			if (cfg.buffering_enabled)
			{
				// Restart algorithm
				cellAudio.trace("restarting audio");
				ringbuffer->enqueue_silence(cfg.desired_full_buffers, true);
				finish_port_volume_stepping();
				m_average_playtime = static_cast<f32>(ringbuffer->get_enqueued_playtime());
				untouched_expected = 0;
			}

			m_audio_should_restart = false;
			continue;
		}

		bool operational = ringbuffer->get_operational_status();

		if (!operational && loop_count % 128 == 0)
		{
			update_config(true);
			operational = ringbuffer->get_operational_status();
		}

		if (ringbuffer->device_changed())
		{
			cellAudio.warning("Default device changed, attempting to switch...");
			update_config(false);

			if (operational != ringbuffer->get_operational_status())
			{
				continue;
			}
		}

		if (!m_backend_failed && !operational)
		{
			cellAudio.error("Backend stopped unexpectedly (likely device change). Attempting to recover...");
			m_backend_failed = true;
		}
		else if (m_backend_failed && operational)
		{
			cellAudio.success("Backend recovered");
			m_backend_failed = false;
		}

		if (!cfg.buffering_enabled)
		{
			const u64 period_end = (m_counter * cfg.audio_block_period) + m_start_time;
			const s64 time_left = period_end - timestamp;

			if (time_left > cfg.period_comparison_margin)
			{
				thread_ctrl::wait_for(time_left - cfg.period_comparison_margin);
				continue;
			}
		}
		else
		{
			const u64 enqueued_samples = ringbuffer->get_enqueued_samples();
			const f32 frequency_ratio = ringbuffer->get_frequency_ratio();
			const u64 enqueued_playtime = ringbuffer->get_enqueued_playtime();
			const u64 enqueued_buffers = enqueued_samples / AUDIO_BUFFER_SAMPLES;

			const auto tag_info = count_port_buffer_tags();
			const u32 active_ports = std::get<0>(tag_info);
			const u32 in_progress  = std::get<1>(tag_info);
			const u32 untouched    = std::get<2>(tag_info);
			const u32 incomplete   = std::get<3>(tag_info);

			// Ratio between the rolling average of the audio period, and the desired audio period
			const f32 average_playtime_ratio = m_average_playtime / cfg.audio_buffer_length;

			// Use the above average ratio to decide how much buffer we should be aiming for
			f32 desired_duration_adjusted = cfg.desired_buffer_duration + (cfg.audio_block_period / 2.0f);
			if (average_playtime_ratio < 1.0f)
			{
				desired_duration_adjusted /= std::max(average_playtime_ratio, 0.25f);
			}

			if (cfg.time_stretching_enabled)
			{
				//  1.0 means exactly as desired
				// <1.0 means not as full as desired
				// >1.0 means more full than desired
				const f32 desired_duration_rate = enqueued_playtime / desired_duration_adjusted;

				// update frequency ratio if necessary
				if (desired_duration_rate < cfg.time_stretching_threshold)
				{
					const f32 normalized_desired_duration_rate = desired_duration_rate / cfg.time_stretching_threshold;

					// change frequency ratio in steps
					const f32 req_time_stretching_step = (normalized_desired_duration_rate + frequency_ratio) / 2.0f;
					if (std::abs(req_time_stretching_step - frequency_ratio) > cfg.time_stretching_step)
					{
						ringbuffer->set_frequency_ratio(req_time_stretching_step);
					}
				}
				else if (frequency_ratio != RESAMPLER_MAX_FREQ_VAL)
				{
					ringbuffer->set_frequency_ratio(RESAMPLER_MAX_FREQ_VAL);
				}
			}

			//  1.0 means exactly as desired
			// <1.0 means not as full as desired
			// >1.0 means more full than desired
			const f32 desired_duration_rate = enqueued_playtime / desired_duration_adjusted;

			if (desired_duration_rate >= 1.0f)
			{
				// more full than desired
				const f32 multiplier = 1.0f / desired_duration_rate;
				m_dynamic_period = cfg.maximum_block_period - static_cast<u64>((cfg.maximum_block_period - cfg.audio_block_period) * multiplier);
			}
			else
			{
				// not as full as desired
				const f32 multiplier = desired_duration_rate * desired_duration_rate; // quite aggressive, but helps more times than it hurts
				m_dynamic_period = cfg.minimum_block_period + static_cast<u64>((cfg.audio_block_period - cfg.minimum_block_period) * multiplier);
			}

			const s64 time_left = m_dynamic_period - time_since_last_period;
			if (time_left > cfg.period_comparison_margin)
			{
				thread_ctrl::wait_for(time_left - cfg.period_comparison_margin);
				continue;
			}

			// Fast path for 0 ports active
			if (active_ports == 0)
			{
				// no need to mix, just enqueue silence and advance time
				cellAudio.trace("enqueuing silence: no active ports, enqueued_buffers=%llu", enqueued_buffers);
				ringbuffer->enqueue_silence();
				untouched_expected = 0;
				advance(timestamp);
				continue;
			}

			// Wait until buffers have been touched
			//cellAudio.error("active=%u, in_progress=%u, untouched=%u, incomplete=%u", active_ports, in_progress, untouched, incomplete);
			if (untouched > untouched_expected)
			{
				// Games may sometimes "skip" audio periods entirely if they're falling behind (a sort of "frameskip" for audio)
				// As such, if the game doesn't touch buffers for too long we advance time hoping the game recovers
				if (
					(untouched == active_ports && time_since_last_period > cfg.fully_untouched_timeout) ||
					(time_since_last_period > cfg.partially_untouched_timeout) || g_cfg.audio.disable_sampling_skip
				   )
				{
					// There's no audio in the buffers, simply advance time and hope the game recovers
					cellAudio.trace("advancing time: untouched=%u/%u (expected=%u), enqueued_buffers=%llu", untouched, active_ports, untouched_expected, enqueued_buffers);
					untouched_expected = untouched;
					advance(timestamp);
					continue;
				}

				cellAudio.trace("waiting: untouched=%u/%u (expected=%u), enqueued_buffers=%llu", untouched, active_ports, untouched_expected, enqueued_buffers);
				thread_ctrl::wait_for(1000);
				continue;
			}

			// Fast-path for when there is no audio in the buffers
			if (untouched == active_ports)
			{
				// There's no audio in the buffers, simply advance time
				cellAudio.trace("enqueuing silence: untouched=%u/%u (expected=%u), enqueued_buffers=%llu", untouched, active_ports, untouched_expected, enqueued_buffers);
				ringbuffer->enqueue_silence();
				untouched_expected = untouched;
				advance(timestamp);
				continue;
			}

			// Wait for buffer(s) to be completely filled
			if (in_progress > 0)
			{
				cellAudio.trace("waiting: in_progress=%u/%u, enqueued_buffers=%u", in_progress, active_ports, enqueued_buffers);
				thread_ctrl::wait_for(500);
				continue;
			}

			//cellAudio.error("active=%u, untouched=%u, in_progress=%d, incomplete=%d, enqueued_buffers=%u", active_ports, untouched, in_progress, incomplete, enqueued_buffers);

			// Store number of untouched buffers for future reference
			untouched_expected = untouched;

			// Log if we enqueued untouched/incomplete buffers
			if (untouched > 0 || incomplete > 0)
			{
				cellAudio.trace("enqueueing: untouched=%u/%u (expected=%u), incomplete=%u/%u enqueued_buffers=%llu", untouched, active_ports, untouched_expected, incomplete, active_ports, enqueued_buffers);
			}
		}

		// Mix
		float* buf = ringbuffer->get_current_buffer();

		switch (cfg.audio_channels)
		{
		case 2:
			switch (cfg.audio_downmix)
			{
			case AudioChannelCnt::SURROUND_7_1:
				mix<AudioChannelCnt::STEREO, AudioChannelCnt::SURROUND_7_1>(buf);
				break;
			case AudioChannelCnt::STEREO:
				mix<AudioChannelCnt::STEREO, AudioChannelCnt::STEREO>(buf);
				break;
			case AudioChannelCnt::SURROUND_5_1:
				mix<AudioChannelCnt::STEREO, AudioChannelCnt::SURROUND_5_1>(buf);
				break;
			}
			break;

		case 6:
			switch (cfg.audio_downmix)
			{
			case AudioChannelCnt::SURROUND_7_1:
				mix<AudioChannelCnt::SURROUND_5_1, AudioChannelCnt::SURROUND_7_1>(buf);
				break;
			case AudioChannelCnt::STEREO:
				mix<AudioChannelCnt::SURROUND_5_1, AudioChannelCnt::STEREO>(buf);
				break;
			case AudioChannelCnt::SURROUND_5_1:
				mix<AudioChannelCnt::SURROUND_5_1, AudioChannelCnt::SURROUND_5_1>(buf);
				break;
			}
			break;

		case 8:
			switch (cfg.audio_downmix)
			{
			case AudioChannelCnt::SURROUND_7_1:
				mix<AudioChannelCnt::SURROUND_7_1, AudioChannelCnt::SURROUND_7_1>(buf);
				break;
			case AudioChannelCnt::STEREO:
				mix<AudioChannelCnt::SURROUND_7_1, AudioChannelCnt::STEREO>(buf);
				break;
			case AudioChannelCnt::SURROUND_5_1:
				mix<AudioChannelCnt::SURROUND_7_1, AudioChannelCnt::SURROUND_5_1>(buf);
				break;
			}
			break;

		default:
			fmt::throw_exception("Unsupported channel count in cell_audio_config: %d", cfg.audio_channels);
		}

		// Enqueue
		ringbuffer->enqueue();

		// Advance time
		advance(timestamp);
	}

	// Destroy ringbuffer
	ringbuffer.reset();
}

audio_port* cell_audio_thread::open_port()
{
	for (u32 i = 0; i < AUDIO_PORT_COUNT; i++)
	{
		if (ports[i].state.compare_and_swap_test(audio_port_state::closed, audio_port_state::opened))
		{
			return &ports[i];
		}
	}

	return nullptr;
}

template <AudioChannelCnt channels, AudioChannelCnt downmix>
void cell_audio_thread::mix(float* out_buffer, s32 offset)
{
	AUDIT(out_buffer != nullptr);

	constexpr u32 out_channels = static_cast<u32>(channels);
	constexpr u32 out_buffer_sz = out_channels * AUDIO_BUFFER_SAMPLES;

	const float master_volume = audio::get_volume();

	// Reset out_buffer
	std::memset(out_buffer, 0, out_buffer_sz * sizeof(float));

	// mixing
	for (audio_port& port : ports)
	{
		if (port.state != audio_port_state::started) continue;

		auto buf = port.get_vm_ptr(offset);

		static constexpr float minus_3db = 0.707f; // value taken from https://www.dolby.com/us/en/technologies/a-guide-to-dolby-metadata.pdf
		float m = master_volume;

		// part of cellAudioSetPortLevel functionality
		// spread port volume changes over 13ms
		auto step_volume = [master_volume, &m](audio_port& port)
		{
			const audio_port::level_set_t param = port.level_set.load();

			if (param.inc != 0.0f)
			{
				port.level += param.inc;
				const bool dec = param.inc < 0.0f;

				if ((!dec && param.value - port.level <= 0.0f) || (dec && param.value - port.level >= 0.0f))
				{
					port.level = param.value;
					port.level_set.compare_and_swap(param, { param.value, 0.0f });
				}
			}

			m = port.level * master_volume;
		};

		if (port.num_channels == 2)
		{
			for (u32 out = 0, in = 0; out < out_buffer_sz; out += out_channels, in += 2)
			{
				step_volume(port);

				const float left  = buf[in + 0] * m;
				const float right = buf[in + 1] * m;

				out_buffer[out + 0] += left;
				out_buffer[out + 1] += right;
			}
		}
		else if (port.num_channels == 8)
		{
			for (u32 out = 0, in = 0; out < out_buffer_sz; out += out_channels, in += 8)
			{
				step_volume(port);

				const float left       = buf[in + 0] * m;
				const float right      = buf[in + 1] * m;
				const float center     = buf[in + 2] * m;
				const float low_freq   = buf[in + 3] * m;
				const float side_left  = buf[in + 4] * m;
				const float side_right = buf[in + 5] * m;
				const float rear_left  = buf[in + 6] * m;
				const float rear_right = buf[in + 7] * m;

				if constexpr (downmix == AudioChannelCnt::STEREO)
				{
					// Don't mix in the lfe as per dolby specification and based on documentation
					const float mid = center * 0.5f;
					out_buffer[out + 0] += left * minus_3db + mid + side_left * 0.5f + rear_left * 0.5f;
					out_buffer[out + 1] += right * minus_3db + mid + side_right * 0.5f + rear_right * 0.5f;
				}
				else if constexpr (downmix == AudioChannelCnt::SURROUND_5_1)
				{
					out_buffer[out + 0] += left;
					out_buffer[out + 1] += right;

					if constexpr (out_channels >= 6) // Only mix the surround channels into the output if surround output is configured
					{
						out_buffer[out + 2] += center;
						out_buffer[out + 3] += low_freq;

						if constexpr (out_channels == 6)
						{
							out_buffer[out + 4] += side_left + rear_left;
							out_buffer[out + 5] += side_right + rear_right;
						}
						else // When using 7.1 ouput, out_buffer[out + 4] and out_buffer[out + 5] are the rear channels, so the side channels need to be mixed into [out + 6] and [out + 7]
						{
							out_buffer[out + 6] += side_left + rear_left;
							out_buffer[out + 7] += side_right + rear_right;
						}
					}
				}
				else
				{
					out_buffer[out + 0] += left;
					out_buffer[out + 1] += right;

					if constexpr (out_channels >= 6) // Only mix the surround channels into the output if surround output is configured
					{
						out_buffer[out + 2] += center;
						out_buffer[out + 3] += low_freq;

						if constexpr (out_channels == 6)
						{
							out_buffer[out + 4] += side_left;
							out_buffer[out + 5] += side_right;
						}
						else
						{
							out_buffer[out + 4] += rear_left;
							out_buffer[out + 5] += rear_right;
							out_buffer[out + 6] += side_left;
							out_buffer[out + 7] += side_right;
						}
					}
				}
			}
		}
		else
		{
			fmt::throw_exception("Unknown channel count (port=%u, channel=%d)", port.number, port.num_channels);
		}
	}
}

void cell_audio_thread::finish_port_volume_stepping()
{
	// part of cellAudioSetPortLevel functionality
	for (audio_port& port : ports)
	{
		if (port.state != audio_port_state::started) continue;

		const audio_port::level_set_t param = port.level_set.load();
		port.level = param.value;
		port.level_set.compare_and_swap(param, { param.value, 0.0f });
	}
}

error_code cellAudioInit()
{
	cellAudio.warning("cellAudioInit()");

	auto& g_audio = g_fxo->get<cell_audio>();

	std::lock_guard lock(g_audio.mutex);

	if (g_audio.init)
	{
		return CELL_AUDIO_ERROR_ALREADY_INIT;
	}

	std::memset(g_audio_buffer.get_ptr(), 0, g_audio_buffer.alloc_size);
	std::memset(g_audio_indices.get_ptr(), 0, g_audio_indices.alloc_size);

	for (u32 i = 0; i < AUDIO_PORT_COUNT; i++)
	{
		g_audio.ports[i].number = i;
		g_audio.ports[i].addr   = g_audio_buffer + AUDIO_PORT_OFFSET * i;
		g_audio.ports[i].index  = (g_audio_indices + i).ptr(&aligned_index_t::index);
		g_audio.ports[i].state  = audio_port_state::closed;
	}

	g_audio.init = 1;

	return CELL_OK;
}

error_code cellAudioQuit()
{
	cellAudio.warning("cellAudioQuit()");

	auto& g_audio = g_fxo->get<cell_audio>();

	std::lock_guard lock(g_audio.mutex);

	if (!g_audio.init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	// NOTE: Do not clear event queues here. They are handled independently.
	g_audio.init = 0;

	return CELL_OK;
}

error_code cellAudioPortOpen(vm::ptr<CellAudioPortParam> audioParam, vm::ptr<u32> portNum)
{
	cellAudio.warning("cellAudioPortOpen(audioParam=*0x%x, portNum=*0x%x)", audioParam, portNum);

	auto& g_audio = g_fxo->get<cell_audio>();

	if (!g_audio.init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (!audioParam || !portNum)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	const u64 num_channels = audioParam->nChannel;
	const u64 num_blocks = audioParam->nBlock;
	const u64 attr = audioParam->attr;

	// check attributes
	if (num_channels != CELL_AUDIO_PORT_2CH &&
		num_channels != CELL_AUDIO_PORT_8CH &&
		num_channels != 0)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	if (num_blocks != 2 &&
		num_blocks != 4 &&
		num_blocks != CELL_AUDIO_BLOCK_8 &&
		num_blocks != CELL_AUDIO_BLOCK_16 &&
		num_blocks != CELL_AUDIO_BLOCK_32)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	// list unsupported flags
	if (attr & CELL_AUDIO_PORTATTR_BGM)
	{
		cellAudio.todo("cellAudioPortOpen(): CELL_AUDIO_PORTATTR_BGM");
	}
	if (attr & CELL_AUDIO_PORTATTR_OUT_SECONDARY)
	{
		cellAudio.todo("cellAudioPortOpen(): CELL_AUDIO_PORTATTR_OUT_SECONDARY");
	}
	if (attr & CELL_AUDIO_PORTATTR_OUT_PERSONAL_0)
	{
		cellAudio.todo("cellAudioPortOpen(): CELL_AUDIO_PORTATTR_OUT_PERSONAL_0");
	}
	if (attr & CELL_AUDIO_PORTATTR_OUT_PERSONAL_1)
	{
		cellAudio.todo("cellAudioPortOpen(): CELL_AUDIO_PORTATTR_OUT_PERSONAL_1");
	}
	if (attr & CELL_AUDIO_PORTATTR_OUT_PERSONAL_2)
	{
		cellAudio.todo("cellAudioPortOpen(): CELL_AUDIO_PORTATTR_OUT_PERSONAL_2");
	}
	if (attr & CELL_AUDIO_PORTATTR_OUT_PERSONAL_3)
	{
		cellAudio.todo("cellAudioPortOpen(): CELL_AUDIO_PORTATTR_OUT_PERSONAL_3");
	}
	if (attr & CELL_AUDIO_PORTATTR_OUT_NO_ROUTE)
	{
		cellAudio.todo("cellAudioPortOpen(): CELL_AUDIO_PORTATTR_OUT_NO_ROUTE");
	}
	if (attr & 0xFFFFFFFFF0EFEFEEULL)
	{
		cellAudio.todo("cellAudioPortOpen(): unknown attributes (0x%llx)", attr);
	}

	// Waiting for VSH and doing some more things
	lv2_sleep(200);

	std::lock_guard lock(g_audio.mutex);

	if (!g_audio.init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	// Open audio port
	audio_port* port = g_audio.open_port();

	if (!port)
	{
		return CELL_AUDIO_ERROR_PORT_FULL;
	}

	// TODO: is this necessary in any way? (Based on libaudio.prx)
	//const u64 num_channels_non_0 = std::max<u64>(1, num_channels);

	port->num_channels   = ::narrow<u32>(num_channels);
	port->num_blocks     = ::narrow<u32>(num_blocks);
	port->attr           = attr;
	port->size           = ::narrow<u32>(num_channels * num_blocks * AUDIO_BUFFER_SAMPLES * sizeof(f32));
	port->cur_pos        = 0;
	port->global_counter = g_audio.m_counter;
	port->active_counter = 0;
	port->timestamp      = get_guest_system_time(g_audio.m_last_period_end);

	if (attr & CELL_AUDIO_PORTATTR_INITLEVEL)
	{
		port->level = audioParam->level;
	}
	else
	{
		port->level = 1.0f;
	}

	port->level_set.store({ port->level, 0.0f });

	if (port->level <= -1.0f)
	{
		return CELL_AUDIO_ERROR_PORT_OPEN;
	}

	*portNum = port->number;
	return CELL_OK;
}

error_code cellAudioGetPortConfig(u32 portNum, vm::ptr<CellAudioPortConfig> portConfig)
{
	cellAudio.trace("cellAudioGetPortConfig(portNum=%d, portConfig=*0x%x)", portNum, portConfig);

	auto& g_audio = g_fxo->get<cell_audio>();

	std::lock_guard lock(g_audio.mutex);

	if (!g_audio.init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (!portConfig || portNum >= AUDIO_PORT_COUNT)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	audio_port& port = g_audio.ports[portNum];

	portConfig->readIndexAddr = port.index;

	switch (audio_port_state state = port.state.load())
	{
	case audio_port_state::closed:
		portConfig->status = CELL_AUDIO_STATUS_CLOSE;
		break;
	case audio_port_state::opened:
		portConfig->status = CELL_AUDIO_STATUS_READY;
		break;
	case audio_port_state::started:
		portConfig->status = CELL_AUDIO_STATUS_RUN;
		break;
	default:
		cellAudio.error("Invalid port state (%d: %d)", portNum, static_cast<u32>(state));
		return CELL_AUDIO_ERROR_AUDIOSYSTEM;
	}

	portConfig->nChannel = port.num_channels;
	portConfig->nBlock   = port.num_blocks;
	portConfig->portSize = port.size;
	portConfig->portAddr = port.addr.addr();
	return CELL_OK;
}

error_code cellAudioPortStart(u32 portNum)
{
	cellAudio.warning("cellAudioPortStart(portNum=%d)", portNum);

	auto& g_audio = g_fxo->get<cell_audio>();

	if (!g_audio.init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (portNum >= AUDIO_PORT_COUNT)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	// Waiting for VSH
	lv2_sleep(30);

	std::lock_guard lock(g_audio.mutex);

	if (!g_audio.init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	switch (audio_port_state state = g_audio.ports[portNum].state.compare_and_swap(audio_port_state::opened, audio_port_state::started))
	{
	case audio_port_state::closed: return CELL_AUDIO_ERROR_PORT_NOT_OPEN;
	case audio_port_state::started: return CELL_AUDIO_ERROR_PORT_ALREADY_RUN;
	case audio_port_state::opened: return CELL_OK;
	default: fmt::throw_exception("Invalid port state (%d: %d)", portNum, static_cast<u32>(state));
	}
}

error_code cellAudioPortClose(u32 portNum)
{
	cellAudio.warning("cellAudioPortClose(portNum=%d)", portNum);

	auto& g_audio = g_fxo->get<cell_audio>();

	std::lock_guard lock(g_audio.mutex);

	if (!g_audio.init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (portNum >= AUDIO_PORT_COUNT)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	switch (audio_port_state state = g_audio.ports[portNum].state.exchange(audio_port_state::closed))
	{
	case audio_port_state::closed: return CELL_AUDIO_ERROR_PORT_NOT_OPEN;
	case audio_port_state::started: return CELL_OK;
	case audio_port_state::opened: return CELL_OK;
	default: fmt::throw_exception("Invalid port state (%d: %d)", portNum, static_cast<u32>(state));
	}
}

error_code cellAudioPortStop(u32 portNum)
{
	cellAudio.warning("cellAudioPortStop(portNum=%d)", portNum);

	auto& g_audio = g_fxo->get<cell_audio>();

	std::lock_guard lock(g_audio.mutex);

	if (!g_audio.init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (portNum >= AUDIO_PORT_COUNT)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	switch (audio_port_state state = g_audio.ports[portNum].state.compare_and_swap(audio_port_state::started, audio_port_state::opened))
	{
	case audio_port_state::closed: return CELL_AUDIO_ERROR_PORT_NOT_RUN;
	case audio_port_state::started: return CELL_OK;
	case audio_port_state::opened: return CELL_AUDIO_ERROR_PORT_NOT_RUN;
	default: fmt::throw_exception("Invalid port state (%d: %d)", portNum, static_cast<u32>(state));
	}
}

error_code cellAudioGetPortTimestamp(u32 portNum, u64 tag, vm::ptr<u64> stamp)
{
	cellAudio.trace("cellAudioGetPortTimestamp(portNum=%d, tag=0x%llx, stamp=*0x%x)", portNum, tag, stamp);

	auto& g_audio = g_fxo->get<cell_audio>();

	std::lock_guard lock(g_audio.mutex);

	if (!g_audio.init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (portNum >= AUDIO_PORT_COUNT)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	audio_port& port = g_audio.ports[portNum];

	if (port.state == audio_port_state::closed)
	{
		return CELL_AUDIO_ERROR_PORT_NOT_OPEN;
	}

	if (port.global_counter < tag)
	{
		return CELL_AUDIO_ERROR_TAG_NOT_FOUND;
	}

	const u64 delta_tag = port.global_counter - tag;
	const u64 delta_tag_stamp = delta_tag * g_audio.cfg.audio_block_period;

	// Apparently no error is returned if stamp is null
	*stamp = get_guest_system_time(port.timestamp - delta_tag_stamp);

	return CELL_OK;
}

error_code cellAudioGetPortBlockTag(u32 portNum, u64 blockNo, vm::ptr<u64> tag)
{
	cellAudio.trace("cellAudioGetPortBlockTag(portNum=%d, blockNo=0x%llx, tag=*0x%x)", portNum, blockNo, tag);

	auto& g_audio = g_fxo->get<cell_audio>();

	std::lock_guard lock(g_audio.mutex);

	if (!g_audio.init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (portNum >= AUDIO_PORT_COUNT)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	audio_port& port = g_audio.ports[portNum];

	if (port.state == audio_port_state::closed)
	{
		return CELL_AUDIO_ERROR_PORT_NOT_OPEN;
	}

	if (blockNo >= port.num_blocks)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	// Apparently no error is returned if tag is null
	*tag = port.global_counter + blockNo - port.cur_pos;

	return CELL_OK;
}

error_code cellAudioSetPortLevel(u32 portNum, float level)
{
	cellAudio.trace("cellAudioSetPortLevel(portNum=%d, level=%f)", portNum, level);

	auto& g_audio = g_fxo->get<cell_audio>();

	std::lock_guard lock(g_audio.mutex);

	if (!g_audio.init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (portNum >= AUDIO_PORT_COUNT)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	audio_port& port = g_audio.ports[portNum];

	if (port.state == audio_port_state::closed)
	{
		return CELL_AUDIO_ERROR_PORT_NOT_OPEN;
	}

	if (level >= 0.0f)
	{
		port.level_set.exchange({ level, (port.level - level) / 624.0f });
	}
	else
	{
		cellAudio.todo("cellAudioSetPortLevel(%d): negative level value (%f)", portNum, level);
	}

	return CELL_OK;
}

static error_code AudioCreateNotifyEventQueue(ppu_thread& ppu, vm::ptr<u32> id, vm::ptr<u64> key, u32 queue_type)
{
	vm::var<sys_event_queue_attribute_t> attr;
	attr->protocol = SYS_SYNC_FIFO;
	attr->type     = queue_type;
	attr->name_u64 = 0;

	for (u64 i = 0; i < MAX_AUDIO_EVENT_QUEUES; i++)
	{
		// Create an event queue "bruteforcing" an available key
		const u64 key_value = 0x80004d494f323221ull + i;

		// This originally reads from a global sdk value set by cellAudioInit
		// So check initialization as well
		const u32 queue_depth = g_fxo->get<cell_audio>().init && g_ps3_process_info.sdk_ver <= 0x35FFFF ? 2 : 8;

		if (CellError res{sys_event_queue_create(ppu, id, attr, key_value, queue_depth) + 0u})
		{
			if (res != CELL_EEXIST)
			{
				return res;
			}
		}
		else
		{
			*key = key_value;
			return CELL_OK;
		}
	}

	return CELL_AUDIO_ERROR_EVENT_QUEUE;
}

error_code cellAudioCreateNotifyEventQueue(ppu_thread& ppu, vm::ptr<u32> id, vm::ptr<u64> key)
{
	cellAudio.warning("cellAudioCreateNotifyEventQueue(id=*0x%x, key=*0x%x)", id, key);

	return AudioCreateNotifyEventQueue(ppu, id, key, SYS_PPU_QUEUE);
}

error_code cellAudioCreateNotifyEventQueueEx(ppu_thread& ppu, vm::ptr<u32> id, vm::ptr<u64> key, u32 iFlags)
{
	cellAudio.warning("cellAudioCreateNotifyEventQueueEx(id=*0x%x, key=*0x%x, iFlags=0x%x)", id, key, iFlags);

	if (iFlags & ~CELL_AUDIO_CREATEEVENTFLAG_SPU)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	const u32 queue_type = (iFlags & CELL_AUDIO_CREATEEVENTFLAG_SPU) ? SYS_SPU_QUEUE : SYS_PPU_QUEUE;
	return AudioCreateNotifyEventQueue(ppu, id, key, queue_type);
}

error_code AudioSetNotifyEventQueue(ppu_thread& ppu, u64 key, u32 iFlags)
{
	auto& g_audio = g_fxo->get<cell_audio>();

	if (!g_audio.init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	// Waiting for VSH
	lv2_sleep(20, &ppu);

	// Dirty hack for sound: confirm the creation of _mxr000 event queue by _cellsurMixerMain thread
	constexpr u64 c_mxr000 = 0x8000cafe0246030;

	if (key == c_mxr000 || key == 0)
	{
		bool has_sur_mixer_thread = false;

		for (usz count = 0; !lv2_event_queue::find(c_mxr000) && count < 100; count++)
		{
			if (has_sur_mixer_thread || idm::select<named_thread<ppu_thread>>([&](u32 id, named_thread<ppu_thread>& test_ppu)
			{
				// Confirm thread existence
				if (id == ppu.id)
				{
					return false;
				}

				const auto ptr = test_ppu.ppu_tname.load();

				if (!ptr)
				{
					return false;
				}

				return *ptr == "_cellsurMixerMain"sv;
			}).ret)
			{
				has_sur_mixer_thread = true;
			}
			else
			{
				break;
			}

			if (ppu.is_stopped())
			{
				ppu.state += cpu_flag::again;
				return {};
			}

			cellAudio.error("AudioSetNotifyEventQueue(): Waiting for _mxr000. x%d", count);

			lv2_sleep(50'000, &ppu);
		}

		if (has_sur_mixer_thread && lv2_event_queue::find(c_mxr000))
		{
			key = c_mxr000;
		}
	}

	std::lock_guard lock(g_audio.mutex);

	if (!g_audio.init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	auto q = lv2_event_queue::find(key);

	if (!q)
	{
		return CELL_AUDIO_ERROR_TRANS_EVENT;
	}

	for (auto i = g_audio.keys.cbegin(); i != g_audio.keys.cend();) // check for duplicates
	{
		if (i->port == q)
		{
			return CELL_AUDIO_ERROR_TRANS_EVENT;
		}

		if (!lv2_obj::check(i->port))
		{
			// Cleanup, avoid cases where there are multiple ports with the same key
			i = g_audio.keys.erase(i);
		}
		else
		{
			i++;
		}
	}

	// Set unique source associated with the key
	g_audio.keys.push_back
	({
		.start_period = g_audio.event_period,
		.flags = iFlags,
		.source = ((process_getpid() + u64{}) << 32) + lv2_event_port::id_base + (g_audio.key_count++ * lv2_event_port::id_step),
		.ack_timestamp = 0,
		.port = std::move(q)
	});

	g_audio.key_count %= lv2_event_port::id_count;

	return CELL_OK;
}

error_code cellAudioSetNotifyEventQueue(ppu_thread& ppu, u64 key)
{
	ppu.state += cpu_flag::wait;

	cellAudio.warning("cellAudioSetNotifyEventQueue(key=0x%llx)", key);

	return AudioSetNotifyEventQueue(ppu, key, 0);
}

error_code cellAudioSetNotifyEventQueueEx(ppu_thread& ppu, u64 key, u32 iFlags)
{
	ppu.state += cpu_flag::wait;

	cellAudio.todo("cellAudioSetNotifyEventQueueEx(key=0x%llx, iFlags=0x%x)", key, iFlags);

	if (iFlags & (~0u >> 5))
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	return AudioSetNotifyEventQueue(ppu, key, iFlags);
}

error_code AudioRemoveNotifyEventQueue(u64 key, u32 iFlags)
{
	auto& g_audio = g_fxo->get<cell_audio>();

	std::lock_guard lock(g_audio.mutex);

	if (!g_audio.init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	for (auto i = g_audio.keys.cbegin(); i != g_audio.keys.cend(); i++)
	{
		if (lv2_obj::check(i->port) && i->port->key == key)
		{
			if (i->flags != iFlags)
			{
				break;
			}

			g_audio.keys.erase(i);

			return CELL_OK;
		}
	}

	return CELL_AUDIO_ERROR_TRANS_EVENT;
}

error_code cellAudioRemoveNotifyEventQueue(u64 key)
{
	cellAudio.warning("cellAudioRemoveNotifyEventQueue(key=0x%llx)", key);

	return AudioRemoveNotifyEventQueue(key, 0);
}

error_code cellAudioRemoveNotifyEventQueueEx(u64 key, u32 iFlags)
{
	cellAudio.todo("cellAudioRemoveNotifyEventQueueEx(key=0x%llx, iFlags=0x%x)", key, iFlags);

	if (iFlags & (~0u >> 5))
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	return AudioRemoveNotifyEventQueue(key, iFlags);
}

error_code cellAudioAddData(u32 portNum, vm::ptr<float> src, u32 samples, float volume)
{
	cellAudio.trace("cellAudioAddData(portNum=%d, src=*0x%x, samples=%d, volume=%f)", portNum, src, samples, volume);

	auto& g_audio = g_fxo->get<cell_audio>();

	std::unique_lock lock(g_audio.mutex);

	if (!g_audio.init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (portNum >= AUDIO_PORT_COUNT || !src || !src.aligned() || samples != CELL_AUDIO_BLOCK_SAMPLES)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	const audio_port& port = g_audio.ports[portNum];

	const auto dst = port.get_vm_ptr();

	lock.unlock();

	volume = std::isfinite(volume) ? std::clamp(volume, -16.f, 16.f) : 0.f;

	for (u32 i = 0; i < samples * port.num_channels; i++)
	{
		dst[i] += src[i] * volume; // mix all channels
	}

	return CELL_OK;
}

error_code cellAudioAdd2chData(u32 portNum, vm::ptr<float> src, u32 samples, float volume)
{
	cellAudio.trace("cellAudioAdd2chData(portNum=%d, src=*0x%x, samples=%d, volume=%f)", portNum, src, samples, volume);

	auto& g_audio = g_fxo->get<cell_audio>();

	std::unique_lock lock(g_audio.mutex);

	if (!g_audio.init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (portNum >= AUDIO_PORT_COUNT || !src || !src.aligned() || samples != CELL_AUDIO_BLOCK_SAMPLES)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	const audio_port& port = g_audio.ports[portNum];

	const auto dst = port.get_vm_ptr();

	lock.unlock();

	volume = std::isfinite(volume) ? std::clamp(volume, -16.f, 16.f) : 0.f;

	if (port.num_channels == 2)
	{
		for (u32 i = 0; i < samples; i++)
		{
			dst[i * 2 + 0] += src[i * 2 + 0] * volume; // mix L ch
			dst[i * 2 + 1] += src[i * 2 + 1] * volume; // mix R ch
		}
	}
	else if (port.num_channels == 8)
	{
		for (u32 i = 0; i < samples; i++)
		{
			dst[i * 8 + 0] += src[i * 2 + 0] * volume; // mix L ch
			dst[i * 8 + 1] += src[i * 2 + 1] * volume; // mix R ch
			//dst[i * 8 + 2] += 0.0f; // center
			//dst[i * 8 + 3] += 0.0f; // LFE
			//dst[i * 8 + 4] += 0.0f; // rear L
			//dst[i * 8 + 5] += 0.0f; // rear R
			//dst[i * 8 + 6] += 0.0f; // side L
			//dst[i * 8 + 7] += 0.0f; // side R
		}
	}
	else
	{
		cellAudio.error("cellAudioAdd2chData(portNum=%d): invalid port.channel value (%d)", portNum, port.num_channels);
	}

	return CELL_OK;
}

error_code cellAudioAdd6chData(u32 portNum, vm::ptr<float> src, float volume)
{
	cellAudio.trace("cellAudioAdd6chData(portNum=%d, src=*0x%x, volume=%f)", portNum, src, volume);

	auto& g_audio = g_fxo->get<cell_audio>();

	std::unique_lock lock(g_audio.mutex);

	if (!g_audio.init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (portNum >= AUDIO_PORT_COUNT || !src || !src.aligned())
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	const audio_port& port = g_audio.ports[portNum];

	const auto dst = port.get_vm_ptr();

	lock.unlock();

	volume = std::isfinite(volume) ? std::clamp(volume, -16.f, 16.f) : 0.f;

	if (port.num_channels == 8)
	{
		for (u32 i = 0; i < CELL_AUDIO_BLOCK_SAMPLES; i++)
		{
			// Channel order in src is Front Left, Center, Front Right, Surround Left, Surround Right, LFE
			dst[i * 8 + 0] += src[i * 6 + 0] * volume; // mix L ch
			dst[i * 8 + 1] += src[i * 6 + 2] * volume; // mix R ch
			dst[i * 8 + 2] += src[i * 6 + 1] * volume; // mix center
			dst[i * 8 + 3] += src[i * 6 + 5] * volume; // mix LFE
			dst[i * 8 + 4] += src[i * 6 + 3] * volume; // mix rear L
			dst[i * 8 + 5] += src[i * 6 + 4] * volume; // mix rear R
			//dst[i * 8 + 6] += 0.0f; // side L
			//dst[i * 8 + 7] += 0.0f; // side R
		}
	}
	else
	{
		cellAudio.error("cellAudioAdd6chData(portNum=%d): invalid port.channel value (%d)", portNum, port.num_channels);
	}

	return CELL_OK;
}

error_code cellAudioMiscSetAccessoryVolume(u32 devNum, float volume)
{
	cellAudio.todo("cellAudioMiscSetAccessoryVolume(devNum=%d, volume=%f)", devNum, volume);

	auto& g_audio = g_fxo->get<cell_audio>();

	std::unique_lock lock(g_audio.mutex);

	if (!g_audio.init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	// TODO

	return CELL_OK;
}

error_code cellAudioSendAck(u64 data3)
{
	cellAudio.trace("cellAudioSendAck(data3=0x%llx)", data3);

	auto& g_audio = g_fxo->get<cell_audio>();

	std::unique_lock lock(g_audio.mutex);

	if (!g_audio.init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	// TODO: error checks

	for (cell_audio_thread::key_info& k : g_audio.keys)
	{
		if (k.source == data3)
		{
			k.ack_timestamp = get_system_time();
			break;
		}
	}

	return CELL_OK;
}

error_code cellAudioSetPersonalDevice(s32 iPersonalStream, s32 iDevice)
{
	cellAudio.todo("cellAudioSetPersonalDevice(iPersonalStream=%d, iDevice=%d)", iPersonalStream, iDevice);

	auto& g_audio = g_fxo->get<cell_audio>();

	std::unique_lock lock(g_audio.mutex);

	if (!g_audio.init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (iPersonalStream < 0 || iPersonalStream >= 4 ||
		(iDevice != CELL_AUDIO_PERSONAL_DEVICE_PRIMARY && (iDevice < 0 || iDevice >= 5)))
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	// TODO

	return CELL_OK;
}

error_code cellAudioUnsetPersonalDevice(s32 iPersonalStream)
{
	cellAudio.todo("cellAudioUnsetPersonalDevice(iPersonalStream=%d)", iPersonalStream);

	auto& g_audio = g_fxo->get<cell_audio>();

	std::unique_lock lock(g_audio.mutex);

	if (!g_audio.init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (iPersonalStream < 0 || iPersonalStream >= 4)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	// TODO

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellAudio)("cellAudio", []()
{
	// Private variables
	REG_VAR(cellAudio, g_audio_buffer).flag(MFF_HIDDEN);
	REG_VAR(cellAudio, g_audio_indices).flag(MFF_HIDDEN);

	REG_FUNC(cellAudio, cellAudioInit);
	REG_FUNC(cellAudio, cellAudioPortClose);
	REG_FUNC(cellAudio, cellAudioPortStop);
	REG_FUNC(cellAudio, cellAudioGetPortConfig);
	REG_FUNC(cellAudio, cellAudioPortStart);
	REG_FUNC(cellAudio, cellAudioQuit);
	REG_FUNC(cellAudio, cellAudioPortOpen);
	REG_FUNC(cellAudio, cellAudioSetPortLevel);
	REG_FUNC(cellAudio, cellAudioCreateNotifyEventQueue);
	REG_FUNC(cellAudio, cellAudioCreateNotifyEventQueueEx);
	REG_FUNC(cellAudio, cellAudioMiscSetAccessoryVolume);
	REG_FUNC(cellAudio, cellAudioSetNotifyEventQueue);
	REG_FUNC(cellAudio, cellAudioSetNotifyEventQueueEx);
	REG_FUNC(cellAudio, cellAudioGetPortTimestamp);
	REG_FUNC(cellAudio, cellAudioAdd2chData);
	REG_FUNC(cellAudio, cellAudioAdd6chData);
	REG_FUNC(cellAudio, cellAudioAddData);
	REG_FUNC(cellAudio, cellAudioGetPortBlockTag);
	REG_FUNC(cellAudio, cellAudioRemoveNotifyEventQueue);
	REG_FUNC(cellAudio, cellAudioRemoveNotifyEventQueueEx);
	REG_FUNC(cellAudio, cellAudioSendAck);
	REG_FUNC(cellAudio, cellAudioSetPersonalDevice);
	REG_FUNC(cellAudio, cellAudioUnsetPersonalDevice);
});
