#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/Cell/lv2/sys_process.h"
#include "Emu/Cell/lv2/sys_event.h"
#include "cellAudio.h"
#include <atomic>
#include <cmath>

LOG_CHANNEL(cellAudio);

vm::gvar<char, AUDIO_PORT_OFFSET * AUDIO_PORT_COUNT> g_audio_buffer;

vm::gvar<u64, AUDIO_PORT_COUNT> g_audio_indices;

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
	// Warn if audio backend does not support all requested features
	if (raw_buffering_enabled && !buffering_enabled)
	{
		cellAudio.error("Audio backend %s does not support buffering, this option will be ignored.", backend->GetName());
	}
	if (raw_time_stretching_enabled && !time_stretching_enabled)
	{
		cellAudio.error("Audio backend %s does not support time stretching, this option will be ignored.", backend->GetName());
	}
}


audio_ringbuffer::audio_ringbuffer(cell_audio_config& _cfg)
	: backend(_cfg.backend)
	, cfg(_cfg)
	, buf_sz(AUDIO_BUFFER_SAMPLES * _cfg.audio_channels)
	, emu_paused(Emu.IsPaused())
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
	if (g_cfg.audio.dump_to_file)
	{
		m_dump.reset(new AudioDumper(cfg.audio_channels));
	}

	// Initialize backend
	{
		std::string str;
		backend->dump_capabilities(str);
		cellAudio.notice("cellAudio initializing. Backend: %s, Capabilities: %s", backend->GetName(), str.c_str());
	}

	backend->Open(cfg.num_allocated_buffers);
	backend_open = true;

	ASSERT(!get_backend_playing());
}

audio_ringbuffer::~audio_ringbuffer()
{
	if (!backend_open)
	{
		return;
	}

	if (get_backend_playing() && has_capability(AudioBackend::PLAY_PAUSE_FLUSH))
	{
		backend->Pause();
	}

	backend->Close();
}

f32 audio_ringbuffer::set_frequency_ratio(f32 new_ratio)
{
	if (!has_capability(AudioBackend::SET_FREQUENCY_RATIO))
	{
		ASSERT(new_ratio == 1.0f);
		frequency_ratio = 1.0f;
	}
	else
	{
		frequency_ratio = backend->SetFrequencyRatio(new_ratio);
		//cellAudio.trace("set_frequency_ratio(%1.2f) -> %1.2f", new_ratio, frequency_ratio);
	}
	return frequency_ratio;
}

void audio_ringbuffer::enqueue(const float* in_buffer)
{
	AUDIT(cur_pos < cfg.num_allocated_buffers);

	// Prepare buffer
	const void* buf = in_buffer;

	if (buf == nullptr)
	{
		buf = buffer[cur_pos].get();
		cur_pos = (cur_pos + 1) % cfg.num_allocated_buffers;
	}

	// Dump audio if enabled
	if (m_dump)
	{
		m_dump->WriteData(buf, cfg.audio_buffer_size);
	}

	// Enqueue audio
	bool success = backend->AddData(buf, AUDIO_BUFFER_SAMPLES * cfg.audio_channels);
	if (!success)
	{
		cellAudio.error("Could not enqueue buffer onto audio backend. Attempting to recover...");
		flush();
		return;
	}

	if (has_capability(AudioBackend::PLAY_PAUSE_FLUSH))
	{
		enqueued_samples += AUDIO_BUFFER_SAMPLES;

		// Start playing audio
		play();
	}
}

void audio_ringbuffer::enqueue_silence(u32 buf_count)
{
	AUDIT(has_capability(AudioBackend::PLAY_PAUSE_FLUSH));

	for (u32 i = 0; i < buf_count; i++)
	{
		enqueue(silence_buffer);
	}
}

void audio_ringbuffer::play()
{
	if (!has_capability(AudioBackend::PLAY_PAUSE_FLUSH))
	{
		playing = true;
		return;
	}

	if (playing && has_capability(AudioBackend::IS_PLAYING))
	{
		return;
	}

	if (frequency_ratio != 1.0f)
	{
		set_frequency_ratio(1.0f);
	}

	playing = true;

	ASSERT(enqueued_samples > 0);

	play_timestamp = get_timestamp();
	backend->Play();
}

void audio_ringbuffer::flush()
{
	if (!has_capability(AudioBackend::PLAY_PAUSE_FLUSH))
	{
		playing = false;
		return;
	}

	//cellAudio.trace("Flushing an estimated %llu enqueued samples", enqueued_samples);

	backend->Pause();
	playing = false;

	backend->Flush();

	if (frequency_ratio != 1.0f)
	{
		set_frequency_ratio(1.0f);
	}

	enqueued_samples = 0;
}

