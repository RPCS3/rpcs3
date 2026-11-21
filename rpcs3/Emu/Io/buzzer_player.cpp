#include "stdafx.h"
#include "buzzer_player.h"
#include "Emu/Audio/AudioBackend.h"
#include "Emu/emu_callbacks.h"
#include "Emu/system_config.h"
#include "Emu/IdManager.h"

#include <atomic>
#include <bit>
#include <cmath>
#include <mutex>
#include <numbers>

namespace
{
	constexpr u32 BUZZER_SAMPLE_RATE = 48000;
	constexpr f32 BUZZER_VOLUME = 0.5f;
	constexpr u32 CHANNELS = 2;

	// Fade envelope durations (cover enough zero-crossings at 523–4186 Hz)
	constexpr usz FADE_IN_FRAMES  = BUZZER_SAMPLE_RATE * 1 / 1000;   // 1.0 ms = 48 frames
	constexpr usz FADE_OUT_FRAMES = BUZZER_SAMPLE_RATE * 4 / 1000;   // 4.0 ms = 192 frames

	constexpr f64 TWO_PI = std::numbers::pi_v<f64> * 2;
	constexpr std::array<u32, 5> HARMONICS = {1u, 3u, 5u, 7u, 9u};

	// Precomputed per-frequency harmonic coefficients (avoids std::exp in inner loop)
	struct harmonic_coeffs
	{
		std::array<f64, 5> coeff{};
		u32 count = 0;

		constexpr harmonic_coeffs() = default;

		explicit harmonic_coeffs(f64 freq)
		{
			for (u32 i = 0; i < HARMONICS.size(); ++i)
			{
				const f64 h_freq = freq * HARMONICS[i];
				if (h_freq >= 0.45 * BUZZER_SAMPLE_RATE)
					break;
				const f64 diff = (h_freq - 4500.0) / 1400.0;
				coeff[i] = (1.0 + 2.2 * std::exp(-(diff * diff))) / HARMONICS[i];
				count = i + 1;
			}
		}
	};

	f64 synthesize_buzzer_sample(f64 phase, const harmonic_coeffs& hc)
	{
		f64 wave = 0.0;
		for (u32 i = 0; i < hc.count; ++i)
		{
			wave += hc.coeff[i] * std::sin(HARMONICS[i] * phase);
		}
		return wave;
	}

	// Normalize peak to BUZZER_VOLUME and apply fade-in/fade-out envelope to avoid clicks
	void finalize_waveform(std::vector<f32>& wave)
	{
		if (wave.empty())
		{
			return;
		}

		const usz total_frames = wave.size() / CHANNELS;

		f32 peak = 1e-9f;
		for (const f32 s : wave)
		{
			peak = std::max(peak, std::abs(s));
		}

		const f32 norm = BUZZER_VOLUME / peak;
		for (f32& s : wave)
		{
			s *= norm;
		}

		const usz fade_in = std::min(FADE_IN_FRAMES, total_frames / 2);
		const usz fade_out = std::min(FADE_OUT_FRAMES, total_frames / 2);

		// Linear fade-in (preserves percussive onset)
		for (usz i = 0; i < fade_in; ++i)
		{
			const f32 factor = static_cast<f32>(i) / static_cast<f32>(fade_in);
			const usz base = i * CHANNELS;
			wave[base + 0] *= factor;
			wave[base + 1] *= factor;
		}

		// Raised-cosine fade-out (zero-derivative endpoints avoid clicks)
		for (usz i = 0; i < fade_out; ++i)
		{
			const f32 factor = 0.5f * (1.0f - std::cos(std::numbers::pi_v<f32> * static_cast<f32>(i) / static_cast<f32>(fade_out)));
			const usz base = (total_frames - 1 - i) * CHANNELS;
			wave[base + 0] *= factor;
			wave[base + 1] *= factor;
		}
	}

