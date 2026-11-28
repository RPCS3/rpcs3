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
#include <span>

namespace
{
	constexpr u32 BUZZER_SAMPLE_RATE = 48000;
	constexpr f32 BUZZER_VOLUME = 0.5f;
	constexpr u32 CHANNELS = 2;
	constexpr u32 BEEP_FREQ_HZ = 4186;  // NOTE_C8 (~4.2 kHz buzzer resonance peak)
	constexpr u32 COUNT_FREQ_HZ = 1175; // NOTE_D6

	struct note_segment
	{
		u32 freq;
		u32 duration_ms;
	};

	constexpr usz ms_to_frames(u32 ms)
	{
		return static_cast<usz>(BUZZER_SAMPLE_RATE) * ms / 1000;
	}

	constexpr usz ms_to_samples(u32 ms)
	{
		return ms_to_frames(ms) * CHANNELS;
	}

	// Fade envelope durations (cover enough zero-crossings at 523–4186 Hz)
	constexpr usz FADE_IN_FRAMES  = ms_to_frames(1);   // 1.0 ms = 48 frames
	constexpr usz FADE_OUT_FRAMES = ms_to_frames(4);   // 4.0 ms = 192 frames

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

	// Synthesize continuous-phase stereo waveform from note segments and apply fade envelope
	std::vector<f32> generate_sequence_wave(std::span<const note_segment> notes)
	{
		usz total_frames = 0;
		for (const auto& note : notes)
		{
			total_frames += ms_to_frames(note.duration_ms);
		}

		std::vector<f32> wave(total_frames * CHANNELS);
		f64 phase = 0.0;
		usz idx = 0;

		for (const auto& note : notes)
		{
			const usz frames = ms_to_frames(note.duration_ms);
			const f64 step = TWO_PI * note.freq / BUZZER_SAMPLE_RATE;
			const harmonic_coeffs hc(note.freq);
			for (usz i = 0; i < frames; ++i)
			{
				const f32 sample = static_cast<f32>(synthesize_buzzer_sample(phase, hc));
				wave[idx++] = sample;
				wave[idx++] = sample;
				phase += step;
				if (phase >= TWO_PI)
				{
					phase -= TWO_PI;
				}
			}
		}

		finalize_waveform(wave);
		return wave;
	}

	std::vector<f32> generate_tone_wave(u32 freq, u32 duration_ms)
	{
		const note_segment seg{freq, duration_ms};
		return generate_sequence_wave({&seg, 1});
	}

	void append_tone_with_gap(std::vector<f32>& wave, std::span<const f32> tone, usz gap_samples)
	{
		wave.insert(wave.end(), tone.begin(), tone.end());
		wave.insert(wave.end(), gap_samples, 0.0f);
	}

	// Generate stereo-interleaved 4186 Hz beep sequence: count x [60 ms tone, 30 ms silence]
	std::vector<f32> generate_beep_wave(u32 count)
	{
		const std::vector<f32> tone = generate_tone_wave(BEEP_FREQ_HZ, 60);
		constexpr usz interval_samples = ms_to_samples(30);

		std::vector<f32> wave;
		wave.reserve(count * (tone.size() + interval_samples));

		for (u32 i = 0; i < count; ++i)
		{
			append_tone_with_gap(wave, tone, interval_samples);
		}

		return wave;
	}

	// Generate stereo-interleaved 0x04 short-long alert: 4 groups of [60 ON, 60 OFF, 480 ON, 60 OFF]
	std::vector<f32> generate_short_long_wave()
	{
		constexpr u32 groups = 4;
		const std::vector<f32> short_tone = generate_tone_wave(BEEP_FREQ_HZ, 60);
		const std::vector<f32> long_tone = generate_tone_wave(BEEP_FREQ_HZ, 480);
		constexpr usz interval_samples = ms_to_samples(60);

		std::vector<f32> wave;
		wave.reserve(groups * (short_tone.size() + long_tone.size() + 2 * interval_samples));

		for (u32 i = 0; i < groups; ++i)
		{
			append_tone_with_gap(wave, short_tone, interval_samples);
			append_tone_with_gap(wave, long_tone, interval_samples);
		}

		return wave;
	}

