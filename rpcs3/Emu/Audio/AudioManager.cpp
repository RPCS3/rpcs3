#include "stdafx.h"
#include "Emu/System.h"
#include "AudioManager.h"

void AudioManager::Init()
{
	if (!m_audio_out) m_audio_out = Emu.GetCallbacks().get_audio();
}

void AudioManager::Close()
{
	m_audio_out.reset();
}