	// Generate stereo-interleaved beep waveform: 4186 Hz (NOTE_C8), 60 ms tone, 30 ms gap
	std::vector<f32> generate_beep_wave(u32 count)
	{
		if (count == 0)
		{
			return {};
		}

		constexpr u32 freq = 4186; // NOTE_C8 (~4.2 kHz buzzer resonance peak)
		const usz tone_frames = BUZZER_SAMPLE_RATE * 60 / 1000;
		const usz interval_frames = BUZZER_SAMPLE_RATE * 30 / 1000;

		const harmonic_coeffs hc(freq);

		std::vector<f32> tone(tone_frames * CHANNELS);
		for (usz i = 0; i < tone_frames; ++i)
		{
			const f64 phase = TWO_PI * freq * i / BUZZER_SAMPLE_RATE;
			const f32 sample = static_cast<f32>(synthesize_buzzer_sample(phase, hc));
			tone[i * CHANNELS + 0] = sample;
			tone[i * CHANNELS + 1] = sample;
		}
		finalize_waveform(tone);

		if (count == 1)
		{
			return tone;
		}

		std::vector<f32> wave;
		wave.reserve((count * tone_frames + (count - 1) * interval_frames) * CHANNELS);

		for (u32 i = 0; i < count; ++i)
		{
			wave.insert(wave.end(), tone.begin(), tone.end());
			if (i < count - 1)
			{
				wave.insert(wave.end(), interval_frames * CHANNELS, 0.0f);
			}
		}

		return wave;
	}

	// Generate stereo-interleaved C5–C6 scale, 250 ms/note, continuous phase
	std::vector<f32> generate_scale_wave()
	{
		constexpr std::array<u32, 8> scale_freqs = {523, 587, 659, 698, 784, 880, 988, 1047};
		const usz note_frames = BUZZER_SAMPLE_RATE * 250 / 1000;

		std::vector<f32> wave(note_frames * scale_freqs.size() * CHANNELS);
		f64 phase = 0.0;
		usz idx = 0;

		for (const u32 freq : scale_freqs)
		{
			const f64 step = TWO_PI * freq / BUZZER_SAMPLE_RATE;
			const harmonic_coeffs hc(freq);
			for (usz i = 0; i < note_frames; ++i)
			{
				phase += step;
				if (phase >= TWO_PI) phase -= TWO_PI;
				const f32 sample = static_cast<f32>(synthesize_buzzer_sample(phase, hc));
				wave[idx++] = sample;
				wave[idx++] = sample;
			}
		}

		finalize_waveform(wave);
		return wave;
	}
} // anonymous namespace

// Precomputed stereo-interleaved waveform cache (g_fxo managed)
struct buzzer_waveforms
{
	std::vector<f32> beep_1 = generate_beep_wave(1);
	std::vector<f32> beep_3 = generate_beep_wave(3);
	std::vector<f32> scale = generate_scale_wave();

	buzzer_waveforms() = default;
	buzzer_waveforms(const buzzer_waveforms&) = delete;
	buzzer_waveforms& operator=(const buzzer_waveforms&) = delete;
};

struct buzzer_voice_slot
{
	std::atomic<bool> active{false};
	std::atomic<u32> wave_pos{0};
	std::atomic<const f32*> wave_data{nullptr};
	std::atomic<u32> wave_size{0};
	std::atomic<u64> owner_id{0};

	buzzer_voice_slot() = default;
	buzzer_voice_slot(const buzzer_voice_slot&) = delete;
	buzzer_voice_slot& operator=(const buzzer_voice_slot&) = delete;
};

// Shared audio engine for all buzzer_player instances (g_fxo managed).
// Mixes concurrent voices and handles backend lifecycle.
class buzzer_audio_manager
{
public:
	buzzer_audio_manager() = default;
	~buzzer_audio_manager()
	{
		close_backend();
	}

	buzzer_audio_manager(const buzzer_audio_manager&) = delete;
	buzzer_audio_manager& operator=(const buzzer_audio_manager&) = delete;

	u64 register_instance()
	{
		static std::atomic<u64> s_next_id{1};
		const u64 id = s_next_id.fetch_add(1, std::memory_order_relaxed);
		m_registered_instance_count.fetch_add(1, std::memory_order_relaxed);
		ensure_backend();
		return id;
	}

	void unregister_instance(u64 owner_id)
	{
		stop(owner_id);

		if (m_registered_instance_count.fetch_sub(1, std::memory_order_acq_rel) == 1)
		{
			close_backend();
		}
	}

