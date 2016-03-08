#pragma once

#include "AudioThread.h"

class AudioManager
{
	std::shared_ptr<AudioThread> m_audio_out;

public:
	void Init();
	void Close();

	AudioThread& GetAudioOut() { return *m_audio_out; }
};
