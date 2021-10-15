#pragma once

#ifndef HAVE_FAUDIO
#error "FAudio support disabled but still being built."
#endif

#include "Emu/Audio/AudioBackend.h"
#include "FAudio.h"

class FAudioBackend : public AudioBackend
{
private:
	FAudio* m_instance{};
	FAudioMasteringVoice* m_master_voice{};
	FAudioSourceVoice* m_source_voice{};

public:
	FAudioBackend();
	~FAudioBackend() override;

	FAudioBackend(const FAudioBackend&) = delete;
	FAudioBackend& operator=(const FAudioBackend&) = delete;

	const char* GetName() const override
	{
		return "FAudio";
	}

	static const u32 capabilities = PLAY_PAUSE_FLUSH | IS_PLAYING | GET_NUM_ENQUEUED_SAMPLES | SET_FREQUENCY_RATIO;
	u32 GetCapabilities() const override
	{
		return capabilities;
	}

	void Open(u32 /* num_buffers */) override;
	void Close() override;

	void Play() override;
	void Pause() override;
	bool IsPlaying() override;

	bool AddData(const void* src, u32 num_samples) override;
	void Flush() override;

	u64 GetNumEnqueuedSamples() override;
	f32 SetFrequencyRatio(f32 new_ratio) override;
};
