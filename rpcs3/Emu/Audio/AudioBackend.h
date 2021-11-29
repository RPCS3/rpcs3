#pragma once

#include "util/types.hpp"
#include "Utilities/StrFmt.h"

enum : u32
{
	DEFAULT_AUDIO_SAMPLING_RATE = 48000,
	MAX_AUDIO_BUFFERS = 64,
	AUDIO_BUFFER_SAMPLES = 256,
	AUDIO_MIN_LATENCY = 512,
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

enum class AudioChannelCnt : u32
{
	STEREO       = 2,
	SURROUND_5_1 = 6,
	SURROUND_7_1 = 8,
};

class AudioBackend
{
public:
	enum Capabilities : u32
	{
		SET_FREQUENCY_RATIO = 0x1, // Implements SetFrequencyRatio
	};

	AudioBackend();

	virtual ~AudioBackend() = default;

	/*
	 * Pure virtual methods
	 */
	virtual std::string_view GetName() const = 0;
	virtual u32 GetCapabilities() const = 0;

	virtual void Open(AudioFreq freq, AudioSampleSize sample_size, AudioChannelCnt ch_cnt) = 0;
	virtual void Close() = 0;

	// Sets write callback. It's called when backend requests new data to be sent
	// Callback should return number of submitted bytes. Calling other backend functions from callback is unsafe
	virtual void SetWriteCallback(std::function<u32(u32 /* byte_cnt */, void * /* buffer */)> cb) = 0;

	// Returns length of one callback frame in seconds.
	virtual f64 GetCallbackFrameLen() = 0;

	// Returns true if audio is currently being played, false otherwise
	virtual bool IsPlaying() = 0;

	// Start playing enqueued data
	virtual void Play() = 0;

	// Pause playing enqueued data
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
	 * Virtual methods - should be implemented depending on backend capabilities
	 */

	// Sets a new frequency ratio. Backend is allowed to modify the ratio value, e.g. clamping it to the allowed range
	// Returns the new frequency ratio set
	// Should be implemented if capabilities & SET_FREQUENCY_RATIO
	virtual f32 SetFrequencyRatio(f32 /* new_ratio */) // returns the new ratio
	{
		fmt::throw_exception("SetFrequencyRatio() not implemented");
		return 1.0f;
	}

	/*
	 * Helper methods
	 */
	u32 get_sampling_rate() const;

	u32 get_sample_size() const;

	u32 get_channels() const;

	bool get_convert_to_s16() const;

	bool has_capability(u32 cap) const;

	void dump_capabilities(std::string& out) const;

protected:
	AudioSampleSize m_sample_size = AudioSampleSize::FLOAT;
	AudioFreq       m_sampling_rate = AudioFreq::FREQ_48K;
	AudioChannelCnt m_channels = AudioChannelCnt::STEREO;
};
