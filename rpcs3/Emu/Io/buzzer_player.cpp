#include "stdafx.h"
#include "buzzer_player.h"
#include "Emu/Audio/AudioBackend.h"
#include "Emu/emu_callbacks.h"
#include "Emu/system_config.h"

#include <cmath>
#include <numbers>

namespace
{
	constexpr u32 BUZZER_SAMPLE_RATE = 48000;
	constexpr f32 BUZZER_VOLUME = 0.45f;
	constexpr u32 CHANNELS = 2;

	// Synthesizes a single sample with piezoelectric buzzer cavity resonance odd harmonics (h = 1, 3, 5, 7, 9)
	f64 synthesize_buzzer_sample(f64 phase, f64 freq)
	{
		f64 wave = 0.0;
		for (const u32 h : {1u, 3u, 5u, 7u, 9u})
		{
			const f64 h_freq = freq * h;
			if (h_freq >= 0.45 * BUZZER_SAMPLE_RATE)
			{
				break;
			}

			// ~4.5 kHz cavity resonance boost
			const f64 diff = (h_freq - 4500.0) / 1400.0;
			const f64 boost = 1.0 + 2.2 * std::exp(-(diff * diff));
			wave += (boost / h) * std::sin(h * phase);
		}

		return wave;
	}

	// Normalizes waveform peak to target volume and applies linear fade envelope to eliminate speaker pop/click transients
	void finalize_waveform(std::vector<f32>& wave, f32 target_volume, usz fade_samples)
	{
		f32 peak = 1e-9f;
		for (const f32 s : wave)
		{
			peak = std::max(peak, std::abs(s));
		}

		const f32 norm = target_volume / peak;
		for (f32& s : wave)
		{
			s *= norm;
		}

		fade_samples = std::min(fade_samples, wave.size() / 2);
		for (usz i = 0; i < fade_samples; ++i)
		{
			const f32 factor = static_cast<f32>(i) / static_cast<f32>(fade_samples);
			wave[i] *= factor;
			wave[wave.size() - 1 - i] *= factor;
		}
	}

	// 4186 Hz (NOTE_C8), 60 ms tone, 30 ms interval silence, 2 ms fade envelope
	std::vector<f32> generate_beep_wave(u32 count)
	{
		if (count == 0)
		{
			return {};
		}

		constexpr u32 freq = 4186; // NOTE_C8 (~4.2 kHz buzzer resonance peak)
		const usz tone_samples = BUZZER_SAMPLE_RATE * 60 / 1000;
		const usz interval_samples = BUZZER_SAMPLE_RATE * 30 / 1000;

		std::vector<f32> tone(tone_samples);
		for (usz i = 0; i < tone_samples; ++i)
		{
			const f64 phase = 2.0 * std::numbers::pi * freq * i / BUZZER_SAMPLE_RATE;
			tone[i] = static_cast<f32>(synthesize_buzzer_sample(phase, freq));
		}
		finalize_waveform(tone, BUZZER_VOLUME, BUZZER_SAMPLE_RATE * 2 / 1000);

		if (count == 1)
		{
			return tone;
		}

		std::vector<f32> wave;
		wave.reserve(count * tone_samples + (count - 1) * interval_samples);

		for (u32 i = 0; i < count; ++i)
		{
			wave.insert(wave.end(), tone.begin(), tone.end());
			if (i < count - 1)
			{
				wave.insert(wave.end(), interval_samples, 0.0f);
			}
		}

		return wave;
	}

	// NOTE_C5 ~ NOTE_C6 integer frequencies: 523, 587, 659, 698, 784, 880, 988, 1047 Hz
	// 250 ms per note (2.0s total), continuous phase integration (0 ms seamless note transitions),
	// 4 ms initial/final fade envelope, piezoelectric buzzer harmonic timbre
	std::vector<f32> generate_scale_wave()
	{
		constexpr std::array<u32, 8> scale_freqs = {523, 587, 659, 698, 784, 880, 988, 1047};
		const usz note_samples = BUZZER_SAMPLE_RATE * 250 / 1000;

		std::vector<f32> wave(note_samples * scale_freqs.size());
		f64 phase = 0.0;
		usz idx = 0;

		for (const u32 freq : scale_freqs)
		{
			const f64 step = 2.0 * std::numbers::pi * freq / BUZZER_SAMPLE_RATE;
			for (usz i = 0; i < note_samples; ++i)
			{
				phase = std::fmod(phase + step, 2.0 * std::numbers::pi);
				wave[idx++] = static_cast<f32>(synthesize_buzzer_sample(phase, freq));
			}
		}

		finalize_waveform(wave, BUZZER_VOLUME, BUZZER_SAMPLE_RATE * 4 / 1000);
		return wave;
	}

