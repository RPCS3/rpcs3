#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/Cell/lv2/sys_event.h"
#include "cellAudio.h"
#include <atomic>

LOG_CHANNEL(cellAudio);

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

audio_ringbuffer::audio_ringbuffer(u32 num_buffers, u32 audio_sampling_rate, u32 channels)
	: num_allocated_buffers(num_buffers)
	, audio_sampling_rate(audio_sampling_rate)
	, backend(Emu.GetCallbacks().get_audio())
	, channels(channels)
	, buf_sz(AUDIO_BUFFER_SAMPLES * channels)
	, emu_paused(Emu.IsPaused())
{
	// Initialize buffers
	if (num_allocated_buffers >= MAX_AUDIO_BUFFERS)
	{
		fmt::throw_exception("MAX_AUDIO_BUFFERS is too small");
	}

	for (u32 i = 0; i < num_allocated_buffers; i++)
	{
		buffer[i].reset(new float[buf_sz]{});
	}

	// Init audio dumper if enabled
	if (g_cfg.audio.dump_to_file)
	{
		m_dump.reset(new AudioDumper(channels));
	}

	// Initialize backend
	backend->Open();
	backend_open = true;
	ASSERT(!backend->IsPlaying());
}

audio_ringbuffer::~audio_ringbuffer()
{
	if (!backend_open)
		return;

	if (backend->IsPlaying())
		backend->Pause();

	backend->Close();
}

void audio_ringbuffer::enqueue(const float* in_buffer)
{
	AUDIT(next_buf < num_allocated_buffers);

	// Prepare buffer
	const void* buf = in_buffer;

	if (buf == nullptr)
	{
		buf = buffer[next_buf].get();
		next_buf = (next_buf + 1) % num_allocated_buffers;
	}

	// Dump audio if enabled
	if (m_dump)
	{
		m_dump->WriteData(buf, buf_sz);
	}

	// Enqueue audio
	bool success = backend->AddData(buf, buf_sz);
	if (!success)
	{
		cellAudio.error("Could not enqueue buffer onto audio backend. Attempting to recover...");
		flush();
		return;
	}

	enqueued_samples += AUDIO_BUFFER_SAMPLES;

	// Start playing audio
	play();
}

void audio_ringbuffer::enqueue_silence(u32 buf_count)
{
	for (u32 i = 0; i < buf_count; i++)
	{
		enqueue(silence_buffer);
	}
}

void audio_ringbuffer::play()
{
	if (playing)
		return;

	playing = true;

	ASSERT(enqueued_samples > 0);

	play_timestamp = get_timestamp();
	backend->Play();
}

void audio_ringbuffer::flush()
{
	cellAudio.trace("Flushing an estimated %llu enqueued samples", enqueued_samples);

	backend->Pause();
	playing = false;

	backend->Flush();
	enqueued_samples = 0;
}