u64 audio_ringbuffer::update()
{
	// Check emulator pause state
	if (Emu.IsPaused())
	{
		// Emulator paused
		if (playing && has_capability(AudioBackend::PLAY_PAUSE_FLUSH))
		{
			backend->Pause();
		}
		emu_paused = true;
	}
	else if (emu_paused)
	{
		// Emulator unpaused
		if (enqueued_samples > 0 && has_capability(AudioBackend::PLAY_PAUSE_FLUSH))
		{
			play();
		}
		emu_paused = false;
	}

	// Prepare timestamp and playing status
	const u64 timestamp = get_timestamp();
	const bool new_playing = !emu_paused && get_backend_playing();

	// Calculate how many audio samples have played since last time
	if (cfg.buffering_enabled && (playing || new_playing))
	{
		if (has_capability(AudioBackend::GET_NUM_ENQUEUED_SAMPLES))
		{
			// Backend supports querying for the remaining playtime, so just ask it
			enqueued_samples = backend->GetNumEnqueuedSamples();
		}
		else
		{
			const u64 play_delta = timestamp - (play_timestamp > update_timestamp ? play_timestamp : update_timestamp);

			const u64 delta_samples_tmp = play_delta * static_cast<u64>(cfg.audio_sampling_rate * frequency_ratio) + last_remainder;
			last_remainder = delta_samples_tmp % 1'000'000;
			const u64 delta_samples = delta_samples_tmp / 1'000'000;

			//cellAudio.error("play_delta=%llu delta_samples=%llu", play_delta, delta_samples);
			if (delta_samples > 0)
			{

				if (enqueued_samples < delta_samples)
				{
					enqueued_samples = 0;
				}
				else
				{
					enqueued_samples -= delta_samples;
				}

				if (enqueued_samples == 0)
				{
					cellAudio.trace("Audio buffer about to underrun!");
				}
			}
		}
	}

	// Update playing state
	if (playing != new_playing)
	{
		if (!new_playing)
		{
			cellAudio.warning("Audio backend stopped unexpectedly, likely due to a buffer underrun");

			flush();
			playing = false;
		}
		else
		{
			playing = true;
		}
	}

	// Store and return timestamp
	update_timestamp = timestamp;
	return timestamp;
}

void audio_port::tag(s32 offset)
{
	auto port_pos = position(offset);
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

	prev_touched_tag_nr = UINT32_MAX;
}

std::tuple<u32, u32, u32, u32> cell_audio_thread::count_port_buffer_tags()
{
	AUDIT(cfg.buffering_enabled);

	u32 active = 0;
	u32 in_progress = 0;
	u32 untouched = 0;
	u32 incomplete = 0;

	for (auto& port : ports)
	{
		if (port.state != audio_port_state::started) continue;
		active++;

		auto port_buf = port.get_vm_ptr();
		u32 port_pos = port.position();

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

				retouched |= (tag_nr <= port.prev_touched_tag_nr) && port.prev_touched_tag_nr != UINT32_MAX;
				last_touched_tag_nr = tag_nr;
			}
		}

		// Decide whether the buffer is untouched, in progress, incomplete, or complete
		if (last_touched_tag_nr == UINT32_MAX)
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
	for (auto& port : ports)
	{
		if (port.state != audio_port_state::started) continue;

		memset(port.get_vm_ptr(offset), 0, port.block_size() * sizeof(float));

		if (cfg.buffering_enabled)
		{
			port.tag(offset);
		}
	}
}

void cell_audio_thread::advance(u64 timestamp, bool reset)
{
	std::unique_lock lock(mutex);

	// update ports
	if (reset)
	{
		reset_ports(0);
	}

	for (auto& port : ports)
	{
		if (port.state != audio_port_state::started) continue;

		port.global_counter = m_counter;
		port.active_counter++;
		port.timestamp = timestamp;

		port.cur_pos = port.position(1);
		g_audio_indices[port.number] = port.cur_pos;
	}

	if (cfg.buffering_enabled)
	{
		// Calculate rolling average of enqueued playtime
		const u64 enqueued_playtime = ringbuffer->get_enqueued_playtime(/* raw */ true);
		m_average_playtime = cfg.period_average_alpha * enqueued_playtime + (1.0f - cfg.period_average_alpha) * m_average_playtime;
		//cellAudio.error("m_average_playtime=%4.2f, enqueued_playtime=%u", m_average_playtime, enqueued_playtime);
	}

	m_counter++;
	m_last_period_end = timestamp;
	m_dynamic_period = 0;

	// send aftermix event (normal audio event)
	std::array<std::shared_ptr<lv2_event_queue>, MAX_AUDIO_EVENT_QUEUES> queues;
	std::array<u64, MAX_AUDIO_EVENT_QUEUES> event_sources;
	u32 queue_count = 0;

	event_period++;

	for (const auto& key_inf : keys)
	{
		if (const auto queue = lv2_event_queue::find(key_inf.key))
		{
			if (key_inf.flags & CELL_AUDIO_EVENTFLAG_NOMIX)
			{
				continue;
			}

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
				continue;
			}

			event_sources[queue_count] = key_inf.source;
			queues[queue_count++] = queue;
		}
	}

	lock.unlock();

	for (u32 i = 0; i < queue_count; i++)
	{
		queues[i]->send(event_sources[i], 0, 0, 0);
	}
}