	// Shared static waveform cache to avoid duplicate allocations across instances
	struct buzzer_waveforms
	{
		const std::vector<f32> beep_1 = generate_beep_wave(1);
		const std::vector<f32> beep_3 = generate_beep_wave(3);
		const std::vector<f32> scale  = generate_scale_wave();

		static const buzzer_waveforms& get()
		{
			static const buzzer_waveforms s_waveforms;
			return s_waveforms;
		}
	};
} // anonymous namespace

buzzer_player::buzzer_player() = default;

buzzer_player::~buzzer_player()
{
	stop();

	std::lock_guard lock(m_init_mutex);
	close_backend();
}

void buzzer_player::play_beep_1()
{
	play(buzzer_waveforms::get().beep_1);
}

void buzzer_player::play_beep_3()
{
	play(buzzer_waveforms::get().beep_3);
}

void buzzer_player::play_scale()
{
	play(buzzer_waveforms::get().scale);
}

void buzzer_player::stop()
{
	std::lock_guard lock(m_playback_mutex);
	m_wave = {};
	m_wave_pos = 0;
	m_is_playing.store(false, std::memory_order_release);
}

bool buzzer_player::is_playing() const
{
	return m_is_playing.load(std::memory_order_relaxed);
}

void buzzer_player::check_device()
{
	if (!m_is_playing.load(std::memory_order_acquire))
	{
		return;
	}

	// Safeguard: abort playback after 5 seconds to prevent endless retries if audio device fails permanently
	{
		std::lock_guard lock(m_playback_mutex);
		if (!m_is_playing.load(std::memory_order_relaxed))
		{
			return;
		}

		if (steady_clock::now() - m_play_start_time > 5s)
		{
			m_wave = {};
			m_wave_pos = 0;
			m_is_playing.store(false, std::memory_order_release);
			return;
		}
	}

	ensure_backend();
}

void buzzer_player::play(std::span<const f32> wave)
{
	if (wave.empty())
	{
		return;
	}

	if (!ensure_backend())
	{
		return;
	}

	std::lock_guard lock(m_playback_mutex);
	m_wave = wave;
	m_wave_pos = 0;
	m_play_start_time = steady_clock::now();
	m_is_playing.store(true, std::memory_order_release);
}

void buzzer_player::close_backend()
{
	if (m_backend)
	{
		m_backend->SetWriteCallback(nullptr);
		m_backend->SetStateCallback(nullptr);
		m_backend->Close();
		m_backend.reset();
	}
}

bool buzzer_player::ensure_backend()
{
	std::lock_guard lock(m_init_mutex);

	const std::string cur_device = g_cfg.audio.audio_device.get();

	if (m_backend && m_backend->Operational() && !m_backend->DefaultDeviceChanged()
		&& !m_device_changed.load(std::memory_order_acquire) && m_opened_device == cur_device)
	{
		return true;
	}

	if (!g_emu_callbacks.get_audio)
	{
		return false;
	}

	close_backend();

	m_device_changed.store(false, std::memory_order_release);

	m_backend = g_emu_callbacks.get_audio();
	if (!m_backend || !m_backend->Initialized())
	{
		m_backend.reset();
		return false;
	}

	m_backend->SetStateCallback([this](AudioStateEvent)
	{
		m_device_changed.store(true, std::memory_order_release);
	});

	m_backend->SetWriteCallback([this](u32 byte_cnt, void* buffer) -> u32
	{
		if (!buffer || byte_cnt == 0)
		{
			return 0;
		}

		auto* const out = static_cast<f32*>(buffer);
		constexpr u32 frame_size = sizeof(f32) * CHANNELS;
		const u32 req_frames = byte_cnt / frame_size;
		u32 frames_written = 0;

		{
			std::lock_guard lock(m_playback_mutex);

			if (!m_wave.empty() && m_wave_pos < m_wave.size())
			{
				const usz remaining = m_wave.size() - m_wave_pos;
				const u32 to_write = static_cast<u32>(std::min<usz>(remaining, req_frames));

				for (u32 i = 0; i < to_write; ++i)
				{
					const f32 sample = m_wave[m_wave_pos++];
					out[i * 2 + 0] = sample;
					out[i * 2 + 1] = sample;
				}
				frames_written = to_write;

				if (m_wave_pos >= m_wave.size())
				{
					m_wave = {};
					m_wave_pos = 0;
					m_is_playing.store(false, std::memory_order_release);
				}
			}
		}

		const u32 written_bytes = frames_written * frame_size;
		if (written_bytes < byte_cnt)
		{
			std::memset(static_cast<u8*>(buffer) + written_bytes, 0, byte_cnt - written_bytes);
		}

		return byte_cnt;
	});

	m_opened_device = cur_device;
	if (!m_backend->Open(cur_device, AudioFreq::FREQ_48K, AudioSampleSize::FLOAT, AudioChannelCnt::STEREO, audio_channel_layout::stereo))
	{
		close_backend();
		return false;
	}

	m_backend->Play();
	return true;
}
