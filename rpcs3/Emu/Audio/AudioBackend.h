#pragma once

#include "util/types.hpp"
#include "util/logs.hpp"
#include "Utilities/mutex.h"
#include "Utilities/StrFmt.h"
#include "Emu/system_config_types.h"
#include <numbers>

enum : u32
{
	DEFAULT_AUDIO_SAMPLING_RATE = 48000,
	MAX_AUDIO_BUFFERS = 64,
	AUDIO_BUFFER_SAMPLES = 256,
	AUDIO_MAX_CHANNELS = 8,
};

enum class AudioFreq : u32
{
	FREQ_32K  = 32000,
	FREQ_44K  = 44100,
	FREQ_48K  = 48000,
	FREQ_88K  = 88200,
	FREQ_96K  = 96000,
	FREQ_176K = 176400,
	FREQ_192K = 192000,
};

enum class AudioSampleSize : u32
{
	FLOAT = sizeof(float),
	S16 = sizeof(s16),
};

// This enum is only used for emulation
enum class AudioChannelCnt : u32
{
	STEREO       = 2,
	SURROUND_5_1 = 6,
	SURROUND_7_1 = 8,
};

enum class AudioStateEvent : u32
{
	UNSPECIFIED_ERROR,
	DEFAULT_DEVICE_MAYBE_CHANGED,
};

class AudioBackend
{
public:

	struct VolumeParam
	{
		f32 initial_volume = 1.0f;
		f32 current_volume = 1.0f;
		f32 target_volume = 1.0f;
		u32 freq = 48000;
		u32 ch_cnt = 2;
	};

	AudioBackend();

	virtual ~AudioBackend() = default;

	/*
	 * Virtual methods
	 */
	virtual std::string_view GetName() const = 0;

	// (Re)create output stream with new parameters. Blocks until data callback returns.
	// If dev_id is empty, then default device will be selected.
	// May override channel count if device has smaller number of channels.
	// Should return 'true' on success.
	virtual bool Open(std::string_view dev_id, AudioFreq freq, AudioSampleSize sample_size, AudioChannelCnt ch_cnt, audio_channel_layout layout) = 0;

	// Reset backend state. Blocks until data callback returns.
	virtual void Close() = 0;

	// Sets write callback. It's called when backend requests new data to be sent.
	// Callback should return number of submitted bytes. Calling other backend functions from callback is unsafe.
	virtual void SetWriteCallback(std::function<u32(u32 /* byte_cnt */, void* /* buffer */)> cb);

	// Sets error callback. It's called when backend detects event in audio chain that needs immediate attention.
	// Calling other backend functions from callback is unsafe.
	virtual void SetStateCallback(std::function<void(AudioStateEvent)> cb);

	/*
	 * All functions below require that Open() was called prior.
	 */

	// Returns length of one write callback frame in seconds. Open() must be called prior.
	virtual f64 GetCallbackFrameLen() = 0;

	// Returns true if audio is currently being played, false otherwise. Reflects end result of Play() and Pause() calls.
	virtual bool IsPlaying() { return m_playing; }

	// Start playing enqueued data.
	virtual void Play() = 0;

	// Pause playing enqueued data. No additional callbacks will be issued. Blocks until data callback returns.
	virtual void Pause() = 0;

	/*
	 * This virtual method should be reimplemented if backend can fail to be initialized under non-error conditions
	 * eg. when there is no audio devices attached
	 */
	virtual bool Initialized() { return true; }

	/*
	 * This virtual method should be reimplemented if backend can fail during normal operation
	 */
	virtual bool Operational() { return true; }

	/*
	 * This virtual method should be reimplemented if backend can report device changes
	 */
	virtual bool DefaultDeviceChanged() { return false; }

	/*
	 * Helper methods
	 */
	u32 get_sampling_rate() const;

	u32 get_sample_size() const;

	u32 get_channels() const;
	audio_channel_layout get_channel_layout() const;

	bool get_convert_to_s16() const;

	/*
	 * Convert float buffer to s16 one. src and dst could be the same. cnt is number of buffer elements.
	 */
	static void convert_to_s16(u32 cnt, const f32* src, void* dst);

	/*
	 * Apply volume parameters to the buffer. Gradually changes volume. src and dst could be the same.
	 * Number of channels must be >1 and multiple of 2.
	 * sample_cnt is number of buffer elements. Returns current volume.
	 */
	static f32 apply_volume(const VolumeParam& param, u32 sample_cnt, const f32* src, f32* dst);

	/*
	 * Apply volume value to the buffer. src and dst could be the same. sample_cnt is number of buffer elements.
	 * Returns current volume.
	 */
	static void apply_volume_static(f32 vol, u32 sample_cnt, const f32* src, f32* dst);

	/*
	 * Normalize float samples in range from -1.0 to 1.0.
	 */
	static void normalize(u32 sample_cnt, const f32* src, f32* dst);