	void play(u64 owner_id, std::span<const f32> wave)
	{
		if (wave.empty())
		{
			return;
		}

		if (!ensure_backend())
		{
			return;
		}

		buzzer_voice_slot* target_slot = nullptr;
		u32 target_idx = 0;

		// 1. Reuse a slot already owned by this instance
		for (u32 i = 0; i < MAX_VOICE_SLOTS; ++i)
		{
			if (m_voice_slots[i].owner_id.load(std::memory_order_relaxed) == owner_id)
			{
				target_slot = &m_voice_slots[i];
				target_idx = i;
				break;
			}
		}

		// 2. Claim an idle slot
		if (!target_slot)
		{
			for (u32 i = 0; i < MAX_VOICE_SLOTS; ++i)
			{
				u64 expected = 0;
				if (m_voice_slots[i].owner_id.compare_exchange_strong(expected, owner_id, std::memory_order_acq_rel))
				{
					target_slot = &m_voice_slots[i];
					target_idx = i;
					break;
				}
			}
		}

		// 3. Fallback to slot 0
		if (!target_slot)
		{
			target_slot = &m_voice_slots[0];
			target_idx = 0;
			target_slot->owner_id.store(owner_id, std::memory_order_relaxed);
		}

		target_slot->active.store(false, std::memory_order_release);

		target_slot->wave_data.store(wave.data(), std::memory_order_relaxed);
		target_slot->wave_size.store(static_cast<u32>(wave.size()), std::memory_order_relaxed);
		target_slot->wave_pos.store(0, std::memory_order_relaxed);

		target_slot->active.store(true, std::memory_order_release);

		m_active_mask.fetch_or(1u << target_idx, std::memory_order_release);
	}

	void stop(u64 owner_id)
	{
		if (m_active_mask.load(std::memory_order_acquire) == 0)
		{
			return;
		}

		for (u32 i = 0; i < MAX_VOICE_SLOTS; ++i)
		{
			auto& slot = m_voice_slots[i];
			if (slot.owner_id.load(std::memory_order_relaxed) == owner_id)
			{
				slot.active.store(false, std::memory_order_release);
				slot.owner_id.store(0, std::memory_order_release);
				m_active_mask.fetch_and(~(1u << i), std::memory_order_release);
			}
		}
	}

	bool is_playing(u64 owner_id) const
	{
		if (m_active_mask.load(std::memory_order_acquire) == 0)
		{
			return false;
		}

		for (u32 i = 0; i < MAX_VOICE_SLOTS; ++i)
		{
			const auto& slot = m_voice_slots[i];
			if (slot.owner_id.load(std::memory_order_relaxed) == owner_id &&
				slot.active.load(std::memory_order_acquire))
			{
				return true;
			}
		}

		return false;
	}

	void check_device()
	{
		ensure_backend();
	}

	void notify_settings_changed()
	{
		m_state_flags.fetch_or(FLAG_CONFIG_DIRTY, std::memory_order_release);
	}

private:
	static constexpr u32 FLAG_READY                  = 1u << 0;
	static constexpr u32 FLAG_CONFIG_DIRTY           = 1u << 1;
	static constexpr u32 FLAG_DEVICE_MAYBE_CHANGED   = 1u << 2;
	static constexpr u32 FLAG_BACKEND_ERROR          = 1u << 3;
	static constexpr u32 FLAGS_NEED_REINIT = FLAG_CONFIG_DIRTY | FLAG_DEVICE_MAYBE_CHANGED | FLAG_BACKEND_ERROR;

	void close_backend_locked()
	{
		if (m_backend)
		{
			m_backend->SetWriteCallback(nullptr);
			m_backend->SetStateCallback(nullptr);
			m_backend->Close();
			m_backend.reset();
		}
		m_state_flags.fetch_and(~(FLAG_READY | FLAG_DEVICE_MAYBE_CHANGED | FLAG_BACKEND_ERROR | FLAG_CONFIG_DIRTY), std::memory_order_release);
	}

	void close_backend()
	{
		std::lock_guard lock(m_init_mutex);
		close_backend_locked();
	}