u64 audio_ringbuffer::update()
{
	// Check emulator pause state
	if (Emu.IsPaused())
	{
		// Emulator paused
		if (playing)
		{
			backend->Pause();
		}
		emu_paused = true;
	}
	else if (emu_paused)
	{
		// Emulator unpaused
		play();
		emu_paused = false;
	}

	const u64 timestamp = get_timestamp();
	const bool new_playing = !emu_paused && backend->IsPlaying();

	// Calculate how many audio samples have played since last time
	// TODO: Natively query backend for remaining samples
	if (playing || new_playing)
	{
		const u64 play_delta = timestamp - (play_timestamp > update_timestamp ? play_timestamp : update_timestamp);

		// NOTE: Only works with a fixed sampling rate
		const u64 delta_samples_tmp = (play_delta * audio_sampling_rate) + last_remainder;
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
				cellAudio.warning("Audio buffer about to underrun!");
			}
		}
	}

	// Update playing state
	if (playing != new_playing)
	{
		if (!new_playing)
		{
			cellAudio.error("Audio backend stopped unexpectedly, likely due to a buffer underrun");

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

constexpr bool audio_port::is_tag(float val)
{
	return val == 0.0f && std::signbit(val);
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
		tag_backup[port_pos][tag_nr] = 0.0f;
	}

	prev_touched_tag_nr = UINT32_MAX;
}

void audio_port::apply_tag_backups(s32 offset)
{
	auto port_pos = position(offset);
	auto port_buf = get_vm_ptr(offset);

	const u32 tag_first_pos = num_channels == 2 ? PORT_BUFFER_TAG_FIRST_2CH : PORT_BUFFER_TAG_FIRST_8CH;
	const u32 tag_delta = num_channels == 2 ? PORT_BUFFER_TAG_DELTA_2CH : PORT_BUFFER_TAG_DELTA_8CH;

	for (u32 tag_pos = tag_first_pos, tag_nr = 0; tag_nr < PORT_BUFFER_TAG_COUNT; tag_pos += tag_delta, tag_nr++)
	{
		port_buf[tag_pos] += tag_backup[port_pos][tag_nr];
	}
}

std::tuple<u32, u32, u32, u32> cell_audio_thread::count_port_buffer_tags()
{
	AUDIT(buffering_enabled);

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

		const f32 tag = -0.0f;

		u32 last_touched_tag_nr = port.prev_touched_tag_nr;
		bool retouched = false;
		for (u32 tag_pos = tag_first_pos, tag_nr = 0; tag_nr < PORT_BUFFER_TAG_COUNT; tag_pos += tag_delta, tag_nr++)
		{
			// grab current value and re-tag atomically
			f32 val = atomic_storage<to_be_t<float>, 4>::exchange(port_buf[tag_pos], tag);

			if (!audio_port::is_tag(val))
			{
				port.tag_backup[port_pos][tag_nr] += val;

				retouched |= tag_nr < port.prev_touched_tag_nr && port.prev_touched_tag_nr != UINT32_MAX;
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
			// hasn't been touched since the last call
			incomplete++;
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
	// Memset previous buffer to 0
	for (auto& port : ports)
	{
		if (port.state != audio_port_state::started) continue;

		memset(port.get_vm_ptr(offset), 0, port.block_size() * sizeof(float));

		if (buffering_enabled)
		{
			//port.reset_tag_backups(offset);
			port.tag(offset);
		}
	}
}

void cell_audio_thread::advance(u64 timestamp, bool reset)
{
	auto _locked = g_idm->lock<named_thread<cell_audio_thread>>(0);

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
		m_indexes[port.number] = port.cur_pos;
	}

	m_counter++;
	m_last_period_end = timestamp;
	m_dynamic_period = 0;

	// send aftermix event (normal audio event)
	std::array<std::shared_ptr<lv2_event_queue>, MAX_AUDIO_EVENT_QUEUES> queues;
	u32 queue_count = 0;

	for (u64 key : keys)
	{
		if (auto queue = lv2_event_queue::find(key))
		{
			queues[queue_count++] = queue;
		}
	}

	_locked.unlock();

	for (u32 i = 0; i < queue_count; i++)
	{
		queues[i]->send(0, 0, 0, 0); // TODO: check arguments
	}
}

void cell_audio_thread::operator()()
{
	thread_ctrl::set_native_priority(1);

	// Allocate ringbuffer
	ringbuffer.reset(new audio_ringbuffer(num_allocated_buffers, audio_sampling_rate, audio_channels));

	// Initialize loop variables
	m_counter = 0;
	m_start_time = ringbuffer->get_timestamp();
	m_last_period_end = m_start_time;
	m_dynamic_period = 0;

	u32 untouched_expected = 0;
	u32 in_progress_expected = 0;

	// Main cellAudio loop
	while (thread_ctrl::state() != thread_state::aborting && !Emu.IsStopped())
	{
		const u64 timestamp = ringbuffer->update();

		if (Emu.IsPaused())
		{
			thread_ctrl::wait_for(10000);
			continue;
		}

		// TODO: send beforemix event (in ~2,6 ms before mixing)

		const u64 time_since_last_period = timestamp - m_last_period_end;
		const bool playing = !ringbuffer->is_playing();

		if (!buffering_enabled)
		{
			const u64 period_end = (m_counter * audio_block_period) + m_start_time;
			const s64 time_left = period_end - timestamp;

			if (time_left > period_comparison_margin)
			{
				thread_ctrl::wait_for(get_thread_wait_delay(time_left));
				continue;
			}
		}
		else
		{
			const u64 enqueued_playtime = ringbuffer->get_enqueued_samples() * 1'000'000 / audio_sampling_rate;
			const u64 enqueued_buffers = (enqueued_playtime) / audio_block_period;

			const bool playing = ringbuffer->is_playing();

			const auto tag_info = count_port_buffer_tags();
			const u32 active_ports = std::get<0>(tag_info);
			const u32 in_progress  = std::get<1>(tag_info);
			const u32 untouched    = std::get<2>(tag_info);
			const u32 incomplete   = std::get<3>(tag_info);

			// Wait for a dynamic period - try to maintain an average as close as possible to 5.(3)ms
			if (m_dynamic_period == 0)
			{
				if (!playing)
				{
					// When the buffer is empty, always use the correct block period
					m_dynamic_period = audio_block_period;
				}
				else
				{
					//  1.0 means exactly as desired
					// <1.0 means not as full as desired
					// >1.0 means more full than desired
					const f32 desired_duration_rate = (enqueued_playtime) / static_cast<f32>(desired_buffer_duration);

					if (desired_duration_rate >= 1.0f)
					{
						// more full than desired
						const f32 multiplier = 1.0f / desired_duration_rate;
						m_dynamic_period = maximum_block_period - static_cast<u64>((maximum_block_period - audio_block_period) * multiplier);
					}
					else
					{
						// not as full as desired
						const f32 multiplier = desired_duration_rate;
						m_dynamic_period = minimum_block_period + static_cast<u64>((audio_block_period - minimum_block_period) * multiplier);
					}
				}
			}

			s64 time_left = m_dynamic_period - time_since_last_period;
			if (time_left > period_comparison_margin)
			{
				thread_ctrl::wait_for(get_thread_wait_delay(time_left));
				continue;
			}

			// Fast path for 0 ports active
			if (active_ports == 0)
			{
				// no need to mix, just enqueue silence and advance time
				cellAudio.error("advancing time: no active ports, enqueued_buffers=%llu", enqueued_buffers);
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
			if (
				(untouched == active_ports) || // wait for at least one touched buffer
				(untouched > (untouched_expected > 0 ? active_ports - untouched_expected : 0)) // with multiple audio ports, wait for the expected number of buffers to be touched
			   )
			{
				if (!playing)
				{
					// We ran out of buffer, probably because we waited too long
					// Don't enqueue anything, just advance time
					cellAudio.error("advancing time: untouched=%u/%u (expected=%u), enqueued_buffers=0", untouched, active_ports, untouched_expected);
					untouched_expected = untouched;
					advance(timestamp);
				}
				else
				{
					cellAudio.error("waiting: untouched=%u/%u (expected=%u), enqueued_buffers=%llu", untouched, active_ports, untouched_expected, enqueued_buffers);
					thread_ctrl::wait_for(1000);
				}

				continue;
			}
			
			// Wait for buffer(s) to be completely filled
			if (in_progress > 0)
			{
				cellAudio.error("waiting: in_progress=%u/%u, enqueued_buffers=%u", in_progress, active_ports, enqueued_buffers);
				thread_ctrl::wait_for(500);
				continue;
			}

			//cellAudio.error("time_since_last=%llu, dynamic_period=%llu => %3.2f%%", time_since_last_period, m_dynamic_period, (((f32)m_dynamic_period) / audio_block_period) * 100);
			//cellAudio.error("active=%u, untouched=%u, in_progress=%d, incomplete=%d, enqueued_buffers=%u", active_ports, untouched, in_progress, incomplete, enqueued_buffers);

			// Store number of untouched buffers for future reference
			untouched_expected = untouched;

			// Warn if we enqueued untouched/incomplete buffers
			if (untouched > 0 || incomplete > 0)
			{
				cellAudio.error("enqueueing: untouched=%u/%u (expected=%u), incomplete=%u/%u enqueued_buffers=%llu", untouched, active_ports, untouched_expected, incomplete, active_ports, enqueued_buffers);
			}

			// Handle audio restart
			if (!playing)
			{
				// We are not playing (likely buffer underrun)
				// align to 5.(3)ms on global clock
				const s64 audio_period_alignment_delta = (timestamp - m_start_time) % audio_block_period;
				if (audio_period_alignment_delta > period_comparison_margin)
				{
					thread_ctrl::wait_for(audio_period_alignment_delta - period_comparison_margin);
				}

				// Flush, add silence, restart algorithm
				cellAudio.error("play/resume audio: received first audio buffer");
				ringbuffer->flush();
				ringbuffer->enqueue_silence(desired_full_buffers);
				finish_port_volume_stepping();
			}
		}

		// Mix
		float *buf = ringbuffer->get_current_buffer();
		if (audio_channels == 2)
		{
			mix<true>(buf);
		}
		else if (audio_channels == 8)
		{
			mix<false>(buf);
		}
		else
		{
			fmt::throw_exception("Unsupported number of audio channels: %u", audio_channels);
		}

		// Enqueue
		ringbuffer->enqueue();

		// Advance time
		advance(timestamp);
	}

	// Destroy ringbuffer
	ringbuffer.reset();
}

template<bool downmix_to_2ch>
void cell_audio_thread::mix(float *out_buffer, s32 offset)
{
	AUDIT(out_buffer != nullptr);

	constexpr u32 channels = downmix_to_2ch ? 2 : 8;
	constexpr u32 out_buffer_sz = channels * AUDIO_BUFFER_SAMPLES;

	bool first_mix = true;

	// mixing
	for (auto& port : ports)
	{
		if (port.state != audio_port_state::started) continue;

		if (buffering_enabled)
		{
			port.apply_tag_backups(offset);
		}

		auto buf = port.get_vm_ptr(offset);
		static const float k = 1.0f;
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

					if (!downmix_to_2ch)
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

					if (downmix_to_2ch)
					{
						const float mid = (center + low_freq) * 0.708f;
						out_buffer[out + 0] = (left + rear_left + side_left + mid) * k;
						out_buffer[out + 1] = (right + rear_right + side_right + mid) * k;
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

					if (downmix_to_2ch)
					{
						const float mid = (center + low_freq) * 0.708f;
						out_buffer[out + 0] += (left + rear_left + side_left + mid) * k;
						out_buffer[out + 1] += (right + rear_right + side_right + mid) * k;
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
			// TODO ruipin: Revisit this
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

	const auto buf = vm::cast(vm::alloc(AUDIO_PORT_OFFSET * AUDIO_PORT_COUNT, vm::main));
	const auto ind = vm::cast(vm::alloc(sizeof(u64) * AUDIO_PORT_COUNT, vm::main));

	// Start audio thread
	auto g_audio = g_idm->lock<named_thread<cell_audio_thread>>(id_new);

	if (!g_audio)
	{
		vm::dealloc(buf);
		vm::dealloc(ind);
		return CELL_AUDIO_ERROR_ALREADY_INIT;
	}

	g_audio.create("cellAudio Thread", buf, ind);
	return CELL_OK;
}

error_code cellAudioQuit(ppu_thread& ppu)
{
	cellAudio.warning("cellAudioQuit()");

	// Stop audio thread
	auto g_audio = g_idm->lock<named_thread<cell_audio_thread>>(0);

	if (!g_audio)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	// Signal to abort, release lock
	*g_audio.get() = thread_state::aborting;
	g_audio.unlock();

	while (true)
	{
		if (ppu.is_stopped())
		{
			return 0;
		}

		thread_ctrl::wait_for(1000);

		auto g_audio = g_idm->lock<named_thread<cell_audio_thread>>(0);

		if (*g_audio.get() == thread_state::finished)
		{
			const auto buf = g_audio->ports[0].addr;
			const auto ind = g_audio->ports[0].index;
			g_audio.destroy();
			g_audio.unlock();
			vm::dealloc(buf.addr());
			vm::dealloc(ind.addr());
			break;
		}
	}

	return CELL_OK;
}

error_code cellAudioPortOpen(vm::ptr<CellAudioPortParam> audioParam, vm::ptr<u32> portNum)
{
	cellAudio.warning("cellAudioPortOpen(audioParam=*0x%x, portNum=*0x%x)", audioParam, portNum);

	const auto g_audio = g_idm->lock<named_thread<cell_audio_thread>>(0);

	if (!g_audio)
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

	const auto g_audio = g_idm->lock<named_thread<cell_audio_thread>>(0);

	if (!g_audio)
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
	default: fmt::throw_exception("Invalid port state (%d: %d)", portNum, (u32)state);
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

	const auto g_audio = g_idm->lock<named_thread<cell_audio_thread>>(0);

	if (!g_audio)
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
	default: fmt::throw_exception("Invalid port state (%d: %d)", portNum, (u32)state);
	}
}

error_code cellAudioPortClose(u32 portNum)
{
	cellAudio.warning("cellAudioPortClose(portNum=%d)", portNum);

	const auto g_audio = g_idm->lock<named_thread<cell_audio_thread>>(0);

	if (!g_audio)
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
	default: fmt::throw_exception("Invalid port state (%d: %d)", portNum, (u32)state);
	}
}

error_code cellAudioPortStop(u32 portNum)
{
	cellAudio.warning("cellAudioPortStop(portNum=%d)", portNum);

	const auto g_audio = g_idm->lock<named_thread<cell_audio_thread>>(0);

	if (!g_audio)
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
	default: fmt::throw_exception("Invalid port state (%d: %d)", portNum, (u32)state);
	}
}

error_code cellAudioGetPortTimestamp(u32 portNum, u64 tag, vm::ptr<u64> stamp)
{
	cellAudio.trace("cellAudioGetPortTimestamp(portNum=%d, tag=0x%llx, stamp=*0x%x)", portNum, tag, stamp);

	const auto g_audio = g_idm->lock<named_thread<cell_audio_thread>>(0);

	if (!g_audio)
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
	u64 delta_tag_stamp = delta_tag * g_audio->audio_block_period;
	*stamp = port.timestamp - delta_tag_stamp;

	return CELL_OK;
}

error_code cellAudioGetPortBlockTag(u32 portNum, u64 blockNo, vm::ptr<u64> tag)
{
	cellAudio.trace("cellAudioGetPortBlockTag(portNum=%d, blockNo=0x%llx, tag=*0x%x)", portNum, blockNo, tag);

	const auto g_audio = g_idm->lock<named_thread<cell_audio_thread>>(0);

	if (!g_audio)
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

	const auto g_audio = g_idm->lock<named_thread<cell_audio_thread>>(0);

	if (!g_audio)
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

error_code cellAudioCreateNotifyEventQueue(vm::ptr<u32> id, vm::ptr<u64> key)
{
	cellAudio.warning("cellAudioCreateNotifyEventQueue(id=*0x%x, key=*0x%x)", id, key);

	vm::var<sys_event_queue_attribute_t> attr;
	attr->protocol = SYS_SYNC_FIFO;
	attr->type     = SYS_PPU_QUEUE;
	attr->name_u64 = 0;

	for (u64 i = 0; i < MAX_AUDIO_EVENT_QUEUES; i++)
	{
		// Create an event queue "bruteforcing" an available key
		const u64 key_value = 0x80004d494f323221ull + i;

		if (const s32 res = sys_event_queue_create(id, attr, key_value, 32))
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

error_code cellAudioCreateNotifyEventQueueEx(vm::ptr<u32> id, vm::ptr<u64> key, u32 iFlags)
{
	cellAudio.todo("cellAudioCreateNotifyEventQueueEx(id=*0x%x, key=*0x%x, iFlags=0x%x)", id, key, iFlags);

	if (iFlags & ~CELL_AUDIO_CREATEEVENTFLAG_SPU)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	// TODO

	return CELL_AUDIO_ERROR_EVENT_QUEUE;
}

error_code cellAudioSetNotifyEventQueue(u64 key)
{
	cellAudio.warning("cellAudioSetNotifyEventQueue(key=0x%llx)", key);

	const auto g_audio = g_idm->lock<named_thread<cell_audio_thread>>(0);

	if (!g_audio)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	for (auto k : g_audio->keys) // check for duplicates
	{
		if (k == key)
		{
			return CELL_AUDIO_ERROR_TRANS_EVENT;
		}
	}

	g_audio->keys.emplace_back(key);

	return CELL_OK;
}

error_code cellAudioSetNotifyEventQueueEx(u64 key, u32 iFlags)
{
	cellAudio.todo("cellAudioSetNotifyEventQueueEx(key=0x%llx, iFlags=0x%x)", key, iFlags);

	// TODO

	return CELL_OK;
}

error_code cellAudioRemoveNotifyEventQueue(u64 key)
{
	cellAudio.warning("cellAudioRemoveNotifyEventQueue(key=0x%llx)", key);

	const auto g_audio = g_idm->lock<named_thread<cell_audio_thread>>(0);

	if (!g_audio)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	for (auto i = g_audio->keys.begin(); i != g_audio->keys.end(); i++)
	{
		if (*i == key)
		{
			g_audio->keys.erase(i);

			return CELL_OK;
		}
	}

	return CELL_AUDIO_ERROR_TRANS_EVENT;
}

error_code cellAudioRemoveNotifyEventQueueEx(u64 key, u32 iFlags)
{
	cellAudio.todo("cellAudioRemoveNotifyEventQueueEx(key=0x%llx, iFlags=0x%x)", key, iFlags);

	// TODO

	return CELL_OK;
}

error_code cellAudioAddData(u32 portNum, vm::ptr<float> src, u32 samples, float volume)
{
	cellAudio.trace("cellAudioAddData(portNum=%d, src=*0x%x, samples=%d, volume=%f)", portNum, src, samples, volume);

	auto g_audio = g_idm->lock<named_thread<cell_audio_thread>>(0);

	if (!g_audio)
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

	g_audio.unlock();

	for (u32 i = 0; i < samples * port.num_channels; i++)
	{
		dst[i] += src[i] * volume; // mix all channels
	}

	return CELL_OK;
}

error_code cellAudioAdd2chData(u32 portNum, vm::ptr<float> src, u32 samples, float volume)
{
	cellAudio.trace("cellAudioAdd2chData(portNum=%d, src=*0x%x, samples=%d, volume=%f)", portNum, src, samples, volume);

	auto g_audio = g_idm->lock<named_thread<cell_audio_thread>>(0);

	if (!g_audio)
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

	g_audio.unlock();

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

	auto g_audio = g_idm->lock<named_thread<cell_audio_thread>>(0);

	if (!g_audio)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (portNum >= AUDIO_PORT_COUNT || !src || !src.aligned())
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	const audio_port& port = g_audio->ports[portNum];

	const auto dst = port.get_vm_ptr();

	g_audio.unlock();

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
