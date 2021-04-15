#pragma once

#include "Emu/Audio/AudioBackend.h"
#include "3rdparty/OpenAL/include/alext.h"

class OpenALBackend : public AudioBackend
{
private:
	ALint m_format{};
	ALuint m_source{};
	ALuint m_buffers[MAX_AUDIO_BUFFERS]{};
	ALsizei m_num_buffers{};

	u32 m_next_buffer = 0;
	u32 m_num_unqueued = 0;

	void unqueue_processed();

public:
	OpenALBackend();
	~OpenALBackend() override;

	const char* GetName() const override { return "OpenAL"; }

	static const u32 capabilities = PLAY_PAUSE_FLUSH | IS_PLAYING | GET_NUM_ENQUEUED_SAMPLES | SET_FREQUENCY_RATIO;
	u32 GetCapabilities() const override { return capabilities; }

	void Open(u32 num_buffers) override;
	void Close() override;

	void Play() override;
	void Pause() override;
	bool IsPlaying() override;

	bool AddData(const void* src, u32 num_samples) override;
	void Flush() override;

	u64 GetNumEnqueuedSamples() override;
	f32 SetFrequencyRatio(f32 new_ratio) override;
};
