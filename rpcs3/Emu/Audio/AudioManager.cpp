#include "stdafx.h"
#include "Emu/System.h"
#include "AudioManager.h"
#include "Emu/state.h"

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

	m_audio_out = Emu.GetCallbacks().get_audio();
}

void AudioManager::Close()
{
	m_audio_out.reset();
}