	// Generate stereo-interleaved 1175 Hz (NOTE_D6) count sequence: short_count x [40 ms tone, 40 ms silence] + 120 ms tone
	std::vector<f32> generate_count_wave(u32 short_count)
	{
		const std::vector<f32> short_tone = generate_tone_wave(COUNT_FREQ_HZ, 40);
		const std::vector<f32> long_tone = generate_tone_wave(COUNT_FREQ_HZ, 120);
		constexpr usz interval_samples = ms_to_samples(40);

		std::vector<f32> wave;
		wave.reserve(short_count * (short_tone.size() + interval_samples) + long_tone.size());

		for (u32 i = 0; i < short_count; ++i)
		{
			append_tone_with_gap(wave, short_tone, interval_samples);
		}

		wave.insert(wave.end(), long_tone.begin(), long_tone.end());
		return wave;
	}

	constexpr std::array<note_segment, 2> DESCENDING_ALERT_NOTES = {{{6272, 480}, {5274, 600}}};
	constexpr std::array<u32, 13> SIREN_FREQS = {3520, 3729, 3951, 4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040};
	constexpr std::array<u32, 8> ARPEGGIO_FREQS = {587, 659, 698, 784, 880, 988, 1047, 1109};
	constexpr std::array<u32, 8> SCALE_C5_C6_FREQS = {523, 587, 659, 698, 784, 880, 988, 1047};
	constexpr std::array<u32, 8> SCALE_C6_C7_FREQS = {1047, 1175, 1319, 1397, 1568, 1760, 1976, 2093};

	std::vector<note_segment> build_scale_notes(std::span<const u32> freqs, u32 note_ms, u32 repeat = 1)
	{
		std::vector<note_segment> notes;
		notes.reserve(freqs.size() * repeat);

		for (u32 r = 0; r < repeat; ++r)
		{
			for (const u32 freq : freqs)
			{
				notes.push_back({freq, note_ms});
			}
		}

		return notes;
	}

	// Generate stereo-interleaved continuous-phase scale/sweep across a frequency sequence
	std::vector<f32> generate_scale_wave(std::span<const u32> freqs, u32 note_ms)
	{
		return generate_sequence_wave(build_scale_notes(freqs, note_ms));
	}

	// Generate stereo-interleaved 0x07 fast triple arpeggio: 3 rounds of 8x 20 ms notes with 40 ms final note (500 ms total)
	std::vector<f32> generate_arpeggio_wave()
	{
		std::vector<note_segment> notes = build_scale_notes(ARPEGGIO_FREQS, 20, 3);
		notes.back().duration_ms = 40;
		return generate_sequence_wave(notes);
	}
} // anonymous namespace

// Precomputed stereo-interleaved waveform cache (g_fxo managed)
struct buzzer_waveforms
{
	std::vector<f32> beep_3 = generate_beep_wave(3); // Shared across 0x01 (first 90 ms), 0x02, and 0x03
	std::vector<f32> short_long_alert = generate_short_long_wave();
	std::vector<f32> descending_alert = generate_sequence_wave(DESCENDING_ALERT_NOTES);
	std::vector<f32> chromatic_siren = generate_scale_wave(SIREN_FREQS, 30);
	std::vector<f32> arpeggio = generate_arpeggio_wave();
	std::vector<f32> scale = generate_scale_wave(SCALE_C5_C6_FREQS, 250);
	std::vector<f32> click = generate_tone_wave(5000, 10);
	std::vector<f32> count_4 = generate_count_wave(4); // Shared suffix across 0x0B..0x0F and 0x1F
	std::vector<f32> high_scale = generate_scale_wave(SCALE_C6_C7_FREQS, 250);

	buzzer_waveforms() = default;
	buzzer_waveforms(const buzzer_waveforms&) = delete;
	buzzer_waveforms& operator=(const buzzer_waveforms&) = delete;
};

