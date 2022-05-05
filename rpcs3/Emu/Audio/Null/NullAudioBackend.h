#pragma once

#include "Emu/Audio/AudioBackend.h"

class NullAudioBackend final : public AudioBackend
{
public:
	NullAudioBackend() {}
	~NullAudioBackend() {}

	std::string_view GetName() const override { return "Null"sv; }

	bool Open(AudioFreq /* freq */, AudioSampleSize /* sample_size */, AudioChannelCnt /* ch_cnt */) override
	{
		Close();
		return true;
	}
	void Close() override { m_playing = false; }

	void SetWriteCallback(std::function<u32(u32, void *)> /* cb */) override {};
	f64 GetCallbackFrameLen() override { return 0.01; };

	void SetErrorCallback(std::function<void()> /* cb */) override {};

	void Play() override { m_playing = true; }
	void Pause() override { m_playing = false; }
	bool IsPlaying() override { return m_playing; }

private:
	bool m_playing = false;
};