	/*
	 * Returns the output channel count and downmix mode.
	 */
	static std::pair<AudioChannelCnt, AudioChannelCnt> get_channel_count_and_downmixer(u32 device_index);

	/*
	 * Returns the max supported channel count.
	 */
	static AudioChannelCnt get_max_channel_count(u32 device_index);

	/*
	 * Get default channel count for a layout
	 */
	static u32 default_layout_channel_count(audio_channel_layout layout);

	/*
	 * Converts raw channel count to value usable by backends
	 */
	static u32 layout_channel_count(u32 channels, audio_channel_layout layout);

	/*
	 * Get the default layout for raw channel count
	 */
	static audio_channel_layout default_layout(u32 channels);

	/*
	 * Downmix audio stream.
	 */
	template<AudioChannelCnt src_ch_cnt, audio_channel_layout dst_layout>
	static void downmix(u32 sample_cnt, const f32* src, f32* dst)
	{
		const u32 dst_ch_cnt = default_layout_channel_count(dst_layout);
		if (static_cast<u32>(src_ch_cnt) <= dst_ch_cnt) fmt::throw_exception("src channel count must be bigger than dst channel count");

		static constexpr f32 center_coef = std::numbers::sqrt2_v<f32> / 2;
		static constexpr f32 surround_coef = std::numbers::sqrt2_v<f32> / 2;

		for (u32 src_sample = 0, dst_sample = 0; src_sample < sample_cnt; src_sample += static_cast<u32>(src_ch_cnt), dst_sample += dst_ch_cnt)
		{
			const f32 left  = src[src_sample + 0];
			const f32 right = src[src_sample + 1];
			
			if constexpr (src_ch_cnt == AudioChannelCnt::STEREO)
			{
				if constexpr (dst_layout == audio_channel_layout::mono)
				{
					dst[dst_sample + 0] = left + right;
				}
			}
			else if constexpr (src_ch_cnt == AudioChannelCnt::SURROUND_5_1)
			{
				const f32 center     = src[src_sample + 2];
				const f32 low_freq   = src[src_sample + 3];
				const f32 side_left  = src[src_sample + 4];
				const f32 side_right = src[src_sample + 5];

				if constexpr (dst_layout == audio_channel_layout::quadraphonic || dst_layout == audio_channel_layout::quadraphonic_lfe)
				{
					const f32 mid = center * center_coef;
					dst[dst_sample + 0] = left + mid;
					dst[dst_sample + 1] = right + mid;
					dst[dst_sample + 2] = side_left;
					dst[dst_sample + 3] = side_right;

					if constexpr (dst_layout == audio_channel_layout::quadraphonic_lfe)
					{
						dst[dst_sample + 4] = low_freq;
					}
				}
				else if constexpr (dst_layout == audio_channel_layout::stereo || dst_layout == audio_channel_layout::stereo_lfe)
				{
					const f32 mid = center * center_coef;
					dst[dst_sample + 0] = left + mid + side_left * surround_coef;
					dst[dst_sample + 1] = right + mid + side_right * surround_coef;

					if constexpr (dst_layout == audio_channel_layout::stereo_lfe)
					{
						dst[dst_sample + 2] = low_freq;
					}
				}
				else if constexpr (dst_layout == audio_channel_layout::mono)
				{
					dst[dst_sample + 0] = left + right + center + side_left + side_right;
				}
			}
			else if constexpr (src_ch_cnt == AudioChannelCnt::SURROUND_7_1)
			{
				const f32 center     = src[src_sample + 2];
				const f32 low_freq   = src[src_sample + 3];
				const f32 rear_left  = src[src_sample + 4];
				const f32 rear_right = src[src_sample + 5];
				const f32 side_left  = src[src_sample + 6];
				const f32 side_right = src[src_sample + 7];

				if constexpr (dst_layout == audio_channel_layout::surround_5_1)
				{
					dst[dst_sample + 0] = left;
					dst[dst_sample + 1] = right;
					dst[dst_sample + 2] = center;
					dst[dst_sample + 3] = low_freq;
					dst[dst_sample + 4] = side_left + rear_left;
					dst[dst_sample + 5] = side_right + rear_right;
				}
				else if constexpr (dst_layout == audio_channel_layout::quadraphonic || dst_layout == audio_channel_layout::quadraphonic_lfe)
				{
					const f32 mid = center * center_coef;
					dst[dst_sample + 0] = left + mid;
					dst[dst_sample + 1] = right + mid;
					dst[dst_sample + 2] = side_left + rear_left;
					dst[dst_sample + 3] = side_right + rear_right;

					if constexpr (dst_layout == audio_channel_layout::quadraphonic_lfe)
					{
						dst[dst_sample + 4] = low_freq;
					}
				}
				else if constexpr (dst_layout == audio_channel_layout::stereo || dst_layout == audio_channel_layout::stereo_lfe)
				{
					const f32 mid = center * center_coef;
					dst[dst_sample + 0] = left + mid + (side_left + rear_left) * surround_coef;
					dst[dst_sample + 1] = right + mid + (side_right + rear_right) * surround_coef;

					if constexpr (dst_layout == audio_channel_layout::stereo_lfe)
					{
						dst[dst_sample + 2] = low_freq;
					}
				}
				else if constexpr (dst_layout == audio_channel_layout::mono)
				{
					dst[dst_sample + 0] = left + right + center + side_left + rear_left + side_right + rear_right;
				}
			}
		}
	}

