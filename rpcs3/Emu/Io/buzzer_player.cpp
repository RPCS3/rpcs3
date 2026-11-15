#include "stdafx.h"
#include "buzzer_player.h"
#include "Emu/Audio/AudioBackend.h"
#include "Emu/emu_callbacks.h"
#include "Emu/system_config.h"

#include <atomic>
#include <cmath>
#include <mutex>
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
		const std::vector<f32> scale = generate_scale_wave();

		static const buzzer_waveforms& get()
		{
			static const buzzer_waveforms s_waveforms;
			return s_waveforms;
		}
	};

	struct buzzer_voice
	{
		u64 owner_id = 0;
		std::span<const f32> wave{};
		usz wave_pos = 0;
		steady_clock::time_point start_time{};
	};

	// Central audio output engine shared across all buzzer_player instances.
	// Mixes multiple concurrent buzzer voices, handles audio backend lifecycle,
	// settings changes, device hotplugging, and device disconnection.
	class buzzer_audio_manager
	{
	public:
		static buzzer_audio_manager& get()
		{
			static buzzer_audio_manager s_manager;
			return s_manager;
		}

		u64 register_instance()
		{
			static std::atomic<u64> s_next_id{1};
			const u64 id = s_next_id.fetch_add(1, std::memory_order_relaxed);
			{
				std::lock_guard lock(m_instances_mutex);
				m_registered_instances.push_back(id);
			}
			return id;
		}

		void unregister_instance(u64 owner_id)
		{
			stop(owner_id);

			bool should_close = false;
			{
				std::lock_guard lock(m_instances_mutex);
				std::erase(m_registered_instances, owner_id);
				if (m_registered_instances.empty())
				{
					should_close = true;
				}
			}

			if (should_close)
			{
				std::lock_guard lock(m_init_mutex);
				close_backend();
			}
		}

		void play(u64 owner_id, std::span<const f32> wave)
		{
			if (wave.empty())
			{
				return;
			}

			ensure_backend();

			const auto now = steady_clock::now();

			std::lock_guard lock(m_playback_mutex);

			// If this specific instance already has an active voice, replace it
			for (auto& v : m_voices)
			{
				if (v.owner_id == owner_id)
				{
					v.wave = wave;
					v.wave_pos = 0;
					v.start_time = now;
					return;
				}
			}

			// Add a new voice that will mix concurrently with other instances
			m_voices.push_back({
				.owner_id = owner_id,
				.wave = wave,
				.wave_pos = 0,
				.start_time = now,
			});
		}

		void stop(u64 owner_id)
		{
			std::lock_guard lock(m_playback_mutex);
			std::erase_if(m_voices, [owner_id](const buzzer_voice& v)
				{
					return v.owner_id == owner_id;
				});
		}

		bool is_playing(u64 owner_id) const
		{
			const auto now = steady_clock::now();
			std::lock_guard lock(m_playback_mutex);
			for (const auto& v : m_voices)
			{
				if (v.owner_id == owner_id)
				{
					const usz duration_ms = (v.wave.size() * 1000) / BUZZER_SAMPLE_RATE;
					if (now - v.start_time <= std::chrono::milliseconds(duration_ms))
					{
						return true;
					}
				}
			}
			return false;
		}

		void check_device()
		{
			// Clean up voices older than 5 seconds or voices that have passed their natural duration
			const auto now = steady_clock::now();
			{
				std::lock_guard lock(m_playback_mutex);
				std::erase_if(m_voices, [now](const buzzer_voice& v)
					{
						if (now - v.start_time > 5s)
						{
							return true;
						}
						if (v.wave_pos >= v.wave.size())
						{
							return true;
						}
						const usz duration_ms = (v.wave.size() * 1000) / BUZZER_SAMPLE_RATE;
						if (now - v.start_time > std::chrono::milliseconds(duration_ms + 200))
						{
							return true;
						}
						return false;
					});
			}

			ensure_backend();
		}

	private:
		buzzer_audio_manager() = default;

		void close_backend()
		{
			if (m_backend)
			{
				m_backend->SetWriteCallback(nullptr);
				m_backend->SetStateCallback(nullptr);
				m_backend->Close();
				m_backend.reset();
			}
		}

		bool ensure_backend()
		{
			std::lock_guard lock(m_init_mutex);

			const audio_renderer cur_renderer = g_cfg.audio.renderer.get();
			const std::string cur_device = g_cfg.audio.audio_device.get();

			const bool renderer_changed = (m_backend && m_opened_renderer != cur_renderer);
			const bool device_setting_changed = (m_backend && m_opened_device != cur_device);
			const bool hw_device_changed = m_device_changed.load(std::memory_order_acquire) || (m_backend && m_backend->DefaultDeviceChanged());
			const bool backend_broken = (m_backend && !m_backend->Operational());
			const bool is_null_backend = (m_backend && m_backend->GetName() == "Null"sv);
			const bool should_retry_from_null = is_null_backend && (cur_renderer != audio_renderer::null);

			const auto now = steady_clock::now();

			// If backend is healthy and configuration has not changed, reuse it
			if (m_backend && !renderer_changed && !device_setting_changed && !hw_device_changed && !backend_broken && !should_retry_from_null)
			{
				return true;
			}

			// Rate-limit retries if recent initialization failed to prevent log spamming and busy loops
			if (!m_backend && now - m_last_init_fail_time < 500ms)
			{
				return false;
			}

			if (should_retry_from_null && now - m_last_init_fail_time < 1s)
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
				m_last_init_fail_time = now;
				return false;
			}

			// If configured renderer is not Null, but get_audio returned Null backend, record failure timestamp
			if (cur_renderer != audio_renderer::null && m_backend->GetName() == "Null"sv)
			{
				m_last_init_fail_time = now;
			}

			m_backend->SetStateCallback([this](AudioStateEvent)
				{
					m_device_changed.store(true, std::memory_order_release);
				});

			m_backend->SetWriteCallback([this](u32 byte_cnt, void* buffer) -> u32
				{
					return on_write_callback(byte_cnt, buffer);
				});

			m_opened_device = cur_device;
			m_opened_renderer = cur_renderer;

			if (!m_backend->Open(cur_device, AudioFreq::FREQ_48K, AudioSampleSize::FLOAT, AudioChannelCnt::STEREO, audio_channel_layout::stereo))
			{
				close_backend();
				m_last_init_fail_time = now;
				return false;
			}

			m_backend->Play();
			return true;
		}

		u32 on_write_callback(u32 byte_cnt, void* buffer)
		{
			constexpr u32 frame_size = sizeof(f32) * CHANNELS;
			if (!buffer || byte_cnt < frame_size)
			{
				if (buffer && byte_cnt > 0)
				{
					std::memset(buffer, 0, byte_cnt);
				}
				return byte_cnt;
			}

			const usz total_samples = byte_cnt / sizeof(f32);
			auto* const out = static_cast<f32*>(buffer);
			const u32 req_frames = static_cast<u32>(total_samples / CHANNELS);

			std::memset(buffer, 0, byte_cnt);

			std::lock_guard lock(m_playback_mutex);

			if (m_voices.empty())
			{
				return byte_cnt;
			}

			for (auto it = m_voices.begin(); it != m_voices.end();)
			{
				auto& voice = *it;
				const usz remaining = voice.wave.size() - voice.wave_pos;
				const u32 to_write = static_cast<u32>(std::min<usz>(remaining, req_frames));

				for (u32 i = 0; i < to_write; ++i)
				{
					if ((static_cast<usz>(i) + 1) * frame_size > byte_cnt)
					{
						break;
					}

					const f32 sample = voice.wave[voice.wave_pos++];
					out[i * 2 + 0] += sample;
					out[i * 2 + 1] += sample;
				}

				if (voice.wave_pos >= voice.wave.size())
				{
					it = m_voices.erase(it);
				}
				else
				{
					++it;
				}
			}

			for (usz i = 0; i < total_samples; ++i)
			{
				if ((i + 1) * sizeof(f32) > byte_cnt)
				{
					break;
				}
				out[i] = std::clamp(out[i], -1.0f, 1.0f);
			}

			return byte_cnt;
		}

		mutable std::mutex m_instances_mutex;
		std::vector<u64> m_registered_instances;

		std::mutex m_init_mutex;
		std::shared_ptr<AudioBackend> m_backend;
		std::string m_opened_device;
		audio_renderer m_opened_renderer{audio_renderer::null};
		std::atomic<bool> m_device_changed{false};
		steady_clock::time_point m_last_init_fail_time{};

		mutable std::mutex m_playback_mutex;
		std::vector<buzzer_voice> m_voices;
	};
} // anonymous namespace

buzzer_player::buzzer_player()
	: m_instance_id(buzzer_audio_manager::get().register_instance())
{
}

buzzer_player::~buzzer_player()
{
	buzzer_audio_manager::get().unregister_instance(m_instance_id);
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
	buzzer_audio_manager::get().stop(m_instance_id);
}

bool buzzer_player::is_playing() const
{
	return buzzer_audio_manager::get().is_playing(m_instance_id);
}

void buzzer_player::check_device()
{
	buzzer_audio_manager::get().check_device();
}

void buzzer_player::play(std::span<const f32> wave)
{
	buzzer_audio_manager::get().play(m_instance_id, wave);
}
