#pragma once

#include "AudioThread.h"

// it cannot be configured currently, and it must NOT use cellSysutil definitions
struct AudioInfo
{
	AudioInfo()
	{
	}

	void Init()
	{
	}
};

class AudioManager
{
	AudioInfo m_audio_info;
	AudioThread* m_audio_out;
public:
	AudioManager();
	~AudioManager();

	void Init();
	void Close();

	AudioThread& GetAudioOut() { assert(m_audio_out); return *m_audio_out; }
	AudioInfo& GetInfo() { return m_audio_info; }
};