	static void downmix(u32 sample_cnt, u32 src_ch_cnt, audio_channel_layout dst_layout, const f32* src, f32* dst)
	{
		const u32 dst_ch_cnt = default_layout_channel_count(dst_layout);

		if (src_ch_cnt <= dst_ch_cnt)
		{
			return;
		}

		switch (src_ch_cnt)
		{
		case static_cast<u32>(AudioChannelCnt::SURROUND_7_1):
		{
			switch (dst_layout)
			{
			case audio_channel_layout::mono:
				AudioBackend::downmix<AudioChannelCnt::SURROUND_7_1, audio_channel_layout::mono>(sample_cnt, src, dst);
				break;
			case audio_channel_layout::stereo:
				AudioBackend::downmix<AudioChannelCnt::SURROUND_7_1, audio_channel_layout::stereo>(sample_cnt, src, dst);
				break;
			case audio_channel_layout::stereo_lfe:
				AudioBackend::downmix<AudioChannelCnt::SURROUND_7_1, audio_channel_layout::stereo_lfe>(sample_cnt, src, dst);
				break;
			case audio_channel_layout::quadraphonic:
				AudioBackend::downmix<AudioChannelCnt::SURROUND_7_1, audio_channel_layout::quadraphonic>(sample_cnt, src, dst);
				break;
			case audio_channel_layout::quadraphonic_lfe:
				AudioBackend::downmix<AudioChannelCnt::SURROUND_7_1, audio_channel_layout::quadraphonic_lfe>(sample_cnt, src, dst);
				break;
			case audio_channel_layout::surround_5_1:
				AudioBackend::downmix<AudioChannelCnt::SURROUND_7_1, audio_channel_layout::surround_5_1>(sample_cnt, src, dst);
				break;
			default:
				fmt::throw_exception("Invalid downmix combination: %u -> %s", src_ch_cnt, dst_layout);
			}
			break;
		}
		case static_cast<u32>(AudioChannelCnt::SURROUND_5_1):
		{
			switch (dst_layout)
			{
			case audio_channel_layout::mono:
				AudioBackend::downmix<AudioChannelCnt::SURROUND_5_1, audio_channel_layout::mono>(sample_cnt, src, dst);
				break;
			case audio_channel_layout::stereo:
				AudioBackend::downmix<AudioChannelCnt::SURROUND_5_1, audio_channel_layout::stereo>(sample_cnt, src, dst);
				break;
			case audio_channel_layout::stereo_lfe:
				AudioBackend::downmix<AudioChannelCnt::SURROUND_5_1, audio_channel_layout::stereo_lfe>(sample_cnt, src, dst);
				break;
			case audio_channel_layout::quadraphonic:
				AudioBackend::downmix<AudioChannelCnt::SURROUND_5_1, audio_channel_layout::quadraphonic>(sample_cnt, src, dst);
				break;
			case audio_channel_layout::quadraphonic_lfe:
				AudioBackend::downmix<AudioChannelCnt::SURROUND_5_1, audio_channel_layout::quadraphonic_lfe>(sample_cnt, src, dst);
				break;
			default:
				fmt::throw_exception("Invalid downmix combination: %u -> %s", src_ch_cnt, dst_layout);
			}
			break;
		}
		case static_cast<u32>(AudioChannelCnt::STEREO):
		{
			switch (dst_layout)
			{
			case audio_channel_layout::mono:
				AudioBackend::downmix<AudioChannelCnt::STEREO, audio_channel_layout::mono>(sample_cnt, src, dst);
				break;
			default:
				fmt::throw_exception("Invalid downmix combination: %u -> %s", src_ch_cnt, dst_layout);
			}
			break;
		}
		default:
		{
			fmt::throw_exception("Invalid downmix combination: %u -> %s", src_ch_cnt, dst_layout);
		}
		}
	}

protected:
	void setup_channel_layout(u32 input_channel_count, u32 output_channel_count, audio_channel_layout layout, logs::channel& log);

	AudioSampleSize m_sample_size = AudioSampleSize::FLOAT;
	AudioFreq       m_sampling_rate = AudioFreq::FREQ_48K;
	u32             m_channels = 2;
	audio_channel_layout m_layout = audio_channel_layout::automatic;

	std::timed_mutex m_cb_mutex{};
	std::function<u32(u32, void *)> m_write_callback{};

	shared_mutex m_state_cb_mutex{};
	std::function<void(AudioStateEvent)> m_state_callback{};

	bool m_playing = false;

private:

	static constexpr f32 VOLUME_CHANGE_DURATION = 0.016f; // sec
};
