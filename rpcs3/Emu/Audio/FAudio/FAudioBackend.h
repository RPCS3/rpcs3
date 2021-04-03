#pragma once

#ifndef HAVE_FAUDIO
#error "FAudio support disabled but still being built."
#endif

#include "Emu/Audio/AudioBackend.h"
#include "3rdparty/FAudio/include/FAudio.h"

class FAudioBackend : public AudioBackend
{
private:
	FAudio* m_instance{};
	FAudioMasteringVoice* m_master_voice{};
	FAudioSourceVoice* m_source_voice{};

public:
	FAudioBackend();
	virtual ~FAudioBackend() override;

	FAudioBackend(const FAudioBackend&) = delete;
	FAudioBackend& operator=(const FAudioBackend&) = delete;

	virtual const char* GetName() const override
	{
		return "FAudio";
	};

	static const u32 capabilities = PLAY_PAUSE_FLUSH | IS_PLAYING | GET_NUM_ENQUEUED_SAMPLES | SET_FREQUENCY_RATIO;
	virtual u32 GetCapabilities() const override
	{
		return capabilities;
	};

	virtual void Open(u32 /* num_buffers */) override;
	virtual void Close() override;

	virtual void Play() override;
	virtual void Pause() override;
	virtual bool IsPlaying() override;

	virtual bool AddData(const void* src, u32 num_samples) override;
	virtual void Flush() override;

	virtual u64 GetNumEnqueuedSamples() override;
	virtual f32 SetFrequencyRatio(f32 new_ratio) override;
};
