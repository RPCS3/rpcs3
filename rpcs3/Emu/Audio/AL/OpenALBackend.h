#pragma once

#include "Emu/Audio/AudioBackend.h"
#include "3rdparty/OpenAL/include/alext.h"
#include <memory>

class OpenALBackend : public AudioBackend
{
private:
	ALint m_format;
	ALuint m_source;
	ALuint m_buffers[MAX_AUDIO_BUFFERS];
	ALsizei m_num_buffers;

	u32 m_next_buffer = 0;
	u32 m_num_unqueued = 0;

	void unqueue_processed();

public:
	OpenALBackend();
	virtual ~OpenALBackend() override;

	virtual const char* GetName() const override { return "OpenAL"; }

	static const u32 capabilities = PLAY_PAUSE_FLUSH | IS_PLAYING | GET_NUM_ENQUEUED_SAMPLES | SET_FREQUENCY_RATIO;
	virtual u32 GetCapabilities() const override { return capabilities; }

	virtual void Open(u32 num_buffers) override;
	virtual void Close() override;

	virtual void Play() override;
	virtual void Pause() override;
	virtual bool IsPlaying() override;

	virtual bool AddData(const void* src, u32 size) override;
	virtual void Flush() override;

	virtual u64 GetNumEnqueuedSamples() override;
	virtual f32 SetFrequencyRatio(f32 new_ratio) override;
};