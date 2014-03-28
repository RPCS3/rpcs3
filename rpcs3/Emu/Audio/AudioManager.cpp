#include "stdafx.h"
#include "AudioManager.h"

OpenALThread* m_audio_out;

AudioManager::AudioManager()
{
}

void AudioManager::Init()
{
	if(m_audio_out) return;

	m_audio_info.Init();

	switch(Ini.AudioOutMode.GetValue())
	{
	default:
	case 0: m_audio_out = nullptr; break;
	case 1: m_audio_out = new OpenALThread(); break;
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
