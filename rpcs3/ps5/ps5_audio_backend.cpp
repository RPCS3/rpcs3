// PS5: RPCS3's audio on the console's own output (ps5_audio_backend.h).
//
// libSceAudioOut as PS5_VulkanTemplate's platform layer drives it
// (skills/ps5-homebrew/references/platform-contracts.md, "Audio"): a main
// port opened for the system user, 256-frame grains at 48 kHz in signed
// 16-bit stereo, sceAudioOutOutput blocking for one grain. RPCS3 opens one
// backend for cellAudio and one for RSXAudio, and the console mixes the
// ports.

#include "stdafx.h"
#include "ps5_audio_backend.h"
#include "Utilities/Thread.h"
#include "Emu/system_config.h"

#include <algorithm>
#include <chrono>
#include <pthread.h>
#include <sched.h>
#include <cmath>
#include <cstring>

LOG_CHANNEL(ps5_audio, "PS5Audio");

// cellAudio.cpp: the audio periods the game was late for (its own stutter)
extern atomic_t<u32> g_ps5_audio_periods_skipped;
extern atomic_t<u32> g_ps5_audio_periods_partial;
extern atomic_t<u32> g_ps5_audio_periods_saved;

extern "C"
{
	int sceAudioOutInit(void);
	int sceAudioOutOpen(s32 user, s32 type, s32 index, u32 grain, u32 rate, u32 format);
	int sceAudioOutOutput(s32 port, const void* samples);
	int sceAudioOutClose(s32 port);
	int sceKernelUsleep(u32 microseconds);
}

namespace
{
	constexpr u32 c_grain = 256;          // frames the console takes at a time
	constexpr u32 c_rate = 48000;         // the only rate it takes
	constexpr s32 c_system_user = 0xff;   // SCE_USER_SERVICE_USER_ID_SYSTEM
	constexpr s32 c_port_main = 0;        // SCE_AUDIO_OUT_PORT_TYPE_MAIN
	constexpr u32 c_format_s16_stereo = 1;
	constexpr int c_already_initialised = static_cast<int>(0x8026000e);

	s16 to_s16(f32 value)
	{
		return static_cast<s16>(std::clamp(value * 32767.0f, -32768.0f, 32767.0f));
	}
}

ps5_audio_backend::~ps5_audio_backend()
{
	Close();
}

bool ps5_audio_backend::Open(std::string_view /* dev_id */, AudioFreq freq, AudioSampleSize sample_size, AudioChannelCnt ch_cnt, audio_channel_layout layout)
{
	Close();

	m_sampling_rate = freq;
	m_sample_size = sample_size;
	// The output is stereo: RPCS3 downmixes to it (cellAudio, RSXAudio)
	setup_channel_layout(static_cast<u32>(ch_cnt), 2, layout, ps5_audio);

	if (const int init = sceAudioOutInit(); init < 0 && init != c_already_initialised)
	{
		ps5_audio.error("sceAudioOutInit failed: 0x%08x", static_cast<u32>(init));
		return false;
	}

	m_port = sceAudioOutOpen(c_system_user, c_port_main, 0, c_grain, c_rate, c_format_s16_stereo);
	if (m_port < 0)
	{
		ps5_audio.error("sceAudioOutOpen failed: 0x%08x", static_cast<u32>(m_port));
		m_port = -1;
		return false;
	}

	m_last[0] = m_last[1] = 0.0f;
	m_phase = 0.0;
	m_errors = 0;
	m_running = true;
	m_thread = std::thread([this]() { output_loop(); });

	ps5_audio.notice("Opened port %d: %u Hz in, %u channels (layout %s), %s samples", m_port, static_cast<u32>(freq), m_channels, m_layout,
		sample_size == AudioSampleSize::S16 ? "16-bit" : "float");
	return true;
}

void ps5_audio_backend::Close()
{
	if (m_thread.joinable())
	{
		m_running = false;
		m_thread.join();
	}

	if (m_port >= 0)
	{
		sceAudioOutClose(m_port);
		m_port = -1;
	}

	std::lock_guard lock(m_cb_mutex);
	m_playing = false;
}

f64 ps5_audio_backend::GetCallbackFrameLen()
{
	return static_cast<f64>(c_grain) / c_rate;
}

void ps5_audio_backend::Play()
{
	std::lock_guard lock(m_cb_mutex);
	m_playing = true;
}

void ps5_audio_backend::Pause()
{
	std::lock_guard lock(m_cb_mutex);
	m_playing = false;
}

