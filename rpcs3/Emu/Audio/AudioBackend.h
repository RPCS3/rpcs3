#pragma once

#include "Utilities/types.h"
#include "Emu/System.h"

enum : u32
{
	DEFAULT_AUDIO_SAMPLING_RATE = 48000,
	MAX_AUDIO_BUFFERS = 64,
	AUDIO_BUFFER_SAMPLES = 256
};

class AudioBackend
{
public:
	enum Capabilities : u32
	{
		PLAY_PAUSE_FLUSH = 0x1, // AddData implements Play, Pause, Flush
		IS_PLAYING = 0x2, // Implements IsPlaying method
		GET_NUM_ENQUEUED_SAMPLES = 0x4, // Supports GetNumEnqueuedSamples method
		SET_FREQUENCY_RATIO = 0x8, // Implements SetFrequencyRatio method
	};

	virtual ~AudioBackend() = default;

	/*
	 * Pure virtual methods
	 */
	virtual const char* GetName() const = 0;
	virtual u32 GetCapabilities() const = 0;

	virtual void Open(u32 num_buffers) = 0;
	virtual void Close() = 0;

	virtual bool AddData(const void* src, u32 num_samples) = 0;


	/*
	 * Virtual methods - should be implemented depending on backend capabilities
	 */

	// Start playing enqueued data
	// Should be implemented if capabilities & PLAY_PAUSE_FLUSH
	virtual void Play()
	{
		fmt::throw_exception("Play() not implemented");
	}

	// Pause playing enqueued data
	// Should be implemented if capabilities & PLAY_PAUSE_FLUSH
	virtual void Pause()
	{
		fmt::throw_exception("Pause() not implemented");
	}

	// Pause audio, and unqueue all currently queued buffers
	// Should be implemented if capabilities & PLAY_PAUSE_FLUSH
	virtual void Flush()
	{

		fmt::throw_exception("Flush() not implemented");
	}

	// Returns true if audio is currently being played, false otherwise
	// Should be implemented if capabilities & IS_PLAYING
	virtual bool IsPlaying()
	{
		fmt::throw_exception("IsPlaying() not implemented");
	}

	// Returns the number of currently enqueued samples
	// Should be implemented if capabilities & GET_NUM_ENQUEUED_SAMPLES
	virtual u64 GetNumEnqueuedSamples()
	{
		fmt::throw_exception("GetNumEnqueuedSamples() not implemented");
		return 0;
	}

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
	static u32 get_sampling_rate()
	{
		const u32 sampling_period_multiplier_u32 = g_cfg.audio.sampling_period_multiplier;

		if (sampling_period_multiplier_u32 == 100)
			return DEFAULT_AUDIO_SAMPLING_RATE;

		const f32 sampling_period_multiplier = sampling_period_multiplier_u32 / 100.0f;
		const f32 sampling_rate_multiplier = 1.0f / sampling_period_multiplier;
		return static_cast<u32>(DEFAULT_AUDIO_SAMPLING_RATE * sampling_rate_multiplier);
	}

	static u32 get_sample_size()
	{
		return g_cfg.audio.convert_to_u16 ? sizeof(u16) : sizeof(float);
	}

	static u32 get_channels()
	{
		return g_cfg.audio.downmix_to_2ch ? 2 : 8;
	}

	bool has_capability(u32 cap) const
	{
		return (cap & GetCapabilities()) == cap;
	}

	void dump_capabilities(std::string& out) const
	{
		u32 count = 0;
		u32 capabilities = GetCapabilities();

		if (capabilities & PLAY_PAUSE_FLUSH)
		{
			fmt::append(out, "PLAY_PAUSE_FLUSH");
			count++;
		}

		if (capabilities & IS_PLAYING)
		{
			fmt::append(out, "%sIS_PLAYING", count > 0 ? " | " : "");
			count++;
		}

		if (capabilities & GET_NUM_ENQUEUED_SAMPLES)
		{
			fmt::append(out, "%sGET_NUM_ENQUEUED_SAMPLES", count > 0 ? " | " : "");
			count++;
		}

		if (capabilities & SET_FREQUENCY_RATIO)
		{
			fmt::append(out, "%sSET_FREQUENCY_RATIO", count > 0 ? " | " : "");
			count++;
		}

		if (count == 0)
		{
			fmt::append(out, "NONE");
		}
	}
};
