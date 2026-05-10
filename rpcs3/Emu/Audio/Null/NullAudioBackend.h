#pragma once

#include "Emu/Audio/AudioBackend.h"

class NullAudioBackend final : public AudioBackend
{
public:
	NullAudioBackend() {}
	~NullAudioBackend() {}

	std::string_view GetName() const override { return "Null"sv; }

	bool Open(std::string_view /* dev_id */, AudioFreq /* freq */, AudioSampleSize /* sample_size */, AudioChannelCnt /* ch_cnt */, audio_channel_layout /*layout*/) override
	{
		Close();
		return true;
	}
	void Close() override { m_playing = false; }

	f64 GetCallbackFrameLen() override { return 0.01; };

	void Play() override { m_playing = true; }
	void Pause() override { m_playing = false; }
	bool IsPlaying() override { return m_playing; }

private:
	bool m_playing = false;
};
