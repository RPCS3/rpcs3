#pragma once

#include "Emu/Audio/AudioBackend.h"

class NullAudioBackend : public AudioBackend
{
public:
	NullAudioBackend() {}
	~NullAudioBackend() {}

	std::string_view GetName() const override { return "Null"sv; }

	void Open(AudioFreq /* freq */, AudioSampleSize /* sample_size */, AudioChannelCnt /* ch_cnt */) override { m_playing = false; }
	void Close() override { m_playing = false; }

	void SetWriteCallback(std::function<u32(u32, void *)> /* cb */) override {};
	f64 GetCallbackFrameLen() override { return 0.01; };

	void Play() override { m_playing = true; }
	void Pause() override { m_playing = false; }
	bool IsPlaying() override { return m_playing; }

private:
	bool m_playing = false;
};
