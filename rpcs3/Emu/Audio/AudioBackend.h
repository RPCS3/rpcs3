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
		NON_BLOCKING = 0x1,
		IS_PLAYING = 0x2,
		GET_NUM_ENQUEUED_SAMPLES = 0x4,
		SET_FREQUENCY_RATIO = 0x8,
	};

	virtual ~AudioBackend() = default;

	// Callbacks
	virtual const char* GetName() const = 0;
	virtual u32 GetCapabilities() const = 0;

	virtual void Open() = 0;
	virtual void Close() = 0;

	virtual void Play() = 0;
	virtual void Pause() = 0;

	virtual bool IsPlaying()
	{
		fmt::throw_exception("IsPlaying() not implemented");
	};

	virtual bool AddData(const void* src, int size) = 0;
	virtual void Flush() = 0;

	virtual u64 GetNumEnqueuedSamples()
	{
		fmt::throw_exception("GetNumEnqueuedSamples() not implemented");
		return 0;
	}

	virtual f32 SetFrequencyRatio(f32 new_ratio) // returns the new ratio
	{
		fmt::throw_exception("SetFrequencyRatio() not implemented");
		return 1.0f;
	}

	// Helper methods
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

	bool has_capability(Capabilities cap) const
	{
		return (cap & GetCapabilities()) != 0;
	}

	void dump_capabilities(std::string& out) const
	{
		u32 count = 0;
		u32 capabilities = GetCapabilities();

		if (capabilities & NON_BLOCKING)
		{
			fmt::append(out, "NON_BLOCKING");
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