void cell_audio_thread::operator()()
{
	thread_ctrl::set_native_priority(1);

	// Allocate ringbuffer
	ringbuffer.reset(new audio_ringbuffer(cfg));

	// Initialize loop variables
	m_counter = 0;
	m_start_time = ringbuffer->get_timestamp();
	m_last_period_end = m_start_time;
	m_dynamic_period = 0;

	u32 untouched_expected = 0;
	u32 in_progress_expected = 0;

	// Main cellAudio loop
	while (thread_ctrl::state() != thread_state::aborting)
	{
		const u64 timestamp = ringbuffer->update();

		if (Emu.IsPaused())
		{
			thread_ctrl::wait_for(10000);
			continue;
		}

		// TODO: send beforemix event (in ~2,6 ms before mixing)

		const u64 time_since_last_period = timestamp - m_last_period_end;

		if (!cfg.buffering_enabled)
		{
			const u64 period_end = (m_counter * cfg.audio_block_period) + m_start_time;
			const s64 time_left = period_end - timestamp;

			if (time_left > cfg.period_comparison_margin)
			{
				thread_ctrl::wait_for(get_thread_wait_delay(time_left));
				continue;
			}
		}
		else
		{
			const u64 enqueued_samples = ringbuffer->get_enqueued_samples();
			f32 frequency_ratio = ringbuffer->get_frequency_ratio();
			u64 enqueued_playtime = ringbuffer->get_enqueued_playtime();
			const u64 enqueued_buffers = enqueued_samples / AUDIO_BUFFER_SAMPLES;

			const bool playing = ringbuffer->is_playing();

			const auto tag_info = count_port_buffer_tags();
			const u32 active_ports = std::get<0>(tag_info);
			const u32 in_progress  = std::get<1>(tag_info);
			const u32 untouched    = std::get<2>(tag_info);
			const u32 incomplete   = std::get<3>(tag_info);

			// Wait for a dynamic period - try to maintain an average as close as possible to 5.(3)ms
			if (!playing)
			{
				// When the buffer is empty, always use the correct block period
				m_dynamic_period = cfg.audio_block_period;
			}
			else
			{
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
					// Calculate what the playtime is without a frequency ratio
					const u64 raw_enqueued_playtime = ringbuffer->get_enqueued_playtime(/* raw= */ true);

					//  1.0 means exactly as desired
					// <1.0 means not as full as desired
					// >1.0 means more full than desired
					const f32 desired_duration_rate = raw_enqueued_playtime / desired_duration_adjusted;

					// update frequency ratio if necessary
					f32 new_ratio = frequency_ratio;
					if (desired_duration_rate < cfg.time_stretching_threshold)
					{
						const f32 normalized_desired_duration_rate = desired_duration_rate / cfg.time_stretching_threshold;
						const f32 request_ratio = normalized_desired_duration_rate * cfg.time_stretching_scale;
						AUDIT(request_ratio <= 1.0f);

						// change frequency ratio in steps
						if (std::abs(frequency_ratio - request_ratio) > cfg.time_stretching_step)
						{
							new_ratio = ringbuffer->set_frequency_ratio(request_ratio);
						}
					}
					else if (frequency_ratio != 1.0f)
					{
						new_ratio = ringbuffer->set_frequency_ratio(1.0f);
					}

					if (new_ratio != frequency_ratio)
					{
						// ratio changed, calculate new dynamic period
						frequency_ratio = new_ratio;
						enqueued_playtime = ringbuffer->get_enqueued_playtime();
						m_dynamic_period = 0;
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
			}

			s64 time_left = m_dynamic_period - time_since_last_period;
			if (time_left > cfg.period_comparison_margin)
			{
				thread_ctrl::wait_for(get_thread_wait_delay(time_left));
				continue;
			}

			// Fast path for 0 ports active
			if (active_ports == 0)
			{
				// no need to mix, just enqueue silence and advance time
				cellAudio.trace("enqueuing silence: no active ports, enqueued_buffers=%llu", enqueued_buffers);
				if (playing)
				{
					ringbuffer->enqueue_silence(1);
				}
				untouched_expected = 0;
				advance(timestamp);
				continue;
			}

			// Wait until buffers have been touched
			//cellAudio.error("active=%u, in_progress=%u, untouched=%u, incomplete=%u", active_ports, in_progress, untouched, incomplete);
			if (untouched > untouched_expected)
			{
				if (!playing)
				{
					// We ran out of buffer, probably because we waited too long
					// Don't enqueue anything, just advance time
					cellAudio.trace("advancing time: untouched=%u/%u (expected=%u), enqueued_buffers=0", untouched, active_ports, untouched_expected);
					untouched_expected = untouched;
					advance(timestamp);
					continue;
				}

				// Games may sometimes "skip" audio periods entirely if they're falling behind (a sort of "frameskip" for audio)
				// As such, if the game doesn't touch buffers for too long we advance time hoping the game recovers
				if (
					(untouched == active_ports && time_since_last_period > cfg.fully_untouched_timeout) ||
					(time_since_last_period > cfg.partially_untouched_timeout)
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
				if (playing)
				{
					ringbuffer->enqueue_silence(1);
				}
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

			// Handle audio restart
			if (!playing)
			{
				// We are not playing (likely buffer underrun)
				// align to 5.(3)ms on global clock - some games seem to prefer this
				const s64 audio_period_alignment_delta = (timestamp - m_start_time) % cfg.audio_block_period;
				if (audio_period_alignment_delta > cfg.period_comparison_margin)
				{
					thread_ctrl::wait_for(audio_period_alignment_delta - cfg.period_comparison_margin);
				}

				// Flush, add silence, restart algorithm
				cellAudio.trace("play/resume audio: received first audio buffer");
				ringbuffer->flush();
				ringbuffer->enqueue_silence(cfg.desired_full_buffers);
				finish_port_volume_stepping();
				m_average_playtime = static_cast<f32>(ringbuffer->get_enqueued_playtime());
			}
		}

		// Mix
		float *buf = ringbuffer->get_current_buffer();
		if (cfg.audio_channels == 2)
		{
			mix<true>(buf);
		}
		else if (cfg.audio_channels == 8)
		{
			mix<false>(buf);
		}
		else
		{
			fmt::throw_exception("Unsupported number of audio channels: %u", cfg.audio_channels);
		}

		// Enqueue
		ringbuffer->enqueue();

		// Advance time
		advance(timestamp);
	}

	// Destroy ringbuffer
	ringbuffer.reset();
}

template <bool DownmixToStereo>
void cell_audio_thread::mix(float *out_buffer, s32 offset)
{
	AUDIT(out_buffer != nullptr);

	constexpr u32 channels = DownmixToStereo ? 2 : 8;
	constexpr u32 out_buffer_sz = channels * AUDIO_BUFFER_SAMPLES;

	bool first_mix = true;

	// mixing
	for (auto& port : ports)
	{
		if (port.state != audio_port_state::started) continue;

		auto buf = port.get_vm_ptr(offset);
		static const float k = 1.f;
		static const float minus_3db = 0.707f; /* value taken from
							  https://www.dolby.com/us/en/technologies/a-guide-to-dolby-metadata.pdf */
		float& m = port.level;

		// part of cellAudioSetPortLevel functionality
		// spread port volume changes over 13ms
		auto step_volume = [](audio_port& port)
		{
			const auto param = port.level_set.load();

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
		};

		if (port.num_channels == 2)
		{
			if (first_mix)
			{
				for (u32 out = 0, in = 0; out < out_buffer_sz; out += channels, in += 2)
				{
					step_volume(port);

					const float left  = buf[in + 0] * m;
					const float right = buf[in + 1] * m;

					out_buffer[out + 0] = left;
					out_buffer[out + 1] = right;

					if constexpr (!DownmixToStereo)
					{
						out_buffer[out + 2] = 0.0f;
						out_buffer[out + 3] = 0.0f;
						out_buffer[out + 4] = 0.0f;
						out_buffer[out + 5] = 0.0f;
						out_buffer[out + 6] = 0.0f;
						out_buffer[out + 7] = 0.0f;
					}
				}
				first_mix = false;
			}
			else
			{
				for (u32 out = 0, in = 0; out < out_buffer_sz; out += channels, in += 2)
				{
					step_volume(port);

					const float left  = buf[in + 0] * m;
					const float right = buf[in + 1] * m;

					out_buffer[out + 0] += left;
					out_buffer[out + 1] += right;
				}
			}
		}
		else if (port.num_channels == 8)
		{
			if (first_mix)
			{
				for (u32 out = 0, in = 0; out < out_buffer_sz; out += channels, in += 8)
				{
					step_volume(port);

					const float left       = buf[in + 0] * m;
					const float right      = buf[in + 1] * m;
					const float center     = buf[in + 2] * m;
					const float low_freq   = buf[in + 3] * m;
					const float rear_left  = buf[in + 4] * m;
					const float rear_right = buf[in + 5] * m;
					const float side_left  = buf[in + 6] * m;
					const float side_right = buf[in + 7] * m;

					if constexpr (DownmixToStereo)
					{
						const float mid = center * minus_3db; /* don't mix in the lfe as per
											 dolby specification */
						out_buffer[out + 0] = (left + rear_left + (side_left * minus_3db) + mid) * k;
						out_buffer[out + 1] = (right + rear_right + (side_right * minus_3db) + mid) * k;
					}
					else
					{
						out_buffer[out + 0] = left;
						out_buffer[out + 1] = right;
						out_buffer[out + 2] = center;
						out_buffer[out + 3] = low_freq;
						out_buffer[out + 4] = rear_left;
						out_buffer[out + 5] = rear_right;
						out_buffer[out + 6] = side_left;
						out_buffer[out + 7] = side_right;
					}
				}
				first_mix = false;
			}
			else
			{
				for (u32 out = 0, in = 0; out < out_buffer_sz; out += channels, in += 8)
				{
					step_volume(port);

					const float left       = buf[in + 0] * m;
					const float right      = buf[in + 1] * m;
					const float center     = buf[in + 2] * m;
					const float low_freq   = buf[in + 3] * m;
					const float rear_left  = buf[in + 4] * m;
					const float rear_right = buf[in + 5] * m;
					const float side_left  = buf[in + 6] * m;
					const float side_right = buf[in + 7] * m;

					if constexpr (DownmixToStereo)
					{
						const float mid = center * minus_3db;
						out_buffer[out + 0] += (left + rear_left + (side_left * minus_3db) + mid) * k;
						out_buffer[out + 1] += (right + rear_right + (side_right * minus_3db) + mid) * k;
					}
					else
					{
						out_buffer[out + 0] += left;
						out_buffer[out + 1] += right;
						out_buffer[out + 2] += center;
						out_buffer[out + 3] += low_freq;
						out_buffer[out + 4] += rear_left;
						out_buffer[out + 5] += rear_right;
						out_buffer[out + 6] += side_left;
						out_buffer[out + 7] += side_right;
					}
				}
			}
		}
		else
		{
			fmt::throw_exception("Unknown channel count (port=%u, channel=%d)" HERE, port.number, port.num_channels);
		}
	}

	// Nothing was mixed, memset out_buffer to 0
	if (first_mix)
	{
		std::memset(out_buffer, 0, out_buffer_sz * sizeof(float));
	}
	else if (g_cfg.audio.convert_to_u16)
	{
		// convert the data from float to u16 with clipping:
		// 2x MULPS
		// 2x MAXPS (optional)
		// 2x MINPS (optional)
		// 2x CVTPS2DQ (converts float to s32)
		// PACKSSDW (converts s32 to s16 with signed saturation)

		for (size_t i = 0; i < out_buffer_sz; i += 8)
		{
			const auto scale = _mm_set1_ps(0x8000);
			_mm_store_ps(out_buffer + i / 2, _mm_castsi128_ps(_mm_packs_epi32(
				_mm_cvtps_epi32(_mm_mul_ps(_mm_load_ps(out_buffer + i), scale)),
				_mm_cvtps_epi32(_mm_mul_ps(_mm_load_ps(out_buffer + i + 4), scale)))));
		}
	}
}

void cell_audio_thread::finish_port_volume_stepping()
{
	// part of cellAudioSetPortLevel functionality
	for (auto& port : ports)
	{
		if (port.state != audio_port_state::started) continue;

		const auto param = port.level_set.load();
		port.level = param.value;
		port.level_set.compare_and_swap(param, { param.value, 0.0f });
	}
}

error_code cellAudioInit()
{
	cellAudio.warning("cellAudioInit()");

	const auto g_audio = g_fxo->get<cell_audio>();

	std::lock_guard lock(g_audio->mutex);

	if (g_audio->init)
	{
		return CELL_AUDIO_ERROR_ALREADY_INIT;
	}

	std::memset(g_audio_buffer.get_ptr(), 0, g_audio_buffer.alloc_size);
	std::memset(g_audio_indices.get_ptr(), 0, g_audio_indices.alloc_size);

	for (u32 i = 0; i < AUDIO_PORT_COUNT; i++)
	{
		g_audio->ports[i].number = i;
		g_audio->ports[i].addr   = g_audio_buffer + AUDIO_PORT_OFFSET * i;
		g_audio->ports[i].index  = g_audio_indices + i;
		g_audio->ports[i].state  = audio_port_state::closed;
	}

	g_audio->init = 1;

	return CELL_OK;
}

error_code cellAudioQuit(ppu_thread& ppu)
{
	cellAudio.warning("cellAudioQuit()");

	const auto g_audio = g_fxo->get<cell_audio>();

	std::lock_guard lock(g_audio->mutex);

	if (!g_audio->init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	// TODO
	g_audio->keys.clear();
	g_audio->key_count = 0;
	g_audio->event_period = 0;
	g_audio->init = 0;

	return CELL_OK;
}

error_code cellAudioPortOpen(vm::ptr<CellAudioPortParam> audioParam, vm::ptr<u32> portNum)
{
	cellAudio.warning("cellAudioPortOpen(audioParam=*0x%x, portNum=*0x%x)", audioParam, portNum);

	const auto g_audio = g_fxo->get<cell_audio>();

	std::lock_guard lock(g_audio->mutex);

	if (!g_audio->init)
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
		num_channels)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	if (num_blocks != CELL_AUDIO_BLOCK_8 &&
		num_blocks != CELL_AUDIO_BLOCK_16 &&
		num_blocks != 2 &&
		num_blocks != 4 &&
		num_blocks != 32)
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

	// Open audio port
	const auto port = g_audio->open_port();

	if (!port)
	{
		return CELL_AUDIO_ERROR_PORT_FULL;
	}

	port->num_channels   = ::narrow<u32>(num_channels);
	port->num_blocks     = ::narrow<u32>(num_blocks);
	port->attr           = attr;
	port->size           = ::narrow<u32>(num_channels * num_blocks * port->block_size());
	port->cur_pos        = 0;
	port->global_counter = g_audio->m_counter;
	port->active_counter = 0;
	port->timestamp      = g_audio->m_last_period_end;

	if (attr & CELL_AUDIO_PORTATTR_INITLEVEL)
	{
		port->level = audioParam->level * g_cfg.audio.volume / 100.0f;
	}
	else
	{
		port->level = g_cfg.audio.volume / 100.0f;
	}

	port->level_set.store({ port->level, 0.0f });

	*portNum = port->number;
	return CELL_OK;
}

error_code cellAudioGetPortConfig(u32 portNum, vm::ptr<CellAudioPortConfig> portConfig)
{
	cellAudio.trace("cellAudioGetPortConfig(portNum=%d, portConfig=*0x%x)", portNum, portConfig);

	const auto g_audio = g_fxo->get<cell_audio>();

	std::lock_guard lock(g_audio->mutex);

	if (!g_audio->init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (!portConfig || portNum >= AUDIO_PORT_COUNT)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	audio_port& port = g_audio->ports[portNum];

	portConfig->readIndexAddr = port.index;

	switch (auto state = port.state.load())
	{
	case audio_port_state::closed: portConfig->status = CELL_AUDIO_STATUS_CLOSE; break;
	case audio_port_state::opened: portConfig->status = CELL_AUDIO_STATUS_READY; break;
	case audio_port_state::started: portConfig->status = CELL_AUDIO_STATUS_RUN; break;
	default: fmt::throw_exception("Invalid port state (%d: %d)", portNum, static_cast<u32>(state));
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

	const auto g_audio = g_fxo->get<cell_audio>();

	std::lock_guard lock(g_audio->mutex);

	if (!g_audio->init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (portNum >= AUDIO_PORT_COUNT)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	switch (auto state = g_audio->ports[portNum].state.compare_and_swap(audio_port_state::opened, audio_port_state::started))
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

	const auto g_audio = g_fxo->get<cell_audio>();

	std::lock_guard lock(g_audio->mutex);

	if (!g_audio->init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (portNum >= AUDIO_PORT_COUNT)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	switch (auto state = g_audio->ports[portNum].state.exchange(audio_port_state::closed))
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

	const auto g_audio = g_fxo->get<cell_audio>();

	std::lock_guard lock(g_audio->mutex);

	if (!g_audio->init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (portNum >= AUDIO_PORT_COUNT)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	switch (auto state = g_audio->ports[portNum].state.compare_and_swap(audio_port_state::started, audio_port_state::opened))
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

	const auto g_audio = g_fxo->get<cell_audio>();

	std::lock_guard lock(g_audio->mutex);

	if (!g_audio->init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (portNum >= AUDIO_PORT_COUNT)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	audio_port& port = g_audio->ports[portNum];

	if (port.state == audio_port_state::closed)
	{
		return CELL_AUDIO_ERROR_PORT_NOT_OPEN;
	}

	if (port.global_counter < tag)
	{
		cellAudio.error("cellAudioGetPortTimestamp(portNum=%d, tag=0x%llx, stamp=*0x%x) -> CELL_AUDIO_ERROR_TAG_NOT_FOUND", portNum, tag, stamp);
		return CELL_AUDIO_ERROR_TAG_NOT_FOUND;
	}

	u64 delta_tag = port.global_counter - tag;
	u64 delta_tag_stamp = delta_tag * g_audio->cfg.audio_block_period;
	*stamp = port.timestamp - delta_tag_stamp;

	return CELL_OK;
}

error_code cellAudioGetPortBlockTag(u32 portNum, u64 blockNo, vm::ptr<u64> tag)
{
	cellAudio.trace("cellAudioGetPortBlockTag(portNum=%d, blockNo=0x%llx, tag=*0x%x)", portNum, blockNo, tag);

	const auto g_audio = g_fxo->get<cell_audio>();

	std::lock_guard lock(g_audio->mutex);

	if (!g_audio->init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (portNum >= AUDIO_PORT_COUNT)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	audio_port& port = g_audio->ports[portNum];

	if (port.state == audio_port_state::closed)
	{
		return CELL_AUDIO_ERROR_PORT_NOT_OPEN;
	}

	if (blockNo >= port.num_blocks)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	*tag = port.global_counter + blockNo - port.cur_pos;

	return CELL_OK;
}

error_code cellAudioSetPortLevel(u32 portNum, float level)
{
	cellAudio.trace("cellAudioSetPortLevel(portNum=%d, level=%f)", portNum, level);

	const auto g_audio = g_fxo->get<cell_audio>();

	std::lock_guard lock(g_audio->mutex);

	if (!g_audio->init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (portNum >= AUDIO_PORT_COUNT)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	audio_port& port = g_audio->ports[portNum];

	if (port.state == audio_port_state::closed)
	{
		return CELL_AUDIO_ERROR_PORT_NOT_OPEN;
	}

	level *= g_cfg.audio.volume / 100.0f;

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

static error_code AudioCreateNotifyEventQueue(vm::ptr<u32> id, vm::ptr<u64> key, u32 queue_type)
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
		const u32 queue_depth = g_fxo->get<cell_audio>()->init && g_ps3_process_info.sdk_ver <= 0x35FFFF ? 2 : 8;

		if (CellError res{sys_event_queue_create(id, attr, key_value, queue_depth) + 0u})
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

error_code cellAudioCreateNotifyEventQueue(vm::ptr<u32> id, vm::ptr<u64> key)
{
	cellAudio.warning("cellAudioCreateNotifyEventQueue(id=*0x%x, key=*0x%x)", id, key);

	return AudioCreateNotifyEventQueue(id, key, SYS_PPU_QUEUE);
}

error_code cellAudioCreateNotifyEventQueueEx(vm::ptr<u32> id, vm::ptr<u64> key, u32 iFlags)
{
	cellAudio.warning("cellAudioCreateNotifyEventQueueEx(id=*0x%x, key=*0x%x, iFlags=0x%x)", id, key, iFlags);

	if (iFlags & ~CELL_AUDIO_CREATEEVENTFLAG_SPU)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	const u32 queue_type = (iFlags & CELL_AUDIO_CREATEEVENTFLAG_SPU) ? SYS_SPU_QUEUE : SYS_PPU_QUEUE;
	return AudioCreateNotifyEventQueue(id, key, queue_type);
}

error_code AudioSetNotifyEventQueue(u64 key, u32 iFlags)
{
	const auto g_audio = g_fxo->get<cell_audio>();

	std::lock_guard lock(g_audio->mutex);

	if (!g_audio->init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (!lv2_event_queue::find(key))
	{
		return CELL_AUDIO_ERROR_TRANS_EVENT;
	}

	for (const auto& k : g_audio->keys) // check for duplicates
	{
		if (k.key == key)
		{
			return CELL_AUDIO_ERROR_TRANS_EVENT;
		}
	}

	// Set unique source associated with the key
	g_audio->keys.push_back({g_audio->event_period, iFlags, ((process_getpid() + u64{}) << 32) + lv2_event_port::id_base + (g_audio->key_count++ * lv2_event_port::id_step), key});
	g_audio->key_count %= lv2_event_port::id_count;

	return CELL_OK;
}

error_code cellAudioSetNotifyEventQueue(u64 key)
{
	cellAudio.warning("cellAudioSetNotifyEventQueue(key=0x%llx)", key);

	return AudioSetNotifyEventQueue(key, 0);
}

error_code cellAudioSetNotifyEventQueueEx(u64 key, u32 iFlags)
{
	cellAudio.todo("cellAudioSetNotifyEventQueueEx(key=0x%llx, iFlags=0x%x)", key, iFlags);

	if (iFlags & (~0u >> 5))
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	return AudioSetNotifyEventQueue(key, iFlags);
}

error_code AudioRemoveNotifyEventQueue(u64 key, u32 iFlags)
{
	const auto g_audio = g_fxo->get<cell_audio>();

	std::lock_guard lock(g_audio->mutex);

	if (!g_audio->init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	for (auto i = g_audio->keys.cbegin(); i != g_audio->keys.cend(); i++)
	{
		if (i->key == key)
		{
			if (i->flags != iFlags)
			{
				break;
			}

			g_audio->keys.erase(i);

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

	const auto g_audio = g_fxo->get<cell_audio>();

	std::unique_lock lock(g_audio->mutex);

	if (!g_audio->init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (portNum >= AUDIO_PORT_COUNT || !src || !src.aligned())
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	if (samples != 256)
	{
		// despite the docs, seems that only fixed value is supported
		cellAudio.error("cellAudioAddData(): invalid samples value (%d)", samples);
		return CELL_AUDIO_ERROR_PARAM;
	}

	const audio_port& port = g_audio->ports[portNum];

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

	const auto g_audio = g_fxo->get<cell_audio>();

	std::unique_lock lock(g_audio->mutex);

	if (!g_audio->init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (portNum >= AUDIO_PORT_COUNT || !src || !src.aligned())
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	if (samples != 256)
	{
		// despite the docs, seems that only fixed value is supported
		cellAudio.error("cellAudioAdd2chData(): invalid samples value (%d)", samples);
		return CELL_AUDIO_ERROR_PARAM;
	}

	const audio_port& port = g_audio->ports[portNum];

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
	else if (port.num_channels == 6)
	{
		for (u32 i = 0; i < samples; i++)
		{
			dst[i * 6 + 0] += src[i * 2 + 0] * volume; // mix L ch
			dst[i * 6 + 1] += src[i * 2 + 1] * volume; // mix R ch
			//dst[i * 6 + 2] += 0.0f; // center
			//dst[i * 6 + 3] += 0.0f; // LFE
			//dst[i * 6 + 4] += 0.0f; // rear L
			//dst[i * 6 + 5] += 0.0f; // rear R
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

	const auto g_audio = g_fxo->get<cell_audio>();

	std::unique_lock lock(g_audio->mutex);

	if (!g_audio->init)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (portNum >= AUDIO_PORT_COUNT || !src || !src.aligned())
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	const audio_port& port = g_audio->ports[portNum];

	const auto dst = port.get_vm_ptr();

	lock.unlock();

	volume = std::isfinite(volume) ? std::clamp(volume, -16.f, 16.f) : 0.f;

	if (port.num_channels == 6)
	{
		for (u32 i = 0; i < 256; i++)
		{
			dst[i * 6 + 0] += src[i * 6 + 0] * volume; // mix L ch
			dst[i * 6 + 1] += src[i * 6 + 1] * volume; // mix R ch
			dst[i * 6 + 2] += src[i * 6 + 2] * volume; // mix center
			dst[i * 6 + 3] += src[i * 6 + 3] * volume; // mix LFE
			dst[i * 6 + 4] += src[i * 6 + 4] * volume; // mix rear L
			dst[i * 6 + 5] += src[i * 6 + 5] * volume; // mix rear R
		}
	}
	else if (port.num_channels == 8)
	{
		for (u32 i = 0; i < 256; i++)
		{
			dst[i * 8 + 0] += src[i * 6 + 0] * volume; // mix L ch
			dst[i * 8 + 1] += src[i * 6 + 1] * volume; // mix R ch
			dst[i * 8 + 2] += src[i * 6 + 2] * volume; // mix center
			dst[i * 8 + 3] += src[i * 6 + 3] * volume; // mix LFE
			dst[i * 8 + 4] += src[i * 6 + 4] * volume; // mix rear L
			dst[i * 8 + 5] += src[i * 6 + 5] * volume; // mix rear R
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
	return CELL_OK;
}

error_code cellAudioSendAck(u64 data3)
{
	cellAudio.todo("cellAudioSendAck(data3=0x%llx)", data3);
	return CELL_OK;
}

error_code cellAudioSetPersonalDevice(s32 iPersonalStream, s32 iDevice)
{
	cellAudio.todo("cellAudioSetPersonalDevice(iPersonalStream=%d, iDevice=%d)", iPersonalStream, iDevice);
	return CELL_OK;
}

error_code cellAudioUnsetPersonalDevice(s32 iPersonalStream)
{
	cellAudio.todo("cellAudioUnsetPersonalDevice(iPersonalStream=%d)", iPersonalStream);
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