struct buzzer_voice_slot
{
	bool active = false;
	bool loop = false;
	u32 wave_pos = 0;
	u32 loop_silence_samples = 0;
	const f32* wave_data = nullptr;
	u32 wave_size = 0;
	u64 owner_id = 0;
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
		const u32 prev_count = m_registered_instance_count.fetch_add(1, std::memory_order_seq_cst);
		ensure_backend(prev_count == 0);
		return id;
	}

	void unregister_instance(u64 owner_id)
	{
		stop(owner_id);

		if (m_registered_instance_count.fetch_sub(1, std::memory_order_acq_rel) == 1)
		{
			std::lock_guard lock(m_init_mutex);
			if (m_registered_instance_count.load(std::memory_order_acquire) == 0)
			{
				// Clear FLAG_READY before the final seq_cst count check so no concurrent
				// register_instance() fast-path can observe FLAG_READY while closing.
				m_state_flags.fetch_and(~FLAG_READY, std::memory_order_seq_cst);
				if (m_registered_instance_count.load(std::memory_order_seq_cst) == 0)
				{
					close_backend_locked();
				}
				else if (m_backend)
				{
					m_state_flags.fetch_or(FLAG_READY, std::memory_order_release);
				}
			}
		}
	}

	void play(u64 owner_id, std::span<const f32> wave, bool loop = false, u32 loop_interval_ms = 0)
	{
		if (wave.empty())
		{
			return;
		}

		if (!ensure_backend())
		{
			return;
		}

		std::lock_guard lock(m_slots_mutex);

		// Reuse existing slot owned by this instance, claim an idle slot, or fall back to slot 0
		u32 target_idx = 0;
		bool found_owner = false;
		u32 first_idle = MAX_VOICE_SLOTS;

		for (u32 i = 0; i < MAX_VOICE_SLOTS; ++i)
		{
			const u64 slot_owner = m_voice_slots[i].owner_id;
			if (slot_owner == owner_id)
			{
				target_idx = i;
				found_owner = true;
				break;
			}
			if (slot_owner == 0 && first_idle == MAX_VOICE_SLOTS)
			{
				first_idle = i;
			}
		}

		if (!found_owner && first_idle < MAX_VOICE_SLOTS)
		{
			target_idx = first_idle;
		}

		auto& target_slot = m_voice_slots[target_idx];
		target_slot.owner_id = owner_id;
		target_slot.wave_data = wave.data();
		target_slot.wave_size = static_cast<u32>(wave.size());
		target_slot.wave_pos = 0;
		target_slot.loop = loop;
		target_slot.loop_silence_samples = loop ? static_cast<u32>(ms_to_samples(loop_interval_ms)) : 0;
		target_slot.active = true;

		m_active_mask |= (1u << target_idx);
	}

	void stop(u64 owner_id)
	{
		std::lock_guard lock(m_slots_mutex);

		u32 mask = m_active_mask;
		while (mask)
		{
			const u32 i = std::countr_zero(mask);
			mask &= mask - 1;

			if (m_voice_slots[i].owner_id == owner_id)
			{
				release_slot(i);
			}
		}
	}

	bool is_playing(u64 owner_id) const
	{
		std::lock_guard lock(m_slots_mutex);

		u32 mask = m_active_mask;
		while (mask)
		{
			const u32 i = std::countr_zero(mask);
			mask &= mask - 1;

			const auto& slot = m_voice_slots[i];
			if (slot.owner_id == owner_id && slot.active)
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

	void release_slot(u32 slot_idx)
	{
		auto& slot = m_voice_slots[slot_idx];
		slot.active = false;
		slot.loop = false;
		slot.loop_silence_samples = 0;
		slot.owner_id = 0;
		m_active_mask &= ~(1u << slot_idx);
	}

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

	bool ensure_backend(bool force_lock = false)
	{
		// Lock-free fast path: return immediately if backend is healthy
		if (!force_lock)
		{
			const u32 flags = m_state_flags.load(std::memory_order_seq_cst);
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

		const u32 total_samples = byte_cnt / sizeof(f32);
		f32* const out = static_cast<f32*>(buffer);
		u32 mixed_count = 0;

		{
			std::lock_guard lock(m_slots_mutex);

			u32 mask = m_active_mask;
			while (mask)
			{
				const u32 slot_idx = std::countr_zero(mask);
				mask &= mask - 1;

				auto& slot = m_voice_slots[slot_idx];
				const u64 cycle_size = static_cast<u64>(slot.wave_size) + slot.loop_silence_samples;
				if (!slot.active || !slot.wave_data || slot.wave_size == 0 || slot.wave_pos >= cycle_size)
				{
					release_slot(slot_idx);
					continue;
				}

				u32 written = 0;
				while (written < total_samples)
				{
					if (slot.wave_pos < slot.wave_size)
					{
						const u32 to_write = std::min(slot.wave_size - slot.wave_pos, total_samples - written);
						const f32* const src = slot.wave_data + slot.wave_pos;
						f32* const dst = out + written;

						if (mixed_count == 0)
						{
							std::memcpy(dst, src, to_write * sizeof(f32));
						}
						else
						{
							for (u32 i = 0; i < to_write; ++i)
							{
								dst[i] += src[i];
							}
						}

						written += to_write;
						slot.wave_pos += to_write;
					}
					else
					{
						const u32 to_silence = static_cast<u32>(std::min<u64>(cycle_size - slot.wave_pos, total_samples - written));
						if (mixed_count == 0)
						{
							std::memset(out + written, 0, to_silence * sizeof(f32));
						}

						written += to_silence;
						slot.wave_pos += to_silence;
					}

					if (slot.wave_pos >= cycle_size)
					{
						if (slot.loop)
						{
							slot.wave_pos = 0;
						}
						else
						{
							break;
						}
					}
				}

				if (mixed_count == 0 && written < total_samples)
				{
					std::memset(out + written, 0, (total_samples - written) * sizeof(f32));
				}

				mixed_count++;

				if (slot.wave_pos >= cycle_size && !slot.loop)
				{
					release_slot(slot_idx);
				}
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

	mutable std::mutex m_slots_mutex;
	u32 m_active_mask = 0;
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

static buzzer_audio_manager* try_get_manager(u64 instance_id)
{
	return instance_id != 0 ? g_fxo->try_get<buzzer_audio_manager>() : nullptr;
}

buzzer_player::buzzer_player()
	: m_instance_id([]() -> u64
	{
		if (g_fxo->is_init())
		{
			static std::mutex s_fxo_init_mutex;
			{
				std::lock_guard lock(s_fxo_init_mutex);
				g_fxo->need<buzzer_waveforms>();
				g_fxo->need<buzzer_audio_manager>();
			}
			return g_fxo->get<buzzer_audio_manager>().register_instance();
		}
		return 0;
	}())
{
}

buzzer_player::~buzzer_player()
{
	if (auto* mgr = try_get_manager(m_instance_id))
	{
		mgr->unregister_instance(m_instance_id);
	}
}

void buzzer_player::play_beep_1()
{
	play(wave_id::beep_1);
}

void buzzer_player::play_beep_3()
{
	play(wave_id::beep_3);
}

void buzzer_player::play_periodic_burst()
{
	play(wave_id::periodic_burst);
}

void buzzer_player::play_short_long_alert()
{
	play(wave_id::short_long_alert);
}

void buzzer_player::play_descending_alert()
{
	play(wave_id::descending_alert);
}

void buzzer_player::play_chromatic_siren()
{
	play(wave_id::chromatic_siren);
}

void buzzer_player::play_arpeggio()
{
	play(wave_id::arpeggio);
}

void buzzer_player::play_scale()
{
	play(wave_id::scale);
}

void buzzer_player::play_click()
{
	play(wave_id::click);
}

void buzzer_player::play_count(u32 count)
{
	play(wave_id::count, count);
}

void buzzer_player::play_high_scale()
{
	play(wave_id::high_scale);
}

void buzzer_player::stop()
{
	if (auto* mgr = try_get_manager(m_instance_id))
	{
		mgr->stop(m_instance_id);
	}
}

bool buzzer_player::is_playing() const
{
	if (const auto* mgr = try_get_manager(m_instance_id))
	{
		return mgr->is_playing(m_instance_id);
	}

	return false;
}

void buzzer_player::check_device()
{
	if (auto* mgr = try_get_manager(m_instance_id))
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

void buzzer_player::play(wave_id id, u32 count)
{
	auto* mgr = try_get_manager(m_instance_id);
	const auto* waves = g_fxo->try_get<buzzer_waveforms>();
	if (!mgr || !waves)
	{
		return;
	}

	std::span<const f32> wave;
	bool loop = false;
	u32 loop_interval_ms = 0;

	switch (id)
	{
	case wave_id::beep_1:
	{
		wave = std::span(waves->beep_3).first(ms_to_samples(90));
		break;
	}
	case wave_id::beep_3:
	{
		wave = waves->beep_3;
		break;
	}
	case wave_id::periodic_burst:
	{
		wave = waves->beep_3;
		loop = true;
		loop_interval_ms = 500;
		break;
	}
	case wave_id::short_long_alert:
	{
		wave = waves->short_long_alert;
		break;
	}
	case wave_id::descending_alert:
	{
		wave = waves->descending_alert;
		break;
	}
	case wave_id::chromatic_siren:
	{
		wave = waves->chromatic_siren;
		loop = true;
		loop_interval_ms = 270;
		break;
	}
	case wave_id::arpeggio:
	{
		wave = waves->arpeggio;
		break;
	}
	case wave_id::scale:
	{
		wave = waves->scale;
		break;
	}
	case wave_id::click:
	{
		wave = waves->click;
		break;
	}
	case wave_id::count:
	{
		wave = std::span(waves->count_4).subspan((4 - std::min(count, 4u)) * ms_to_samples(80));
		break;
	}
	case wave_id::high_scale:
	{
		wave = waves->high_scale;
		break;
	}
	}

	mgr->play(m_instance_id, wave, loop, loop_interval_ms);
}