	bool ensure_backend()
	{
		// Lock-free fast path: return immediately if backend is healthy
		{
			const u32 flags = m_state_flags.load(std::memory_order_acquire);
			if ((flags & FLAG_READY) && !(flags & FLAGS_NEED_REINIT))
			{
				return true;
			}
		}

		std::unique_lock lock(m_init_mutex);

		// Double-check under lock
		{
			const u32 flags = m_state_flags.load(std::memory_order_acquire);
			if ((flags & FLAG_READY) && !(flags & FLAGS_NEED_REINIT))
			{
				return true;
			}
		}

		m_state_flags.fetch_and(~FLAG_CONFIG_DIRTY, std::memory_order_release);

		const audio_renderer cur_renderer = g_cfg.audio.renderer.get();
		const std::string cur_device = g_cfg.audio.audio_device.get();

		const bool renderer_changed = (m_backend && m_opened_renderer != cur_renderer);
		const bool device_setting_changed = (m_backend && m_opened_device != cur_device);
		// Only call DefaultDeviceChanged() when the OS state callback signalled it
		const bool had_device_change_signal = (m_state_flags.fetch_and(~FLAG_DEVICE_MAYBE_CHANGED, std::memory_order_acq_rel) & FLAG_DEVICE_MAYBE_CHANGED) != 0;
		const bool hw_device_changed = had_device_change_signal && m_backend && m_backend->DefaultDeviceChanged();
		const bool backend_broken = (m_state_flags.load(std::memory_order_acquire) & FLAG_BACKEND_ERROR) || (m_backend && !m_backend->Operational());
		const bool is_null_backend = (m_backend && m_backend->GetName() == "Null"sv);
		const bool should_retry_from_null = is_null_backend && (cur_renderer != audio_renderer::null);

		const auto now = steady_clock::now();

		if (m_backend && !renderer_changed && !device_setting_changed && !hw_device_changed && !backend_broken && !should_retry_from_null)
		{
			m_state_flags.fetch_or(FLAG_READY, std::memory_order_release);
			return true;
		}

		// Rate-limit retries after recent failure
		if (now - m_last_init_fail_time < 500ms)
		{
			if (m_backend)
			{
				m_state_flags.fetch_or(FLAG_READY, std::memory_order_release);
				return true;
			}
			return false;
		}

		if (!g_emu_callbacks.get_audio)
		{
			return false;
		}

		if (m_backend)
		{
			close_backend_locked();
		}

		auto backend = g_emu_callbacks.get_audio();
		if (!backend || !backend->Initialized())
		{
			close_backend_locked();
			m_last_init_fail_time = now;
			return false;
		}

		// Record failure if we got a Null backend when a real one was configured
		if (cur_renderer != audio_renderer::null && backend->GetName() == "Null"sv)
		{
			m_last_init_fail_time = now;
		}

		backend->SetStateCallback([this](AudioStateEvent event)
			{
				if (event == AudioStateEvent::DEFAULT_DEVICE_MAYBE_CHANGED)
				{
					m_state_flags.fetch_or(FLAG_DEVICE_MAYBE_CHANGED, std::memory_order_release);
				}
				else
				{
					m_state_flags.fetch_or(FLAG_BACKEND_ERROR, std::memory_order_release);
				}
			});

		backend->SetWriteCallback([this](u32 byte_cnt, void* buffer) -> u32
			{
				return on_write_callback(byte_cnt, buffer);
			});

		m_opened_device = cur_device;
		m_opened_renderer = cur_renderer;

		if (!backend->Open(cur_device, AudioFreq::FREQ_48K, AudioSampleSize::FLOAT, AudioChannelCnt::STEREO, audio_channel_layout::stereo))
		{
			close_backend_locked();
			m_last_init_fail_time = now;
			return false;
		}

		backend->Play();
		m_backend = std::move(backend);
		m_state_flags.fetch_and(~(FLAG_BACKEND_ERROR | FLAG_DEVICE_MAYBE_CHANGED), std::memory_order_release);
		m_state_flags.fetch_or(FLAG_READY, std::memory_order_release);
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

		u32 mask = m_active_mask.load(std::memory_order_acquire);
		if (!mask)
		{
			std::memset(buffer, 0, byte_cnt);
			return byte_cnt;
		}

		const u32 total_samples = byte_cnt / sizeof(f32);
		f32* const out = static_cast<f32*>(buffer);

		u32 mixed_count = 0;

		while (mask)
		{
			const u32 slot_idx = std::countr_zero(mask);
			mask &= mask - 1;

			auto& slot = m_voice_slots[slot_idx];

			if (!slot.active.load(std::memory_order_acquire))
			{
				continue;
			}

			const f32* const wave = slot.wave_data.load(std::memory_order_relaxed);
			const u32 wave_size = slot.wave_size.load(std::memory_order_relaxed);
			u32 pos = slot.wave_pos.load(std::memory_order_relaxed);

			if (!wave || pos >= wave_size)
			{
				slot.active.store(false, std::memory_order_release);
				slot.owner_id.store(0, std::memory_order_release);
				m_active_mask.fetch_and(~(1u << slot_idx), std::memory_order_release);
				continue;
			}

			const u32 to_write = std::min(wave_size - pos, total_samples);
			const f32* const src = wave + pos;

			if (mixed_count == 0)
			{
				std::memcpy(out, src, to_write * sizeof(f32));
				if (to_write < total_samples)
				{
					std::memset(out + to_write, 0, (total_samples - to_write) * sizeof(f32));
				}
			}
			else
			{
				for (u32 i = 0; i < to_write; ++i)
				{
					out[i] += src[i];
				}
			}

			pos += to_write;
			slot.wave_pos.store(pos, std::memory_order_relaxed);
			mixed_count++;

			if (pos >= wave_size)
			{
				slot.active.store(false, std::memory_order_release);
				slot.owner_id.store(0, std::memory_order_release);
				m_active_mask.fetch_and(~(1u << slot_idx), std::memory_order_release);
			}
		}

		if (mixed_count == 0)
		{
			std::memset(buffer, 0, byte_cnt);
		}
		else if (mixed_count > 1)
		{
			for (u32 i = 0; i < total_samples; ++i)
			{
				out[i] = std::clamp(out[i], -1.0f, 1.0f);
			}
		}

		return byte_cnt;
	}

