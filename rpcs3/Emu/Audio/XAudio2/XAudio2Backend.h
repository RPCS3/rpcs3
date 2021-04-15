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
	Microsoft::WRL::ComPtr<IXAudio2> m_xaudio2_instance{};
	IXAudio2MasteringVoice* m_master_voice{};
	IXAudio2SourceVoice* m_source_voice{};

public:
	XAudio2Backend();
	~XAudio2Backend() override;

	XAudio2Backend(const XAudio2Backend&) = delete;
	XAudio2Backend& operator=(const XAudio2Backend&) = delete;

	const char* GetName() const override { return "XAudio2"; }

	static const u32 capabilities = PLAY_PAUSE_FLUSH | IS_PLAYING | GET_NUM_ENQUEUED_SAMPLES | SET_FREQUENCY_RATIO;
	u32 GetCapabilities() const override { return capabilities;	}

	bool Initialized() const override { return m_xaudio2_instance != nullptr; }

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
