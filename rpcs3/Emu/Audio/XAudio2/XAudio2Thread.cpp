#ifdef _WIN32

#include "Utilities/Log.h"
#include "Utilities/StrFmt.h"

#include "XAudio2Thread.h"
#include <Windows.h>

XAudio2Thread::XAudio2Thread()
{
	if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL))
	{
		LOG_ERROR(GENERAL, "XAudio: failed to increase thread priority");
	}

	if (auto lib2_9 = LoadLibraryExW(L"XAudio2_9.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32))
	{
		// xa28* implementation is fully compatible with library 2.9
		xa28_init(lib2_9);

		m_funcs.destroy    = &xa28_destroy;
		m_funcs.play       = &xa28_play;
		m_funcs.flush      = &xa28_flush;
		m_funcs.stop       = &xa28_stop;
		m_funcs.open       = &xa28_open;
		m_funcs.is_playing = &xa28_is_playing;
		m_funcs.add        = &xa28_add;

		LOG_SUCCESS(GENERAL, "XAudio 2.9 initialized");
		return;
	}

	if (auto lib2_7 = LoadLibraryExW(L"XAudio2_7.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32))
	{
		xa27_init(lib2_7);

		m_funcs.destroy    = &xa27_destroy;
		m_funcs.play       = &xa27_play;
		m_funcs.flush      = &xa27_flush;
		m_funcs.stop       = &xa27_stop;
		m_funcs.open       = &xa27_open;
		m_funcs.is_playing = &xa27_is_playing;
		m_funcs.add        = &xa27_add;

		LOG_SUCCESS(GENERAL, "XAudio 2.7 initialized");
		return;
	}
	
	if (auto lib2_8 = LoadLibraryExW(L"XAudio2_8.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32))
	{
		xa28_init(lib2_8);

		m_funcs.destroy    = &xa28_destroy;
		m_funcs.play       = &xa28_play;
		m_funcs.flush      = &xa28_flush;
		m_funcs.stop       = &xa28_stop;
		m_funcs.open       = &xa28_open;
		m_funcs.is_playing = &xa28_is_playing;
		m_funcs.add        = &xa28_add;

		LOG_SUCCESS(GENERAL, "XAudio 2.8 initialized");
		return;
	}

	fmt::throw_exception("No supported XAudio2 library found");
}

XAudio2Thread::~XAudio2Thread()
{
	m_funcs.destroy();
}

void XAudio2Thread::Play()
{
	m_funcs.play();
}

void XAudio2Thread::Close()
{
	m_funcs.stop();
	m_funcs.flush();
}

void XAudio2Thread::Pause()
{
	m_funcs.stop();
}

void XAudio2Thread::Open()
{
	m_funcs.open();
}

bool XAudio2Thread::IsPlaying()
{
	return m_funcs.is_playing();
}

bool XAudio2Thread::AddData(const void* src, int size)
{
	return m_funcs.add(src, size);
}

void XAudio2Thread::Flush()
{
	m_funcs.flush();
}

#endif
