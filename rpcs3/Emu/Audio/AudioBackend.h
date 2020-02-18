#pragma once

#include "Utilities/types.h"

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
		PLAY_PAUSE_FLUSH = 0x1, // Implements Play, Pause, Flush
		IS_PLAYING = 0x2, // Implements IsPlaying
		GET_NUM_ENQUEUED_SAMPLES = 0x4, // Implements GetNumEnqueuedSamples
		SET_FREQUENCY_RATIO = 0x8, // Implements SetFrequencyRatio
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
	 * This virtual method should be reimplemented if backend can fail to be initialized under non-error conditions
	 * eg. when there is no audio devices attached
	 */
	virtual bool Initialized() const { return true; }

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
	static u32 get_sampling_rate();

	static u32 get_sample_size();

	static u32 get_channels();

	bool has_capability(u32 cap) const;

	void dump_capabilities(std::string& out) const;
};