void ps5_audio_backend::output_loop()
{
	alignas(64) s16 grain[c_grain * 2];

	// Above the emulator's threads, as cellAudio's own (set_native_priority:
	// on the console a lower number is a higher priority). Said once, with
	// what the console made of it
	{
		thread_ctrl::set_native_priority(1);
		int policy = 0;
		sched_param param{};
		pthread_getschedparam(pthread_self(), &policy, &param);
		ps5_audio.notice("Port %d's thread: policy %d, priority %d (256 highest, 767 lowest)", m_port, policy, param.sched_priority);
	}

	// Every ten seconds while sound plays: the grains, those the callback
	// could not fill (the emulator behind), and the longest wait between two
	// grains taken (this thread behind; one grain is 5.3 ms)
	using clock = std::chrono::steady_clock;
	u32 grains = 0, short_reads = 0, empty = 0;
	s64 longest_us = 0;
	auto window = clock::now();
	auto last = window;

	while (m_running)
	{
		const fill_result result = fill(grain);

		// Blocks for one grain: the console's pace is the stream's clock. An
		// output that stops taking grains is said once, not left to spin
		if (sceAudioOutOutput(m_port, grain) < 0)
		{
			if (m_errors++ == 0)
			{
				ps5_audio.error("sceAudioOutOutput failed on port %d", m_port);
			}
			sceKernelUsleep(5000);
		}

		const auto now = clock::now();
		const bool playing = m_playing;
		if (playing)
		{
			grains++;
			short_reads += result == fill_result::short_read;
			empty += result == fill_result::empty;
			longest_us = std::max<s64>(longest_us, std::chrono::duration_cast<std::chrono::microseconds>(now - last).count());
		}
		last = now;

		if (now - window >= std::chrono::seconds(10))
		{
			if (grains)
			{
				// And the game's side: periods of its audio skipped or mixed partly
				// silent because it was late (RPCS3's sampling skip), and those
				// Late Audio Wait waited for
				ps5_audio.notice("Port %d: %u grains in 10 s, %u short and %u empty from the emulator, longest wait %.1f ms; the game late for %u periods (skipped), %u more partly silent, %u saved by Late Audio Wait (%u ms)",
					m_port, grains, short_reads, empty, longest_us / 1000., g_ps5_audio_periods_skipped.exchange(0), g_ps5_audio_periods_partial.exchange(0),
					g_ps5_audio_periods_saved.exchange(0), g_cfg.audio.late_audio_wait.get());
			}
			grains = short_reads = empty = 0;
			longest_us = 0;
			window = now;
		}
	}

	// The grain still queued
	sceAudioOutOutput(m_port, nullptr);
}

ps5_audio_backend::fill_result ps5_audio_backend::fill(s16* out)
{
	const u32 sample_bytes = m_sample_size == AudioSampleSize::S16 ? 2 : 4;
	const u32 frame_bytes = sample_bytes * 2;
	const u32 in_rate = static_cast<u32>(m_sampling_rate);
	const f64 step = static_cast<f64>(in_rate) / c_rate;

	// Frames to read for one grain: all of them at 48 kHz, else as many as the
	// resampler steps over (the phase carries the fraction to the next grain)
	const u32 frames = in_rate == c_rate ? c_grain : static_cast<u32>(std::floor(m_phase + c_grain * step));
	m_in.resize(std::max<usz>(m_in.size(), usz{frames} * frame_bytes));

	u32 written = 0;
	{
		std::unique_lock lock(m_cb_mutex, std::defer_lock);
		if (lock.try_lock_for(std::chrono::microseconds{50}) && m_playing && m_write_callback && frames)
		{
			written = std::min(m_write_callback(frames * frame_bytes, m_in.data()), frames * frame_bytes);
		}
	}

	if (!written)
	{
		std::memset(out, 0, c_grain * 2 * sizeof(s16));
		return fill_result::empty;
	}
	const fill_result result = written < frames * frame_bytes ? fill_result::short_read : fill_result::full;

	const u32 got = written / frame_bytes;
	const auto sample = [&](u32 frame, u32 channel) -> f32
	{
		// Short reads hold the last frame read, as cubeb's backend does
		frame = std::min(frame, got - 1);
		if (sample_bytes == 2)
		{
			s16 value;
			std::memcpy(&value, m_in.data() + frame * frame_bytes + channel * 2, 2);
			return value / 32768.0f;
		}
		f32 value;
		std::memcpy(&value, m_in.data() + frame * frame_bytes + channel * 4, 4);
		return value;
	};

	if (in_rate == c_rate)
	{
		for (u32 i = 0; i < c_grain; i++)
		{
			out[i * 2] = to_s16(sample(i, 0));
			out[i * 2 + 1] = to_s16(sample(i, 1));
		}
		return result;
	}

	// Linear between the frames read, the last grain's last frame first
	f64 position = m_phase;
	for (u32 i = 0; i < c_grain; i++, position += step)
	{
		const u32 index = static_cast<u32>(position);
		const f32 t = static_cast<f32>(position - index);
		for (u32 channel = 0; channel < 2; channel++)
		{
			const f32 a = index == 0 ? m_last[channel] : sample(index - 1, channel);
			const f32 b = sample(index, channel);
			out[i * 2 + channel] = to_s16(a + (b - a) * t);
		}
	}

	m_phase = position - frames;
	m_last[0] = sample(frames - 1, 0);
	m_last[1] = sample(frames - 1, 1);
	return result;
}
