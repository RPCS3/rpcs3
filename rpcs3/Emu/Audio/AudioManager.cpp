#include "stdafx.h"
#include "AudioManager.h"
#include "AL/OpenALThread.h"
#include "Emu/state.h"
#include "Null/NullAudioThread.h"
#include "XAudio2/XAudio2Thread.h"

AudioManager::AudioManager() : m_audio_out(nullptr)
{
}

AudioManager::~AudioManager()
{
	Close();
}

void AudioManager::Init()
{
	if (m_audio_out) return;

	m_audio_info.Init();

	switch (rpcs3::state.config.audio.out.value())
	{
	default:
	case audio_output_type::Null: m_audio_out = new NullAudioThread(); break;
	case audio_output_type::OpenAL: m_audio_out = new OpenALThread(); break;
#if defined (_WIN32)
	case audio_output_type::XAudio2: m_audio_out = new XAudio2Thread(); break;
#endif
	}
}

void AudioManager::Close()
{
	delete m_audio_out;
	m_audio_out = nullptr;
}
