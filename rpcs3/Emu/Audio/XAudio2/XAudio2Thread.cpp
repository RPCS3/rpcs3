#ifdef _WIN32

#include "Utilities/Log.h"
#include "Utilities/StrFmt.h"
#include "Utilities/Config.h"
#include "Emu/System.h"

#include "XAudio2Thread.h"
#include <Windows.h>

extern cfg::bool_entry g_cfg_audio_convert_to_u16;

XAudio2Thread::XAudio2Thread()
	: m_xaudio(LoadLibraryA("xaudio2_9.dll"))
{
	m_xaudio ? xa29_init(m_xaudio) : xa27_init();
}

XAudio2Thread::~XAudio2Thread()
{
	m_xaudio ? xa29_destroy() : xa27_destroy();

	FreeLibrary((HMODULE)m_xaudio);
}

void XAudio2Thread::Play()
{
	m_xaudio ? xa29_play() : xa27_play();
}

void XAudio2Thread::Close()
{
	Stop();
	m_xaudio ? xa29_flush() : xa27_flush();
}

void XAudio2Thread::Stop()
{
	m_xaudio ? xa29_stop() : xa27_stop();
}

void XAudio2Thread::Open(const void* src, int size)
{
	m_xaudio ? xa29_open() : xa27_open();
	AddData(src, size);
	Play();
}

void XAudio2Thread::AddData(const void* src, int size)
{
	m_xaudio ? xa29_add(src, size) : xa27_add(src, size);
}

#endif
