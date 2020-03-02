#pragma once

#ifndef _WIN32
#error "XAudio2 can only be built on Windows."
#endif

#include "Emu/Audio/AudioBackend.h"

#include <xaudio2redist.h>
#include <wrl/client.h>


class XAudio2Backend final : public AudioBackend
{
private:
	Microsoft::WRL::ComPtr<IXAudio2> m_xaudio2_instance;
	IXAudio2MasteringVoice* m_master_voice{};
	IXAudio2SourceVoice* m_source_voice{};

public:
	XAudio2Backend();
	virtual ~XAudio2Backend() override;

	virtual const char* GetName() const override { return "XAudio2"; };

	static const u32 capabilities = PLAY_PAUSE_FLUSH | IS_PLAYING | GET_NUM_ENQUEUED_SAMPLES | SET_FREQUENCY_RATIO;
	virtual u32 GetCapabilities() const override { return capabilities;	};

	virtual bool Initialized() const override { return m_xaudio2_instance != nullptr; }

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
