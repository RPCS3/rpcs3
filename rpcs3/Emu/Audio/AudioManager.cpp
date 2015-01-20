#include "stdafx.h"
#include "rpcs3/Ini.h"
#include "AudioManager.h"
#include "AL/OpenALThread.h"
#include "Null/NullAudioThread.h"
#include "XAudio2/XAudio2Thread.h"

AudioManager::AudioManager() : m_audio_out(nullptr)
{
}

void AudioManager::Init()
{
	if (m_audio_out) return;

	m_audio_info.Init();

	switch (Ini.AudioOutMode.GetValue())
	{
	default:
	case 0: m_audio_out = new NullAudioThread(); break;
	case 1: m_audio_out = new OpenALThread(); break;
#if defined (_WIN32)
	case 2: m_audio_out = new XAudio2Thread(); break;
#endif
	}
}

void AudioManager::Close()
{
	delete m_audio_out;
	m_audio_out = nullptr;
}

u8 AudioManager::GetState()
{
	return CELL_AUDIO_OUT_OUTPUT_STATE_ENABLED;
}