	std::atomic<u32> m_active_mask{0};
	static constexpr usz MAX_VOICE_SLOTS = sizeof(m_active_mask) * 8;
	std::array<buzzer_voice_slot, MAX_VOICE_SLOTS> m_voice_slots;
	std::atomic<u32> m_registered_instance_count{0};

	std::mutex m_init_mutex;
	std::shared_ptr<AudioBackend> m_backend;
	std::string m_opened_device;
	audio_renderer m_opened_renderer{audio_renderer::null};
	std::atomic<u32> m_state_flags{0};
	steady_clock::time_point m_last_init_fail_time{};
};

buzzer_player::buzzer_player()
	: m_instance_id([]() -> u64
	{
		if (g_fxo->is_init())
		{
			g_fxo->need<buzzer_waveforms>();
			g_fxo->need<buzzer_audio_manager>();
			return g_fxo->get<buzzer_audio_manager>().register_instance();
		}
		return 0;
	}())
{
}

buzzer_player::~buzzer_player()
{
	if (m_instance_id != 0)
	{
		if (auto* mgr = g_fxo->try_get<buzzer_audio_manager>())
		{
			mgr->unregister_instance(m_instance_id);
		}
	}
}

void buzzer_player::play_beep_1()
{
	if (const auto* waves = g_fxo->try_get<buzzer_waveforms>())
	{
		play(waves->beep_1);
	}
}

void buzzer_player::play_beep_3()
{
	if (const auto* waves = g_fxo->try_get<buzzer_waveforms>())
	{
		play(waves->beep_3);
	}
}

void buzzer_player::play_scale()
{
	if (const auto* waves = g_fxo->try_get<buzzer_waveforms>())
	{
		play(waves->scale);
	}
}

void buzzer_player::stop()
{
	if (m_instance_id == 0)
	{
		return;
	}

	if (auto* mgr = g_fxo->try_get<buzzer_audio_manager>())
	{
		mgr->stop(m_instance_id);
	}
}

bool buzzer_player::is_playing() const
{
	if (m_instance_id == 0)
	{
		return false;
	}

	if (const auto* mgr = g_fxo->try_get<buzzer_audio_manager>())
	{
		return mgr->is_playing(m_instance_id);
	}

	return false;
}

void buzzer_player::check_device()
{
	if (m_instance_id == 0)
	{
		return;
	}

	if (auto* mgr = g_fxo->try_get<buzzer_audio_manager>())
	{
		mgr->check_device();
	}
}

void buzzer_player::notify_settings_changed()
{
	if (auto* mgr = g_fxo->try_get<buzzer_audio_manager>())
	{
		mgr->notify_settings_changed();
	}
}

void buzzer_player::play(std::span<const f32> wave)
{
	if (m_instance_id == 0 || wave.empty())
	{
		return;
	}

	if (auto* mgr = g_fxo->try_get<buzzer_audio_manager>())
	{
		mgr->play(m_instance_id, wave);
	}
}
